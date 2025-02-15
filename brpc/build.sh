#!/usr/bin/env bash

# CMAKE_PREFIX_PATH="/opt/vcpkg/installed/x64-linux/"
# $CMAKE_PREFIX_PATH/tools/protobuf/protoc --cpp_out=. echo.proto

cmake -B build
cmake --build build
