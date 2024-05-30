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
    tcp::acceptor    acceptor{
        io_context,
        tcp::endpoint{tcp::v4(), 13}  // daytime port 13 /etc/services
    };

    while (true) {
        tcp::socket socket{io_context};
        acceptor.accept(socket);

        LOG_INFO("New connection");
        std::error_code ec;
        asio::write(socket, asio::buffer(make_daytime()), ec);
    }
}
