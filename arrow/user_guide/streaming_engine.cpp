#include <arrow/acero/exec_plan.h>
#include <arrow/api.h>
#include <arrow/compute/api.h>
#include <arrow/dataset/api.h>
#include <arrow/dataset/plan.h>
#include <arrow/filesystem/api.h>
#include <arrow/table.h>
#include <arrow/util/async_generator.h>

#include <iostream>
#include <memory>

namespace ds = arrow::dataset;
namespace cp = arrow::compute;
namespace ac = arrow::acero;

template <typename TYPE,
          typename = std::enable_if_t<arrow::is_number_type<TYPE>::value
                                      | arrow::is_boolean_type<TYPE>::value
                                      | arrow::is_temporal_type<TYPE>::value>>
auto GetArrayDataSample(const std::vector<typename TYPE::c_type> &values)
    -> arrow::Result<std::shared_ptr<arrow::Array>> {
    using ArrowBuilderType = typename arrow::TypeTraits<TYPE>::BuilderType;
    ArrowBuilderType builder;
    ARROW_RETURN_NOT_OK(builder.Reserve(values.size()));
    ARROW_RETURN_NOT_OK(builder.AppendValues(values));
    return builder.Finish();
}

auto GetTable() -> arrow::Result<std::shared_ptr<arrow::Table>> {
    auto null_long = std::numeric_limits<int64_t>::quiet_NaN();
    ARROW_ASSIGN_OR_RAISE(auto a_array,
                          GetArrayDataSample<arrow::Int64Type>(
                              {1, 2, null_long, 3, null_long, 4, 5, 6, 7, 8}));
    ARROW_ASSIGN_OR_RAISE(auto b_array,
                          GetArrayDataSample<arrow::Int64Type>(
                              {1, 2, null_long, 3, null_long, 4, 5, 6, 7, 8}));
    ARROW_ASSIGN_OR_RAISE(auto c_array,
                          GetArrayDataSample<arrow::Int64Type>(
                              {1, 2, null_long, 3, null_long, 4, 5, 6, 7, 8}));

    auto record_batch
        = arrow::RecordBatch::Make(arrow::schema({
                                       arrow::field("a", arrow::int64()),
                                       arrow::field("b", arrow::int64()),
                                       arrow::field("c", arrow::int64()),
                                   }),
                                   10,
                                   {a_array, b_array, c_array});
    ARROW_ASSIGN_OR_RAISE(auto table,
                          arrow::Table::FromRecordBatches({record_batch}));
    return table;
}

auto create_dataset() -> arrow::Result<std::shared_ptr<ds::Dataset>> {
    ARROW_ASSIGN_OR_RAISE(auto table, GetTable());
    return std::make_shared<arrow::dataset::InMemoryDataset>(table);
}

auto grouped_filtered_mean(std::shared_ptr<ds::Dataset> dataset)
    -> arrow::Status {
    auto ctx = cp::default_exec_context();

    auto options = std::make_shared<ds::ScanOptions>();
    options->use_threads = true;
    options->filter = cp::greater(cp::field_ref("a"), cp::literal(5));
    ARROW_ASSIGN_OR_RAISE(
        auto projection,
        ds::ProjectionDescr::FromNames({"c", "a"}, *dataset->schema()));
    ds::SetProjection(options.get(), projection);

    arrow::acero::BackpressureOptions backpressure
        = arrow::acero::BackpressureOptions::DefaultBackpressure();

    auto scan_node_options
        = ds::ScanNodeOptions{dataset,
                              options,
                              backpressure.should_apply_backpressure()};

    arrow::AsyncGenerator<std::optional<cp::ExecBatch>> sink_gen;
    ARROW_ASSIGN_OR_RAISE(auto plan, ac::ExecPlan::Make(ctx));
    ARROW_RETURN_NOT_OK(
        ac::Declaration::Sequence(
            {
                {     "scan",scan_node_options                             },
                {   "filter",
                 ac::FilterNodeOptions{
                 cp::greater(cp::field_ref("a"), cp::literal(5))}         },
                {  "project",
                 ac::ProjectNodeOptions{
                 {cp::field_ref("c"), cp::field_ref("a")},
                 {"c", "a"}}                                              },
                {"aggregate",
                 ac::AggregateNodeOptions{
                 {{"hash_mean", nullptr, "c", "mean(c)"}},
                 {"a"}}                                                   },
                {     "sink", ac::SinkNodeOptions{&sink_gen, backpressure}}
    })
            .AddToPlan(plan.get()));

    auto schema = arrow::schema({arrow::field("mean(c)", arrow::float64()),
                                 arrow::field("a", arrow::int32())});

    std::shared_ptr<arrow::RecordBatchReader> sink_reader
        = ac::MakeGeneratorReader(schema,
                                  std::move(sink_gen),
                                  ctx->memory_pool());
    ARROW_RETURN_NOT_OK(plan->Validate());

    std::shared_ptr<arrow::Table> response_table;
    plan->StartProducing();
    ARROW_ASSIGN_OR_RAISE(
        response_table,
        arrow::Table::FromRecordBatchReader(sink_reader.get()));
    std::cout << "Results: \n" << response_table->ToString() << '\n';
    plan->StopProducing();
    auto future = plan->finished();
    return future.status();
}

auto main() -> int {
    auto dataset = create_dataset().ValueOrDie();

    ds::internal::Initialize();
    auto status = grouped_filtered_mean(std::move(dataset));
    if (!status.ok()) {
        std::cerr << status.message() << '\n';
    }
}
