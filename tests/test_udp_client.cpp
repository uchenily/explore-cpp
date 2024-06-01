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
    udp::resolver    resolver{io_context};
    auto             receiver_endpoint = *resolver.resolve(argv[1], "daytime")
                                  .begin(); // /etc/services  port 13
    udp::socket socket{io_context};
    socket.open(udp::v4());

    std::array<char, 1>   send_buf{};
    std::array<char, 128> recv_buf;
    udp::endpoint         sender_endpoint;

    socket.send_to(asio::buffer(send_buf), receiver_endpoint);
    auto len = socket.receive_from(asio::buffer(recv_buf), sender_endpoint);

    LOG_INFO("{}", std::string_view{recv_buf.data(), len});
}
