#include "asio.hpp"
#include "debug.hpp"
#include <ctime>

using namespace asio::ip;

class udp_server {
public:
    udp_server(asio::io_context &io_context)
        : socket_{
            io_context,
            udp::endpoint{udp::v4(), 13}
    } {
        start_receive();
    }

private:
    void start_receive() {
        socket_.async_receive_from(asio::buffer(recv_buffer_),
                                   remote_endpoint_,
                                   [this](auto ec, std::size_t nread) {
                                       this->handle_receive(ec, nread);
                                   });
    }

    void handle_receive(const std::error_code &ec, std::size_t nreceived) {
        LOG_INFO("error_code={} received {} bytes", ec.message(), nreceived);
        if (!ec) {
            socket_.async_send_to(asio::buffer(make_daytime()),
                                  remote_endpoint_,
                                  [this](auto ec, std::size_t nsent) {
                                      this->handle_send(ec, nsent);
                                  });
        }
        start_receive();
    }

    void handle_send(const std::error_code &ec, std::size_t nsent) {
        LOG_INFO("error_code={} sent {} bytes", ec.message(), nsent);
    }

    auto make_daytime() -> std::string {
        time_t now = std::time(nullptr);
        return {ctime(&now)};
    };

private:
    std::array<char, 1> recv_buffer_;
    udp::socket         socket_;
    udp::endpoint       remote_endpoint_;
};

auto main() -> int {
    asio::io_context io_context;

    udp_server server{io_context};
    io_context.run();
}
