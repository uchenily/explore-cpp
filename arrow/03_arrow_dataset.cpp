#include "arrow/api.h"

#include "arrow/csv/api.h"
#include "arrow/dataset/api.h"
#include "arrow/io/api.h"
#include "arrow/ipc/api.h"

#include "parquet/arrow/reader.h"
#include "parquet/arrow/writer.h"

#include <iostream>
#include <unistd.h>

// Generate some data for the rest of this example.
auto CreateTable() -> arrow::Result<std::shared_ptr<arrow::Table>> {
    // This code should look familiar from the basic Arrow example, and is not
    // the focus of this example. However, we need data to work on it, and this
    // makes that!
    auto schema = arrow::schema({arrow::field("a", arrow::int64()),
                                 arrow::field("b", arrow::int64()),
                                 arrow::field("c", arrow::int64())});
    std::shared_ptr<arrow::Array>           array_a;
    std::shared_ptr<arrow::Array>           array_b;
    std::shared_ptr<arrow::Array>           array_c;
    arrow::NumericBuilder<arrow::Int64Type> builder;
    ARROW_RETURN_NOT_OK(builder.AppendValues({0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    ARROW_RETURN_NOT_OK(builder.Finish(&array_a));
    builder.Reset();
    ARROW_RETURN_NOT_OK(builder.AppendValues({9, 8, 7, 6, 5, 4, 3, 2, 1, 0}));
    ARROW_RETURN_NOT_OK(builder.Finish(&array_b));
    builder.Reset();
    ARROW_RETURN_NOT_OK(builder.AppendValues({1, 2, 1, 2, 1, 2, 1, 2, 1, 2}));
    ARROW_RETURN_NOT_OK(builder.Finish(&array_c));
    return arrow::Table::Make(schema, {array_a, array_b, array_c});
}

// Set up a dataset by writing two Parquet files.
auto CreateExampleParquetDataset(
    const std::shared_ptr<arrow::fs::FileSystem> &filesystem,
    const std::string &root_path) -> arrow::Result<std::string> {
    // Much like CreateTable(), this is utility that gets us the dataset we'll
    // be reading from. Don't worry, we also write a dataset in the example
    // proper.
    auto base_path = root_path + "parquet_dataset";
    ARROW_RETURN_NOT_OK(filesystem->CreateDir(base_path));
    // Create an Arrow Table
    ARROW_ASSIGN_OR_RAISE(auto table, CreateTable());
    // Write it into two Parquet files
    ARROW_ASSIGN_OR_RAISE(
        auto output,
        filesystem->OpenOutputStream(base_path + "/data1.parquet"));
    ARROW_RETURN_NOT_OK(parquet::arrow::WriteTable(*table->Slice(0, 5),
                                                   arrow::default_memory_pool(),
                                                   output,
                                                   2048));
    ARROW_ASSIGN_OR_RAISE(
        output,
        filesystem->OpenOutputStream(base_path + "/data2.parquet"));
    ARROW_RETURN_NOT_OK(parquet::arrow::WriteTable(*table->Slice(5),
                                                   arrow::default_memory_pool(),
                                                   output,
                                                   2048));
    return base_path;
}

auto TableToCSV(arrow::Table *table) -> arrow::Status {
    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    ARROW_ASSIGN_OR_RAISE(outfile,
                          arrow::io::FileOutputStream::Open("table.csv"));
    ARROW_ASSIGN_OR_RAISE(auto csv_writer,
                          arrow::csv::MakeCSVWriter(outfile, table->schema()));
    ARROW_RETURN_NOT_OK(csv_writer->WriteTable(*table));
    ARROW_RETURN_NOT_OK(csv_writer->Close());

    return arrow::Status::OK();
}

auto PrepareEnv() -> arrow::Status {
    // Get our environment prepared for reading, by setting up some quick
    // writing.
    ARROW_ASSIGN_OR_RAISE(auto src_table, CreateTable())

    ARROW_RETURN_NOT_OK(TableToCSV(&*src_table));
    std::shared_ptr<arrow::fs::FileSystem> setup_fs;
    // Note this operates in the directory the executable is built in.
    char  setup_path[256];
    char *result = getcwd(setup_path, 256);
    if (result == nullptr) {
        return arrow::Status::IOError("Fetching PWD failed.");
    }

    ARROW_ASSIGN_OR_RAISE(setup_fs,
                          arrow::fs::FileSystemFromUriOrPath(setup_path));
    ARROW_ASSIGN_OR_RAISE(auto dset_path,
                          CreateExampleParquetDataset(setup_fs, ""));

    return arrow::Status::OK();
}

auto RunMain() -> arrow::Status {
    ARROW_RETURN_NOT_OK(PrepareEnv());

    // 任务一: 读取分区的(partitioned)数据集, 保存到table中
    std::shared_ptr<arrow::fs::FileSystem> fs;

    char  init_path[256];
    char *result = getcwd(init_path, 256);
    if (result == nullptr) {
        return arrow::Status::IOError("Fetching PWD failed.");
    }
    ARROW_ASSIGN_OR_RAISE(fs, arrow::fs::FileSystemFromUriOrPath(init_path));
    // A file selector lets us actually traverse a multi-file dataset.
    arrow::fs::FileSelector selector;
    selector.base_dir = "parquet_dataset";
    selector.recursive = true;
    arrow::dataset::FileSystemFactoryOptions options;
    options.partitioning = arrow::dataset::HivePartitioning::MakeFactory();
    auto read_format = std::make_shared<arrow::dataset::ParquetFileFormat>();
    ARROW_ASSIGN_OR_RAISE(
        auto factory,
        arrow::dataset::FileSystemDatasetFactory::Make(fs,
                                                       selector,
                                                       read_format,
                                                       options));

    // 构建dataset
    ARROW_ASSIGN_OR_RAISE(auto read_dataset, factory->Finish());
    // 打印分片
    // Print out the fragments
    ARROW_ASSIGN_OR_RAISE(auto fragments, read_dataset->GetFragments());
    for (const auto &fragment : fragments) {
        std::cout << "Found fragment: " << (*fragment)->ToString() << '\n';
        std::cout << "Partition expression: "
                  << (*fragment)->partition_expression().ToString() << '\n';
    }

    // 将dataset转成table
    ARROW_ASSIGN_OR_RAISE(auto read_scan_builder, read_dataset->NewScan());
    ARROW_ASSIGN_OR_RAISE(auto read_scanner, read_scan_builder->Finish());

    ARROW_ASSIGN_OR_RAISE(std::shared_ptr<arrow::Table> table,
                          read_scanner->ToTable());
    std::cout << table->ToString();

    // 任务二: 将table写入磁盘(分片格式)
    std::shared_ptr<arrow::TableBatchReader> write_dataset
        = std::make_shared<arrow::TableBatchReader>(table);

    auto write_scanner_builder
        = arrow::dataset::ScannerBuilder::FromRecordBatchReader(write_dataset);
    ARROW_ASSIGN_OR_RAISE(auto write_scanner, write_scanner_builder->Finish());
    // The partition schema determines which fields are used as keys for
    // partitioning.
    auto partition_schema = arrow::schema({arrow::field("a", arrow::utf8())});
    // We'll use Hive-style partitioning, which creates directories with
    // "key=value"
    // pairs.
    auto partitioning
        = std::make_shared<arrow::dataset::HivePartitioning>(partition_schema);
    auto write_format = std::make_shared<arrow::dataset::ParquetFileFormat>();

    arrow::dataset::FileSystemDatasetWriteOptions write_options;
    write_options.file_write_options = write_format->DefaultWriteOptions();
    write_options.filesystem = fs;
    write_options.base_dir = "write_dataset";
    write_options.partitioning = partitioning;
    write_options.basename_template = "part{i}.parquet";
    write_options.existing_data_behavior
        = arrow::dataset::ExistingDataBehavior::kOverwriteOrIgnore;

    // 写入磁盘
    ARROW_RETURN_NOT_OK(
        arrow::dataset::FileSystemDataset::Write(write_options, write_scanner));

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
