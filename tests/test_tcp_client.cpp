#include <array>
#include <asio.hpp>
#include <debug.hpp>

using namespace asio::ip;

auto main(int argc, char *argv[]) -> int {
    if (argc != 2) {
        PRINT("Usage: client <host>");
        return 1;
    }

    asio::io_context io_context;
    tcp::resolver    resolver{io_context};
    // auto endpoints = resolver.resolve(argv[1], "echo"); // /etc/services port
    // 7
    auto endpoints
        = resolver.resolve(argv[1], "daytime"); // /etc/services  port 13
    tcp::socket socket{io_context};
    asio::connect(socket, endpoints);

    while (true) {
        std::array<char, 128> buf;
        std::error_code       ec;

        socket.write_some(asio::buffer("hello"), ec);

        auto len = socket.read_some(asio::buffer(buf), ec);
        if (ec == asio::error::eof) {
            LOG_INFO("Connection closed cleanly by peer");
            return 0;
        } else if (ec) {
            LOG_ERROR("Error: {}", ec.message());
            return 2;
        }

        LOG_INFO("{}", std::string_view{buf.data(), len});
    }
}
