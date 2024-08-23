#include "arrow/compute/expression.h"
#include <arrow/api.h>
#include <arrow/compute/cast.h>
#include <arrow/dataset/dataset.h>
#include <arrow/dataset/discovery.h>
#include <arrow/dataset/file_base.h>
#include <arrow/dataset/file_ipc.h>
#include <arrow/dataset/file_parquet.h>
#include <arrow/dataset/scanner.h>
#include <arrow/filesystem/filesystem.h>
#include <arrow/ipc/writer.h>
#include <arrow/util/iterator.h>
#include <parquet/arrow/writer.h>

#include <iostream>
#include <vector>

namespace ds = arrow::dataset;
namespace fs = arrow::fs;
namespace cp = arrow::compute;

/**
 * \brief Run Example
 *
 * ./build/examples/dataset_example file:///<some_path>/<some_directory>
 * parquet
 */

// Generate some data for the rest of this example.
auto CreateTable() -> arrow::Result<std::shared_ptr<arrow::Table>> {
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
    const std::shared_ptr<fs::FileSystem> &filesystem,
    const std::string &root_path) -> arrow::Result<std::string> {
    auto base_path = root_path + "/parquet_dataset";
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
                                                   /*chunk_size=*/2048));
    ARROW_ASSIGN_OR_RAISE(
        output,
        filesystem->OpenOutputStream(base_path + "/data2.parquet"));
    ARROW_RETURN_NOT_OK(parquet::arrow::WriteTable(*table->Slice(5),
                                                   arrow::default_memory_pool(),
                                                   output,
                                                   /*chunk_size=*/2048));
    return base_path;
}

// Set up a dataset by writing two Feather files.
auto CreateExampleFeatherDataset(
    const std::shared_ptr<fs::FileSystem> &filesystem,
    const std::string &root_path) -> arrow::Result<std::string> {
    auto base_path = root_path + "/feather_dataset";
    ARROW_RETURN_NOT_OK(filesystem->CreateDir(base_path));
    // Create an Arrow Table
    ARROW_ASSIGN_OR_RAISE(auto table, CreateTable());
    // Write it into two Feather files
    ARROW_ASSIGN_OR_RAISE(
        auto output,
        filesystem->OpenOutputStream(base_path + "/data1.feather"));
    ARROW_ASSIGN_OR_RAISE(
        auto writer,
        arrow::ipc::MakeFileWriter(output.get(), table->schema()));
    ARROW_RETURN_NOT_OK(writer->WriteTable(*table->Slice(0, 5)));
    ARROW_RETURN_NOT_OK(writer->Close());
    ARROW_ASSIGN_OR_RAISE(
        output,
        filesystem->OpenOutputStream(base_path + "/data2.feather"));
    ARROW_ASSIGN_OR_RAISE(
        writer,
        arrow::ipc::MakeFileWriter(output.get(), table->schema()));
    ARROW_RETURN_NOT_OK(writer->WriteTable(*table->Slice(5)));
    ARROW_RETURN_NOT_OK(writer->Close());
    return base_path;
}

// Set up a dataset by writing files with partitioning
auto CreateExampleParquetHivePartitionedDataset(
    const std::shared_ptr<fs::FileSystem> &filesystem,
    const std::string &root_path) -> arrow::Result<std::string> {
    auto base_path = root_path + "/parquet_dataset";
    ARROW_RETURN_NOT_OK(filesystem->CreateDir(base_path));
    // Create an Arrow Table
    auto schema = arrow::schema({arrow::field("a", arrow::int64()),
                                 arrow::field("b", arrow::int64()),
                                 arrow::field("c", arrow::int64()),
                                 arrow::field("part", arrow::utf8())});
    std::vector<std::shared_ptr<arrow::Array>> arrays(4);
    arrow::NumericBuilder<arrow::Int64Type>    builder;
    ARROW_RETURN_NOT_OK(builder.AppendValues({0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    ARROW_RETURN_NOT_OK(builder.Finish(arrays.data()));
    builder.Reset();
    ARROW_RETURN_NOT_OK(builder.AppendValues({9, 8, 7, 6, 5, 4, 3, 2, 1, 0}));
    ARROW_RETURN_NOT_OK(builder.Finish(&arrays[1]));
    builder.Reset();
    ARROW_RETURN_NOT_OK(builder.AppendValues({1, 2, 1, 2, 1, 2, 1, 2, 1, 2}));
    ARROW_RETURN_NOT_OK(builder.Finish(&arrays[2]));
    arrow::StringBuilder string_builder;
    ARROW_RETURN_NOT_OK(string_builder.AppendValues(
        {"a", "a", "a", "a", "a", "b", "b", "b", "b", "b"}));
    ARROW_RETURN_NOT_OK(string_builder.Finish(&arrays[3]));
    auto table = arrow::Table::Make(schema, arrays);
    // Write it using Datasets
    auto dataset = std::make_shared<ds::InMemoryDataset>(table);
    ARROW_ASSIGN_OR_RAISE(auto scanner_builder, dataset->NewScan());
    ARROW_ASSIGN_OR_RAISE(auto scanner, scanner_builder->Finish());

    // The partition schema determines which fields are part of the
    // partitioning.
    auto partition_schema
        = arrow::schema({arrow::field("part", arrow::utf8())});
    // We'll use Hive-style partitioning, which creates directories with
    // "key=value" pairs.
    auto partitioning
        = std::make_shared<ds::HivePartitioning>(partition_schema);
    // We'll write Parquet files.
    auto format = std::make_shared<ds::ParquetFileFormat>();
    ds::FileSystemDatasetWriteOptions write_options;
    write_options.file_write_options = format->DefaultWriteOptions();
    write_options.filesystem = filesystem;
    write_options.base_dir = base_path;
    write_options.partitioning = partitioning;
    write_options.basename_template = "part{i}.parquet";
    ARROW_RETURN_NOT_OK(ds::FileSystemDataset::Write(write_options, scanner));
    return base_path;
}

// Read the whole dataset with the given format, without partitioning.
auto ScanWholeDataset(const std::shared_ptr<fs::FileSystem> &filesystem,
                      const std::shared_ptr<ds::FileFormat> &format,
                      const std::string                     &base_dir)
    -> arrow::Result<std::shared_ptr<arrow::Table>> {
    // Create a dataset by scanning the filesystem for files
    fs::FileSelector selector;
    selector.base_dir = base_dir;
    ARROW_ASSIGN_OR_RAISE(
        auto factory,
        ds::FileSystemDatasetFactory::Make(filesystem,
                                           selector,
                                           format,
                                           ds::FileSystemFactoryOptions()));
    ARROW_ASSIGN_OR_RAISE(auto dataset, factory->Finish());
    // Print out the fragments
    ARROW_ASSIGN_OR_RAISE(auto fragments, dataset->GetFragments())
    for (const auto &fragment : fragments) {
        std::cout << "Found fragment: " << (*fragment)->ToString() << '\n';
    }
    // Read the entire dataset as a Table
    ARROW_ASSIGN_OR_RAISE(auto scan_builder, dataset->NewScan());
    ARROW_ASSIGN_OR_RAISE(auto scanner, scan_builder->Finish());
    return scanner->ToTable();
}

// Read a dataset, but select only column "b" and only rows where b < 4.
//
// This is useful when you only want a few columns from a dataset. Where
// possible, Datasets will push down the column selection such that less work is
// done.
auto FilterAndSelectDataset(const std::shared_ptr<fs::FileSystem> &filesystem,
                            const std::shared_ptr<ds::FileFormat> &format,
                            const std::string                     &base_dir)
    -> arrow::Result<std::shared_ptr<arrow::Table>> {
    fs::FileSelector selector;
    selector.base_dir = base_dir;
    ARROW_ASSIGN_OR_RAISE(
        auto factory,
        ds::FileSystemDatasetFactory::Make(filesystem,
                                           selector,
                                           format,
                                           ds::FileSystemFactoryOptions()));
    ARROW_ASSIGN_OR_RAISE(auto dataset, factory->Finish());
    // Read specified columns with a row filter
    ARROW_ASSIGN_OR_RAISE(auto scan_builder, dataset->NewScan());
    ARROW_RETURN_NOT_OK(scan_builder->Project({"b"}));
    ARROW_RETURN_NOT_OK(
        scan_builder->Filter(cp::less(cp::field_ref("b"), cp::literal(4))));
    ARROW_ASSIGN_OR_RAISE(auto scanner, scan_builder->Finish());
    return scanner->ToTable();
}

// Read a dataset, but with column projection.
//
// This is useful to derive new columns from existing data. For example, here we
// demonstrate casting a column to a different type, and turning a numeric
// column into a boolean column based on a predicate. You could also rename
// columns or perform computations involving multiple columns.
auto ProjectDataset(const std::shared_ptr<fs::FileSystem> &filesystem,
                    const std::shared_ptr<ds::FileFormat> &format,
                    const std::string                     &base_dir)
    -> arrow::Result<std::shared_ptr<arrow::Table>> {
    fs::FileSelector selector;
    selector.base_dir = base_dir;
    ARROW_ASSIGN_OR_RAISE(
        auto factory,
        ds::FileSystemDatasetFactory::Make(filesystem,
                                           selector,
                                           format,
                                           ds::FileSystemFactoryOptions()));
    ARROW_ASSIGN_OR_RAISE(auto dataset, factory->Finish());
    // Read specified columns with a row filter
    ARROW_ASSIGN_OR_RAISE(auto scan_builder, dataset->NewScan());
    ARROW_RETURN_NOT_OK(scan_builder->Project(
        {
            // Leave column "a" as-is.
            cp::field_ref("a"),
            // Cast column "b" to float32.
            cp::call("cast",
                     {cp::field_ref("b")},
                     arrow::compute::CastOptions::Safe(arrow::float32())),
            // Derive a boolean column from "c".
            cp::equal(cp::field_ref("c"), cp::literal(1)),
        },
        {"a_renamed", "b_as_float32", "c_1"}));
    ARROW_ASSIGN_OR_RAISE(auto scanner, scan_builder->Finish());
    return scanner->ToTable();
}

// Read a dataset, but with column projection.
//
// This time, we read all original columns plus one derived column. This simply
// combines the previous two examples: selecting a subset of columns by name,
// and deriving new columns with an expression.
auto SelectAndProjectDataset(const std::shared_ptr<fs::FileSystem> &filesystem,
                             const std::shared_ptr<ds::FileFormat> &format,
                             const std::string                     &base_dir)
    -> arrow::Result<std::shared_ptr<arrow::Table>> {
    fs::FileSelector selector;
    selector.base_dir = base_dir;
    ARROW_ASSIGN_OR_RAISE(
        auto factory,
        ds::FileSystemDatasetFactory::Make(filesystem,
                                           selector,
                                           format,
                                           ds::FileSystemFactoryOptions()));
    ARROW_ASSIGN_OR_RAISE(auto dataset, factory->Finish());
    // Read specified columns with a row filter
    ARROW_ASSIGN_OR_RAISE(auto scan_builder, dataset->NewScan());
    std::vector<std::string>    names;
    std::vector<cp::Expression> exprs;
    // Read all the original columns.
    for (const auto &field : dataset->schema()->fields()) {
        names.push_back(field->name());
        exprs.push_back(cp::field_ref(field->name()));
    }
    // Also derive a new column.
    names.emplace_back("b_large");
    exprs.push_back(cp::greater(cp::field_ref("b"), cp::literal(1)));
    ARROW_RETURN_NOT_OK(scan_builder->Project(exprs, names));
    ARROW_ASSIGN_OR_RAISE(auto scanner, scan_builder->Finish());
    return scanner->ToTable();
}

// Read an entire dataset, but with partitioning information.
auto ScanPartitionedDataset(const std::shared_ptr<fs::FileSystem> &filesystem,
                            const std::shared_ptr<ds::FileFormat> &format,
                            const std::string                     &base_dir)
    -> arrow::Result<std::shared_ptr<arrow::Table>> {
    fs::FileSelector selector;
    selector.base_dir = base_dir;
    selector.recursive = true; // Make sure to search subdirectories
    ds::FileSystemFactoryOptions options;
    // We'll use Hive-style partitioning. We'll let Arrow Datasets infer the
    // partition schema.
    options.partitioning = ds::HivePartitioning::MakeFactory();
    ARROW_ASSIGN_OR_RAISE(auto factory,
                          ds::FileSystemDatasetFactory::Make(filesystem,
                                                             selector,
                                                             format,
                                                             options));
    ARROW_ASSIGN_OR_RAISE(auto dataset, factory->Finish());
    // Print out the fragments
    ARROW_ASSIGN_OR_RAISE(auto fragments, dataset->GetFragments());
    for (const auto &fragment : fragments) {
        std::cout << "Found fragment: " << (*fragment)->ToString() << '\n';
        std::cout << "Partition expression: "
                  << (*fragment)->partition_expression().ToString() << '\n';
    }
    ARROW_ASSIGN_OR_RAISE(auto scan_builder, dataset->NewScan());
    ARROW_ASSIGN_OR_RAISE(auto scanner, scan_builder->Finish());
    return scanner->ToTable();
}

// Read an entire dataset, but with partitioning information. Also, filter the
// dataset on the partition values.
auto FilterPartitionedDataset(const std::shared_ptr<fs::FileSystem> &filesystem,
                              const std::shared_ptr<ds::FileFormat> &format,
                              const std::string                     &base_dir)
    -> arrow::Result<std::shared_ptr<arrow::Table>> {
    fs::FileSelector selector;
    selector.base_dir = base_dir;
    selector.recursive = true;
    ds::FileSystemFactoryOptions options;
    options.partitioning = ds::HivePartitioning::MakeFactory();
    ARROW_ASSIGN_OR_RAISE(auto factory,
                          ds::FileSystemDatasetFactory::Make(filesystem,
                                                             selector,
                                                             format,
                                                             options));
    ARROW_ASSIGN_OR_RAISE(auto dataset, factory->Finish());
    ARROW_ASSIGN_OR_RAISE(auto scan_builder, dataset->NewScan());
    // Filter based on the partition values. This will mean that we won't even
    // read the files whose partition expressions don't match the filter.
    ARROW_RETURN_NOT_OK(scan_builder->Filter(
        cp::equal(cp::field_ref("part"), cp::literal("b"))));
    ARROW_ASSIGN_OR_RAISE(auto scanner, scan_builder->Finish());
    return scanner->ToTable();
}

auto RunDatasetExample(const std::string &format_name,
                       const std::string &uri,
                       const std::string &mode) -> arrow::Status {
    std::string                     base_path;
    std::shared_ptr<ds::FileFormat> format;
    std::string                     root_path;
    ARROW_ASSIGN_OR_RAISE(auto fs, fs::FileSystemFromUri(uri, &root_path));

    if (format_name == "feather") {
        format = std::make_shared<ds::IpcFileFormat>();
        ARROW_ASSIGN_OR_RAISE(base_path,
                              CreateExampleFeatherDataset(fs, root_path));
    } else if (format_name == "parquet") {
        format = std::make_shared<ds::ParquetFileFormat>();
        ARROW_ASSIGN_OR_RAISE(base_path,
                              CreateExampleParquetDataset(fs, root_path));
    } else if (format_name == "parquet_hive") {
        format = std::make_shared<ds::ParquetFileFormat>();
        ARROW_ASSIGN_OR_RAISE(
            base_path,
            CreateExampleParquetHivePartitionedDataset(fs, root_path));
    } else {
        std::cerr << "Unknown format: " << format_name << '\n';
        std::cerr << "Supported formats: feather, parquet, parquet_hive\n";
        return arrow::Status::ExecutionError("Dataset creating failed.");
    }

    std::shared_ptr<arrow::Table> table;
    if (mode == "no_filter") {
        ARROW_ASSIGN_OR_RAISE(table, ScanWholeDataset(fs, format, base_path));
    } else if (mode == "filter") {
        ARROW_ASSIGN_OR_RAISE(table,
                              FilterAndSelectDataset(fs, format, base_path));
    } else if (mode == "project") {
        ARROW_ASSIGN_OR_RAISE(table, ProjectDataset(fs, format, base_path));
    } else if (mode == "select_project") {
        ARROW_ASSIGN_OR_RAISE(table,
                              SelectAndProjectDataset(fs, format, base_path));
    } else if (mode == "partitioned") {
        ARROW_ASSIGN_OR_RAISE(table,
                              ScanPartitionedDataset(fs, format, base_path));
    } else if (mode == "filter_partitioned") {
        ARROW_ASSIGN_OR_RAISE(table,
                              FilterPartitionedDataset(fs, format, base_path));
    } else {
        std::cerr << "Unknown mode: " << mode << '\n';
        std::cerr << "Supported modes: no_filter, filter, project, "
                     "select_project, partitioned\n";
        return arrow::Status::ExecutionError("Dataset reading failed.");
    }
    std::cout << "Read " << table->num_rows() << " rows" << '\n';
    std::cout << table->ToString() << '\n';
    return arrow::Status::OK();
}

auto main(int argc, char **argv) -> int {
    if (argc < 3) {
        std::cout << "Usage: ../build/examples/dataset_example "
                     "file:///<some_path>/<some_directory> parquet\n";
        std::cout << "Example: ../build/examples/dataset_example file://`pwd`/ "
                     "parquet\n";
        return EXIT_FAILURE;
    }

    std::string uri = argv[1];
    std::string format_name = argv[2];
    std::string mode = argc > 3 ? argv[3] : "no_filter";

    auto status = RunDatasetExample(format_name, uri, mode);
    if (!status.ok()) {
        std::cerr << status.ToString() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
