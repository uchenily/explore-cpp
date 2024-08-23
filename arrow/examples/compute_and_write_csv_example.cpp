#include <arrow/api.h>
#include <arrow/compute/api.h>
#include <arrow/csv/api.h>
#include <arrow/csv/writer.h>
#include <arrow/io/api.h>
#include <arrow/result.h>
#include <arrow/status.h>

#include <iostream>
#include <vector>

// 1. 生成两列数据a, b
// 2. 用循环的方式实现 (a>b?) 列
// 3. 用 arrow compute的方式实现 (a>b?) 列
// 4. 4列数据保存到 table
// 5. 将table写入到文件

auto RunMain() -> arrow::Status {
    // Make Arrays
    arrow::NumericBuilder<arrow::Int64Type> int64_builder;
    arrow::BooleanBuilder                   boolean_builder;

    // Make place for 8 values in total
    ARROW_RETURN_NOT_OK(int64_builder.Resize(8));
    ARROW_RETURN_NOT_OK(boolean_builder.Resize(8));

    // Bulk append the given values
    std::vector<int64_t> int64_values = {1, 2, 3, 4, 5, 6, 7, 8};
    ARROW_RETURN_NOT_OK(int64_builder.AppendValues(int64_values));
    std::shared_ptr<arrow::Array> array_a;
    ARROW_RETURN_NOT_OK(int64_builder.Finish(&array_a));
    int64_builder.Reset();
    int64_values = {2, 5, 1, 3, 6, 2, 7, 4};
    std::shared_ptr<arrow::Array> array_b;
    ARROW_RETURN_NOT_OK(int64_builder.AppendValues(int64_values));
    ARROW_RETURN_NOT_OK(int64_builder.Finish(&array_b));

    // Cast the arrays to their actual types
    auto int64_array_a = std::static_pointer_cast<arrow::Int64Array>(array_a);
    auto int64_array_b = std::static_pointer_cast<arrow::Int64Array>(array_b);
    // Explicit comparison of values using a loop
    for (int64_t i = 0; i < 8; i++) {
        if ((!int64_array_a->IsNull(i)) && (!int64_array_b->IsNull(i))) {
            bool comparison_result
                = int64_array_a->Value(i) > int64_array_b->Value(i);
            boolean_builder.UnsafeAppend(comparison_result);
        } else {
            boolean_builder.UnsafeAppendNull();
        }
    }
    std::shared_ptr<arrow::Array> array_a_gt_b_self;
    ARROW_RETURN_NOT_OK(boolean_builder.Finish(&array_a_gt_b_self));
    std::cout << "Array explicitly compared\n";

    // Explicit comparison of values using a compute function
    ARROW_ASSIGN_OR_RAISE(
        arrow::Datum compared_datum,
        arrow::compute::CallFunction("greater", {array_a, array_b}));
    auto array_a_gt_b_compute = compared_datum.make_array();
    std::cout << "Arrays compared using a compute function\n";

    // Create a table for the output
    auto schema
        = arrow::schema({arrow::field("a", arrow::int64()),
                         arrow::field("b", arrow::int64()),
                         arrow::field("a>b? (self written)", arrow::boolean()),
                         arrow::field("a>b? (arrow)", arrow::boolean())});
    std::shared_ptr<arrow::Table> my_table = arrow::Table::Make(
        schema,
        {array_a, array_b, array_a_gt_b_self, array_a_gt_b_compute});

    std::cout << "Table created" << '\n';

    // Write table to CSV file
    auto csv_filename = "compute_and_write_output.csv";
    ARROW_ASSIGN_OR_RAISE(auto outstream,
                          arrow::io::FileOutputStream::Open(csv_filename));

    std::cout << "Writing CSV file" << '\n';
    ARROW_RETURN_NOT_OK(
        arrow::csv::WriteCSV(*my_table,
                             arrow::csv::WriteOptions::Defaults(),
                             outstream.get()));

    return arrow::Status::OK();
}

auto main() -> int {
    arrow::Status status = RunMain();
    if (!status.ok()) {
        std::cerr << status.ToString() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
