#include "arrow/api.h"

#include <iostream>

auto RunMain() -> arrow::Status {
    arrow::Int8Builder int8builder;
    int8_t             days_raw[5] = {1, 12, 17, 23, 28};
    ARROW_RETURN_NOT_OK(int8builder.AppendValues(days_raw, 5));

    // arrow::Array <- []int8_t
    std::shared_ptr<arrow::Array> days;
    ARROW_ASSIGN_OR_RAISE(days, int8builder.Finish());

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
