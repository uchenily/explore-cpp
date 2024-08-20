# Data Type

可以用三种方式表示数据的类型信息:

- 使用 `arrow::DataType` 实例
- 使用 `arrow::DataType` 的具体子类
- 使用 `arrow::type::type` 枚举值

第一种方式最常用, 最灵活.

另外的两种可以用于性能要求高的地方, 避免付出动态类型和多态性的代价.
> 但是，对于参数类型，仍然需要进行一些运行时切换。不可能在编译时具体化所有可能的类型，因为Arrow数据类型允许任意嵌套。


### 创建数据类型

建议调用[工厂函数](https://arrow.apache.org/docs/cpp/api/datatype.html#api-type-factories)

```cpp
    std::shared_ptr<arrow::DataType> type;

    // A 16-bit integer type
    type = arrow::int16();
    // A 64-bit timestamp type (with microsecond granularity)
    type = arrow::timestamp(arrow::TimeUnit::MICRO);
    // A list type of single-precision floating-point values
    type = arrow::list(arrow::float32());
```


### Visitor Pattern

为了处理arrow::DataType、arrow::Scalar或arrow::Array，可能需要编写根据特定Arrow类型进行专门化的逻辑.

Arrow 提供了这些模板函数:

- `arrow::VisitTypeInline()`
- `arrow::VisitScalarInline()`
- `arrow::VisitArrayInline()`

需要为每一个专用类型实现一个`auto Visit() -> Status` 方法.

一个队任意数值类型列求和的示例:

```cpp
class TableSummation {
    double partial = 0.0;

public:
    auto Compute(std::shared_ptr<arrow::RecordBatch> batch)
        -> arrow::Result<double> {
        for (const std::shared_ptr<arrow::Array> &array : batch->columns()) {
            ARROW_RETURN_NOT_OK(arrow::VisitArrayInline(*array, this));
        }
        return partial;
    }

    // Default implementation
    auto Visit(const arrow::Array &array) -> arrow::Status {
        return arrow::Status::NotImplemented(
            "Cannot compute sum for array of type ",
            array.type()->ToString());
    }

    template <typename ArrayType, typename T = typename ArrayType::TypeClass>
    auto Visit(const ArrayType &array)
        -> arrow::enable_if_number<T, arrow::Status> {
        for (std::optional<typename T::c_type> value : array) {
            if (value.has_value()) {
                partial += static_cast<double>(value.value());
            }
        }
        return arrow::Status::OK();
    }
};
```


Arrow还提供了抽象访问者类（arrow::TypeVisitor、arrow::ScalarVisitor、arrow::ArrayVisitor）和每个相应基类型上的Accept() 方法（例如arrow::Array::Accept()）。但是，这些都不能使用模板函数实现，所以通常还是使用内联类型的访问者
