#include "arrow/api.h"
#include "arrow/csv/api.h"

#include "arrow/io/file.h"
#include "arrow/ipc/reader.h"
#include "arrow/ipc/writer.h"

#include <iostream>

auto GenInitialFile() -> arrow::Status {
    arrow::Int8Builder int8builder;
    int8_t             days_raw[5] = {1, 12, 17, 23, 28};
    ARROW_RETURN_NOT_OK(int8builder.AppendValues(days_raw, 5));
    std::shared_ptr<arrow::Array> days;
    ARROW_ASSIGN_OR_RAISE(days, int8builder.Finish());

    int8_t months_raw[5] = {1, 3, 5, 7, 1};
    ARROW_RETURN_NOT_OK(int8builder.AppendValues(months_raw, 5));
    std::shared_ptr<arrow::Array> months;
    ARROW_ASSIGN_OR_RAISE(months, int8builder.Finish());

    arrow::Int16Builder int16builder;
    int16_t             years_raw[5] = {1990, 2000, 1995, 2000, 1995};
    ARROW_RETURN_NOT_OK(int16builder.AppendValues(years_raw, 5));
    std::shared_ptr<arrow::Array> years;
    ARROW_ASSIGN_OR_RAISE(years, int16builder.Finish());

    std::vector<std::shared_ptr<arrow::Array>> columns = {days, months, years};

    // 创建一个schema
    std::shared_ptr<arrow::Field>  field_day;
    std::shared_ptr<arrow::Field>  field_month;
    std::shared_ptr<arrow::Field>  field_year;
    std::shared_ptr<arrow::Schema> schema;

    field_day = arrow::field("Day", arrow::int8());
    field_month = arrow::field("Month", arrow::int8());
    field_year = arrow::field("Year", arrow::int16());

    schema = arrow::schema({field_day, field_month, field_year});

    // 现在根据schema创建表
    std::shared_ptr<arrow::Table> table;
    table = arrow::Table::Make(schema, columns);

    // 写入到文件
    std::shared_ptr<arrow::io::FileOutputStream> outfile;
    ARROW_ASSIGN_OR_RAISE(outfile,
                          arrow::io::FileOutputStream::Open("test_in.arrow"));
    ARROW_ASSIGN_OR_RAISE(
        std::shared_ptr<arrow::ipc::RecordBatchWriter> ipc_writer,
        arrow::ipc::MakeFileWriter(outfile, schema));

    ARROW_RETURN_NOT_OK(ipc_writer->WriteTable(*table));
    ARROW_RETURN_NOT_OK(ipc_writer->Close());

    ARROW_ASSIGN_OR_RAISE(outfile,
                          arrow::io::FileOutputStream::Open("test_in.csv"));
    ARROW_ASSIGN_OR_RAISE(auto csv_writer,
                          arrow::csv::MakeCSVWriter(outfile, table->schema()));
    ARROW_RETURN_NOT_OK(csv_writer->WriteTable(*table));
    ARROW_RETURN_NOT_OK(csv_writer->Close());

    return arrow::Status::OK();
}

auto RunMain() -> arrow::Status {
    ARROW_RETURN_NOT_OK(GenInitialFile());

    // 打开文件
    std::shared_ptr<arrow::io::ReadableFile> infile;
    ARROW_ASSIGN_OR_RAISE(
        infile,
        arrow::io::ReadableFile::Open("test_in.arrow",
                                      arrow::default_memory_pool()));

    // 创建Arrow file Reader
    ARROW_ASSIGN_OR_RAISE(auto ipc_reader,
                          arrow::ipc::RecordBatchFileReader::Open(infile));

    // 读取Arrow文件, 转成RecordBatch
    std::shared_ptr<arrow::RecordBatch> rbatch;
    ARROW_ASSIGN_OR_RAISE(rbatch, ipc_reader->ReadRecordBatch(0));

    // 准备一个FileOutputStream
    std::shared_ptr<arrow::io::FileOutputStream> outstream;
    ARROW_ASSIGN_OR_RAISE(outstream,
                          arrow::io::FileOutputStream::Open("test_out.arrow"));

    // 将RecordBatch写入到文件中
    ARROW_ASSIGN_OR_RAISE(
        std::shared_ptr<arrow::ipc::RecordBatchWriter> ipc_writer,
        arrow::ipc::MakeFileWriter(outstream, rbatch->schema()));
    ARROW_RETURN_NOT_OK(ipc_writer->WriteRecordBatch(*rbatch));
    ARROW_RETURN_NOT_OK(ipc_writer->Close());

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
