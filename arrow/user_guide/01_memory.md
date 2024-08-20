# Memory Management

### Buffer

当构建一段数据时，你会持有一个可变的缓冲区，然后它将被冻结为一个不可变的容器，如`arrow::Array`


Buffer 使用size（）和data（）访问器（或mutable_data（）用于对可变缓冲区的可写访问）提供对底层内存的快速访问


```cpp
    arrow::BufferBuilder builder;
    builder.Resize(11);
    builder.Append("hello", 6);
    builder.Append("world", 5);

    auto maybe_buffer = builder.Finish();
    if (!maybe_buffer.ok()) {
        std::cout << "allocation error\n";
    }
    std::shared_ptr<arrow::Buffer> buffer = *maybe_buffer;
```

可以用 `TypedBufferBuilder` 指定宽度类型

```cpp
    arrow::TypedBufferBuilder<int32_t> builder;
    builder.Reserve(2);
    builder.Append(0x12345678);
    builder.Append(-0x76543210);

    auto maybe_buffer = builder.Finish();
    if (!maybe_buffer.ok()) {
        std::cout << "allocation error\n";
    }
    std::shared_ptr<arrow::Buffer> buffer = *maybe_buffer;
```


### Memory Pool

Memory Pool 用于大型长期数据(large long-lived data), 比如 array buffers.
一些小的C++对象和临时对象, 通常通过常规的C++分配器.


### Device

可以通过调用 `arrow::Buffer::View()` 或者 `arrow::Buffer:ViewOrCopy()` 以通用的方式查看给定设备上的Buffer.
如果原设备和目标设备相同, 将不会有任何操作.
如果不同, 当阅读Buffer的内容时, 实际的设备到设备传输可能出现延迟.


### Memory Profiling

TODO
