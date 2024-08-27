# High-Level Overview

### 物理层

Memory management 抽象为内存提供了 统一的 API. Buffer 抽象表示为物理数据的连续区域.

### one-dimensional layer

数据类型控制物理数据的逻辑解释.

Array 将一个或者多个具有数据类型的缓冲区组合在一起, 允许将他们视为值的逻辑连续序列.

Chunked Array (分块数组) 是数组的泛化, 将几个相同类型的数组组成一个更长的值逻辑序列.


### two-dimensional layer

Schema 描述多个数据的逻辑集合, 每一个数据都有不同的名称和类型, 以及可选的元数据.

Table 是符合 Schema 的分块数组的集合.

Record batch 是由 schema 描述的连续数组的集合, 它可以顺序 table 的增量构造或者序列化.


### 计算层

Datum 是数据集引用, 能够持有一个数组或者表引用.

Kernel 是一组给定Datums上循环运行的专用计算函数, 这些Datum表示函数的输入和输出参数.

Acero (pronounced [aˈsɜɹo] / ah-SERR-oh) 是一种流失执行引擎, 允许将计算表示为可以转换为数据流的运算符图.


### IO层

Stream 顺序对各种外部数据进行无类型无类型、顺序或可查找的访问

### IPC 层

messaging format 允许在进行 之间交换 Arrow 数据, 使用尽可能少的副本.

### file format 层

可以从各种文件个独读取和输入 Arrow 数据, 例如 Parquet、CSV、Orc 或特定于 Arrow 的 Feather 格式


### 设备层

提供了基本的 CUDA 集成，允许描述由 GPU 分配的内存支持的 Arrow 数据

### 文件系统层

文件系统抽象允许从不同的存储后端（例如本地文件系统或 S3 存储桶）读取和写入数据
