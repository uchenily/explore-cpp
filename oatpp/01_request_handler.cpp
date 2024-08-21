#include "oatpp/network/Server.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/web/server/HttpConnectionHandler.hpp"

// 自定义 handler
class Handler : public oatpp::web::server::HttpRequestHandler {
public:
    std::shared_ptr<OutgoingResponse>
    handle(const std::shared_ptr<IncomingRequest> &request) override {
        return ResponseFactory::createResponse(Status::CODE_200,
                                               "hello World!");
    }
};

void run() {
    auto router = oatpp::web::server::HttpRouter::createShared();
    // 添加 handler
    router->route("GET", "/hello", std::make_shared<Handler>());
    auto connectionHandler
        = oatpp::web::server::HttpConnectionHandler::createShared(router);
    auto connectionProvider
        = oatpp::network::tcp::server::ConnectionProvider::createShared(
            {"localhost", 8000, oatpp::network::Address ::IP_4});
    oatpp::network::Server server(connectionProvider, connectionHandler);

    OATPP_LOGI("MyAPP",
               "server running on port %s",
               connectionProvider->getProperty("port").getData());

    server.run();
}

auto main() -> int {
    oatpp::base::Environment::init();
    run();
    oatpp::base::Environment::destroy();
}

// client:
// curl http://localhost:8000/hello
