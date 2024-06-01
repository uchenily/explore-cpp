#include "asio.hpp"
#include "debug.hpp"

#include <ctime>
#include <string>

using namespace asio::ip;

auto make_daytime() -> std::string {
    time_t now = std::time(nullptr);
    return {ctime(&now)};
};

auto main() -> int {
    asio::io_context io_context;
    udp::socket      socket{
        io_context,
        udp::endpoint{udp::v4(), 13}  // daytime port 13 /etc/services
    };

    while (true) {
        std::array<char, 1> recv_buf;
        udp::endpoint       remote_endpoint;
        std::error_code     ec;

        socket.receive_from(asio::buffer(recv_buf), remote_endpoint);
        LOG_INFO("Received message from {}:{}",
                 remote_endpoint.address().to_string(),
                 remote_endpoint.port());
        socket.send_to(asio::buffer(make_daytime()), remote_endpoint, 0, ec);
    }
}
