cmake_minimum_required(VERSION 3.20)
project(echo CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# 指定 vcpkg 的工具链文件
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
set(CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-linux" CACHE STRING "")

# 查找依赖库
find_package(Thrift CONFIG REQUIRED)

set(THRIFT_IDL_FILE ${CMAKE_SOURCE_DIR}/example.thrift)
set(GEN_CPP_DIR ${CMAKE_BINARY_DIR}/gen-cpp)
file(MAKE_DIRECTORY ${GEN_CPP_DIR})
add_custom_command(
    OUTPUT ${GEN_CPP_DIR}/CalcService.h ${GEN_CPP_DIR}/CalcService.cpp
    COMMAND $ENV{VCPKG_ROOT}/installed/x64-linux/tools/thrift/thrift --gen cpp -out ${GEN_CPP_DIR} ${THRIFT_IDL_FILE}
    DEPENDS ${THRIFT_IDL_FILE}
    COMMENT "Generating C++ code from Thrift IDL"
)
# 添加头文件搜索路径
include_directories(${GEN_CPP_DIR})

# 添加可执行文件
add_executable(server server.cpp gen-cpp/CalcService.cpp)
add_executable(client client.cpp gen-cpp/CalcService.cpp)

# 链接库
target_link_libraries(server thrift::thrift)
target_link_libraries(client thrift::thrift)
