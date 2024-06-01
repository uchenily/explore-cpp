#include "asio.hpp"
#include "debug.hpp"
#include <ctime>
#include <memory>

using namespace asio::ip;

// class tcp_connection : public std::enable_shared_from_this<tcp_connection> {
class tcp_connection {
public:
    using TcpConnectionPtr = std::shared_ptr<tcp_connection>;

    tcp_connection(asio::io_context &io_context)
        : socket_{io_context} {}

    static auto create_connection(asio::io_context &io_context)
        -> TcpConnectionPtr {
        return TcpConnectionPtr{std::make_shared<tcp_connection>(io_context)};
    }

    auto socket() -> tcp::socket & {
        return socket_;
    }

    void start() {
        asio::async_write(socket_,
                          asio::buffer(make_daytime()),
                          // 为了解决当前对象销毁, 但是还是有回调函数执行的情况?
                          // [shared = shared_from_this()](asio::error_code ec,
                          //                               size_t nwritten) {
                          //     shared->handle_write(ec, nwritten);

                          [this](asio::error_code ec, size_t nwritten) {
                              this->handle_write(ec, nwritten);
                          });
    }

private:
    void handle_write(std::error_code &ec, size_t nwritten) {
        LOG_INFO("[TCP] error_code={} written {} bytes",
                 ec.message(),
                 nwritten);
    }

    auto make_daytime() -> std::string {
        time_t now = std::time(nullptr);
        return {ctime(&now)};
    };

private:
    tcp::socket socket_;
};

class tcp_server {
public:
    tcp_server(asio::io_context &io_context)
        : io_context_{io_context}
        , acceptor_{io_context, tcp::endpoint{tcp::v4(), 13}} {
        start_accept();
    }

private:
    void start_accept() {
        tcp_connection::TcpConnectionPtr new_connection
            = tcp_connection::create_connection(io_context_);

        acceptor_.async_accept(new_connection->socket(),
                               [this, new_connection](auto ec) {
                                   this->handle_accept(new_connection, ec);
                               });
    }

    void handle_accept(const tcp_connection::TcpConnectionPtr &new_connection,
                       const std::error_code                  &ec) {
        if (!ec) {
            new_connection->start();
        }
        start_accept();
    }

private:
    asio::io_context &io_context_;
    tcp::acceptor     acceptor_;
};

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
        LOG_INFO("[UDP] error_code={} received {} bytes",
                 ec.message(),
                 nreceived);
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
        LOG_INFO("[UDP] error_code={} sent {} bytes", ec.message(), nsent);
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

    tcp_server server1{io_context};
    udp_server server2{io_context};
    io_context.run();
}
