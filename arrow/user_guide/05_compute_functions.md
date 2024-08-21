# Compute Function

### 通用计算 API

函数存储在全局 `FunctionRegistry` 变量中, 可以按名称查找.

计算的输入使用通用的`Datum` 类表示. 这个类是多个数据形状(如 Scalar, Array, ChunkedArray) 的并集.

可以使用 `arrow::compute::CallFunction()` 按名称调用计算函数.

```cpp
    std::shared_ptr<arrow::Array>  numbers = ...;
    std::shared_ptr<arrow::Scalar> increment = ...;
    arrow::Datum                   incremented_datum;

    ARROW_ASSIGN_OR_RAISE(
        incremented_datum,
        arrow::compute::CallFunction("add", {numbers, increment}));
    std::shared_ptr<arrow::Array> incremented_array
        = std::move(incremented_datum).make_array();
```

许多计算函数也可以直接作为具体的 API 使用, 比如这里的 `arrow::compute::Add()`:

```cpp
    std::shared_ptr<arrow::Array>  numbers = ...;
    std::shared_ptr<arrow::Scalar> increment = ...;
    arrow::Datum                   incremented_datum;

    ARROW_ASSIGN_OR_RAISE(
        incremented_datum,
        arrow::compute::Add(numbers, increment)); // 这一行
    std::shared_ptr<arrow::Array> incremented_array
        = std::move(incremented_datum).make_array();
```

一些函数需要确定函数确切语义的 options 结构:

```cpp
    ScalarAggregateOptions options;
    options.skip_nulls = false;

    std::shared_ptr<arrow::Array> array = ...;
    arrow::Datum                  min_max;
    ARROW_ASSIGN_OR_RAISE(
        min_max,
        arrow::compute::CallFunction("min_max", {array}, &options));

    // unpack result
    std::shared_ptr<arrow::Scalar> min_value;
    std::shared_ptr<arrow::Scalar> max_value;
    min_value = min_max.scalar_as<arrow::StructScalar>().value[0];
    max_value = min_max.scalar_as<arrow::StructScalar>().value[1];
```

但需要注意, Grouped Aggregations 不能通过 `CallFunction` 调用.


#### 类型分类:
- Numeric
- Temporal: Date类型(Date32, Date64), Time类型(Time32, Time64), Timestamp, Duration, Interval
- Binary-like
- String-like
- List-like
- Nested


#### 聚合:
- all
- any
- approximate_median
- count
- count_all
- count_distinct
- first
- first_last
- index
- last
- max
- mean
- min
- min_max
- mode
- product
- quantile
- stddev
- sum
- tdigest
- variance (方差)

#### 分组聚合(group by)(分组依据)

分组组合不能直接调用, 而是用作 SQL-style 的"group by"操作的一部分.

支持的聚合函数如下. (所有函数名称都以 `hash_` 为前缀, 与标量形式分开)

- hash_all
- hash_any
- hash_approximate_median
- hash_count
- hash_count_all
- hash_distinct
- hash_first
- hash_first_last
- hash_last
- hash_list
- hash_max
- hash_mean
- hash_min
- hash_min_max
- hash_one
- hash_product
- hash_stddev
- hash_sum
- hash_tdigest
- hash_variance

#### 标量函数(Element-wise, "scalar")

TODO

#### 算术函数

默认变体不会检测溢出(结果通常会wraps around). 大多数函数也有一个用于溢出检查的变体. 后缀为`_checked`, 当检测到溢出时, 变体返回Invalid Status.

- abs
- add
- divide
- exp
- multiply
- negate
- power
- sign
- sqrt
- subtract

#### 比特函数(Bit-wise)

- bit_wise_and
- bit_wise_not
- bit_wise_or
- bit_wise_xor
- shift_left
- shift_left_checked
- shift_right
- shift_right_checked

#### Round函数

- ceil
- floor
- round
- round_to_multiple
- round_binary
- trunc

#### Log函数

- ln
- ln_checked
- log10
- log10_checked
- log1p
- log1p_checked
- log2
- log2_check
- logb
- logb_checked

#### 三角函数

#### 比较函数

- equal
- greater
- greater_equal
- less
- less_equal
- not_equal

#### 逻辑函数

- and
- and_kleene
- and_not
- and_not_kleene
- invert
- or
- or_kleene
- xor

#### 字符串谓词(string predicates)
第一组
- ascii_is_alnum
- ascii_is_alpha
- ascii_is_decimal
- ascii_is_lower
- ascii_is_printable
- ascii_is_space
- ascii_is_upper
- utf8_is_alnum
- utf8_is_alpha
- utf8_is_decimal
- utf8_is_digit
- utf8_is_lower
- utf8_is_numeric
- utf8_is_printable
- utf8_is_space
- utf8_is_upper

第二组
- ascii_is_title
- utf8_is_title

第三组
- string_is_ascii

#### 字符串转换

#### 字符串填充

#### 字符串修剪

#### 字符串拆分

#### 字符串组件提取

#### 字符串 join

#### 字符串切片

### Containment tests

#### 分类

#### selecting/multiplexing

#### 结构转换

#### 转换

#### Temporal component extraction(时间分量提取)

### 时间差

### 时区处理

#### 随机数生成

### 向量函数(Array-wise, "vector")

累积函数

关联变换

选择

Containment tests

排序和分区

结构转换

成对函数(Pairwise function)
