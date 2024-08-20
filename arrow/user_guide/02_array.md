# Array

Array表示所有具有相同类型的值的已知长度序列.

在内部, 这些值是由一个或者多个buffer表示的.
这些buffer由数据本身加上可选的bitmap buffer组成.(bitmap buffer指示数组哪些条目是空置, 如果已知数组没有空值, 则可以完全省略bitmap buffer)


### Building an array

Arrow对象是不可变的, 不能直接像 `std::vector` 一样直接填充.

有两种策略:
- 如果数据已经存在于内存中，并且布局正确，则可以将所述内存包装在arrow::Buffer实例中，然后构造一个描述数组的arrow::ArrayData
- arrow::ArrayBuilder基类和它的具体子类有助于增量地构建数组数据，而不必自己处理Arrow格式的细节

```cpp
    arrow::Int64Builder builder;
    builder.Append(1);
    builder.Append(2);
    builder.Append(3);
    builder.AppendNull(); // 空值
    builder.Append(5);
    builder.Append(6);
    builder.Append(7);
    builder.Append(8);

    auto maybe_array = builder.Finish();
    if (!maybe_array.ok()) {
        // ... do something on array building failure
    }
    std::shared_ptr<arrow::Array> array = *maybe_array;
    // 如果想访问具体的值, 可以转换为具体的子类: arrow::Int64Array

```

第一个buffer保存null bitmap. 格式是这样的: `1|1|1|1|0|1|1|1`. 由于第四个条目为空，因此缓冲区中该位置的值未定义。

```cpp
    // 转换成实际的类型
    auto int64_array = std::static_pointer_cast<arrow::Int64Array>(array);

    // 获取指向null bitmap的指针
    const uint8_t *null_bitmap = int64_array->null_bitmap_data();

    // 获取指向实际数据的指针
    const int64_t *data = int64_array->raw_values();

    // 或者，给定一个数组索引，直接查询它的null bit和值
    int64_t index = 2;
    if (!int64_array->IsNull(index)) {
        int64_t value = int64_array->Value(index);
    }

```


### 性能问题

虽然可以像上面的例子那样逐个值地构建数组，但为了获得最高的性能，建议在具体的`arrow::ArrayBuilder` 子类中使用批量追加方法（AppendValues）

如果事先知道元素的数量，还建议通过调用Resize() 或Reserve() 方法来预调整工作区域的大小

```cpp
    arrow::Int64Builder builder;
    // Make place for 8 values in total
    builder.Reserve(8);
    // Bulk append the given values (with a null in 4th place as indicated by
    // the validity vector)
    std::vector<bool> validity
        = {true, true, true, false, true, true, true, true};
    std::vector<int64_t> values = {1, 2, 3, 0, 5, 6, 7, 8};
    builder.AppendValues(values, validity);

    auto maybe_array = builder.Finish();
```


如果必须一个接一个地append值，一些具体的构建器子类有标记为“不安全”的方法，这些方法假设工作区已经正确调整大小，并提供更高的性能作为交换:

```cpp
arrow::Int64Builder builder;
// Make place for 8 values in total
builder.Reserve(8);
builder.UnsafeAppend(1);
builder.UnsafeAppend(2);
builder.UnsafeAppend(3);
builder.UnsafeAppendNull();
builder.UnsafeAppend(5);
builder.UnsafeAppend(6);
builder.UnsafeAppend(7);
builder.UnsafeAppend(8);

auto maybe_array = builder.Finish();
```


### Size Limitations and Recommendations

某些数组类型在结构上限制为32位大小。列表数组（最多可以容纳2^31个元素）、字符串数组和二进制数组（最多可以容纳2GB的二进制数据）都是如此。其他一些数组类型在C++实现中最多可以容纳2^63个元素，但其他Arrow实现也可以对这些数组类型进行32位大小限制。

出于这些原因，建议将大型数据分块到更合理大小的子集中


### Chunked Array

与Array一样，是值的逻辑序列; 但与简单数组不同，**分块数组不要求整个序列在内存中物理上连续**。此外，分块数组的组成部分不需要具有相同的大小，但它们必须具有相同的数据类型。(? 变长类型)


Chunked Array通过任意数量的数组构造

```cpp
    std::vector<std::shared_ptr<arrow::Array>> chunks;
    std::shared_ptr<arrow::Array>              array;

    // Build first chunk
    arrow::Int64Builder builder;
    builder.Append(1);
    builder.Append(2);
    builder.Append(3);
    if (!builder.Finish(&array).ok()) {
        // ... do something on array building failure
    }
    chunks.push_back(std::move(array));

    // Build second chunk
    builder.Reset();
    builder.AppendNull();
    builder.Append(5);
    builder.Append(6);
    builder.Append(7);
    builder.Append(8);
    if (!builder.Finish(&array).ok()) {
        // ... do something on array building failure
    }
    chunks.push_back(std::move(array));

    auto chunked_array
        = std::make_shared<arrow::ChunkedArray>(std::move(chunks));

    assert(chunked_array->num_chunks() == 2);
    // Logical length in number of values
    assert(chunked_array->length() == 8);
    assert(chunked_array->null_count() == 1);

```


### Slicing

与Buffer类型, 可以通过制作数组和分块数组的零拷贝片，以获得引用数据的某些逻辑子序列的数组或分块数组
调用`arow::Array::Slice()`,  `arrow::ChunkedArray::Slice()` 实现
