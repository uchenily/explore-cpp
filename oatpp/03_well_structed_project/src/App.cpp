#include "AppComponent.hpp"
#include "controller/MyController.hpp"

#include "oatpp/network/Server.hpp"

void run() {
    // 注册组件
    AppComponent components;

    // 获取 router 组件
    OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);

    // 创建 MyController 且将 endpoints 添加到 router
    auto myController = std::make_shared<MyController>();
    router->addController(myController);

    // 获取 connection handler 组件
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>,
                    connectionHandler);

    // 获取 connection provider 组件
    OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>,
                    connectionProvider);

    // 创建 Server
    oatpp::network::Server server(connectionProvider, connectionHandler);

    OATPP_LOGI("MyAPP",
               "Server running on port %s",
               connectionProvider->getProperty("port").getData());

    server.run();
}

auto main() -> int {
    oatpp::base::Environment::init();
    run();
    oatpp::base::Environment::destroy();
}
