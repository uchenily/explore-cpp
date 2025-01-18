cmake -GNinja -Bbuild -DCMAKE_EXPORT_COMPILE_COMMANDS=ON . -DCMAKE_INSTALL_PREFIX=/opt/cmake-install/ -DCMAKE_PREFIX_PATH="/root/work/build-arrow/cpp/vcpkg_installed/x64-linux;/opt/cmake-install" -DCMAKE_BUILD_TYPE=Debug


cmake --build build
