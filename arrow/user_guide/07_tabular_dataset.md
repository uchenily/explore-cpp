# Tabular Dataset

`arrow::dataset` 提供的功能可以有效处理表格 数据集(可能大于内存)和多文件数据集. 包括功能:
- 统一的接口 (支持不同的源, 不同的文件格式, 不同的文件系统)
- 发现源 (抓取目录, 使用各种分区方案处理分区数据集, 基本的 schema 规范化等)
- 通过谓词下推(过滤行), 投影(选择和派生列) 以及可选的并行读取来优化读取.

目前支持的文件格式包括 Parquet, Feather/Arrow IPC, CSV, ORC
> ORC数据集目前只能读取, 不能写入.


### 读取数据集


### 过滤数据

对一些格式(如 Parquet), 可以通过仅从文件系统中读取指定的列来降低 IO 的成本.

一些格式可以使用`arrow::dataset::ScannerBuilder::Filter()` 筛选器来降低 IO成本.

```cpp
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
```


### 投影列(projecting columns)


```cpp
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
```

从dataset schema构建表达式:

```cpp
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
```


### 读取和写入分区数据

写入示例:

```cpp
// Set up a dataset by writing files with partitioning
auto CreateExampleParquetHivePartitionedDataset(
    const std::shared_ptr<fs::FileSystem> &filesystem,
    const std::string                     &root_path) -> arrow::Result<std::string> {
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
    ARROW_RETURN_NOT_OK(builder.Finish(&arrays[0]));
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
```

读取示例:

```cpp
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
```


### 从其他数据源读取

如果内存中已经想与 Dataset API一起使用的数据, 可以将其包装在 `arrow::dataset::InMemoryDataset` 中:

```cpp
auto table = arrow::Table::FromRecordBatches(...);
auto dataset = std::make_shared<arrow::dataset::InMemoryDataset>(std::move(table));
// Scan the dataset, filter, it, etc.
auto scanner_builder = dataset->NewScan();
```


### 事务与 ACID 保证的说明

Dataset API不提供任何事务支持或者任何 ACID 保证. 这会影响同时读写的情况, 但是并发读没有问题.
