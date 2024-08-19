#include "arrow/api.h"
#include "arrow/compute/api.h"

#include <iostream>

auto RunMain() -> arrow::Status {
    // 创建两列数据
    arrow::Int32Builder int32builder;
    int32_t             some_nums_raw[5] = {34, 624, 2223, 5654, 4356};
    ARROW_RETURN_NOT_OK(int32builder.AppendValues(some_nums_raw, 5));
    std::shared_ptr<arrow::Array> some_nums;
    ARROW_ASSIGN_OR_RAISE(some_nums, int32builder.Finish());

    int32_t more_nums_raw[5] = {75342, 23, 64, 17, 736};
    ARROW_RETURN_NOT_OK(int32builder.AppendValues(more_nums_raw, 5));
    std::shared_ptr<arrow::Array> more_nums;
    ARROW_ASSIGN_OR_RAISE(more_nums, int32builder.Finish());

    // Make a table out of our pair of arrays.
    std::shared_ptr<arrow::Field>  field_a;
    std::shared_ptr<arrow::Field>  field_b;
    std::shared_ptr<arrow::Schema> schema;

    field_a = arrow::field("A", arrow::int32());
    field_b = arrow::field("B", arrow::int32());

    schema = arrow::schema({field_a, field_b});

    std::shared_ptr<arrow::Table> table;
    table = arrow::Table::Make(schema, {some_nums, more_nums}, 5);

    // 任务一: 计算一列的Sum
    arrow::Datum sum;
    ARROW_ASSIGN_OR_RAISE(sum,
                          arrow::compute::Sum({table->GetColumnByName("A")}));
    std::cout << "========== task1 ==========\n";
    std::cout << "Datum kind: " << sum.ToString()
              << " content type: " << sum.type()->ToString() << '\n';
    std::cout << sum.scalar_as<arrow::Int64Scalar>().value << '\n';

    // 任务二: 计算两列元素总和
    arrow::Datum element_wise_sum;
    ARROW_ASSIGN_OR_RAISE(
        element_wise_sum,
        arrow::compute::CallFunction(
            "add",
            {table->GetColumnByName("A"), table->GetColumnByName("B")}));
    std::cout << "========== task2 ==========\n";
    std::cout << "Datum kind: " << element_wise_sum.ToString()
              << " content type: " << element_wise_sum.type()->ToString()
              << '\n';
    // This time, we get a ChunkedArray, not a scalar.
    std::cout << element_wise_sum.chunked_array()->ToString() << '\n';

    // 任务三: 搜索值
    arrow::Datum third_item;
    // 配置索引
    arrow::compute::IndexOptions index_options;
    index_options.value = arrow::MakeScalar(2223);
    ARROW_ASSIGN_OR_RAISE(
        third_item,
        arrow::compute::CallFunction("index",
                                     {

                                         table->GetColumnByName("A")},
                                     &index_options));
    std::cout << "========== task3 ==========\n";
    // Get the kind of Datum and what it holds -- this is a Scalar, with int64
    std::cout << "Datum kind: " << third_item.ToString()
              << " content type: " << third_item.type()->ToString() << '\n';
    // We get a scalar -- the location of 2223 in column A, which is 2 in
    // 0-based indexing.
    std::cout << third_item.scalar_as<arrow::Int64Scalar>().value << '\n';

    return arrow::Status::OK();
}

auto main() -> int {
    arrow::Status status = RunMain();
    if (!status.ok()) {
        std::cerr << status << '\n';
        return 1;
    }
    return 0;
}
