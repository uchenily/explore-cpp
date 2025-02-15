cmake_minimum_required(VERSION 3.20)
project(echo CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# 指定 vcpkg 的工具链文件
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
set(CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-linux" CACHE STRING "")

# gflags 需要
set(GFLAGS_USE_TARGET_NAMESPACE ON)

# 查找依赖库
find_package(unofficial-brpc CONFIG REQUIRED)

# protoc编译proto文件
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER echo.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# 添加可执行文件
add_executable(echo_client client.cpp ${PROTO_SRC} ${PROTO_HEADER})
add_executable(echo_server server.cpp ${PROTO_SRC} ${PROTO_HEADER})

# 链接库
target_link_libraries(echo_server unofficial::brpc::brpc-static)
target_link_libraries(echo_client unofficial::brpc::brpc-static)
