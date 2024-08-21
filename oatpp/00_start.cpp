#include "oatpp/network/Server.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/web/server/HttpConnectionHandler.hpp"

void run() {
    auto router = oatpp::web::server::HttpRouter::createShared();
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
