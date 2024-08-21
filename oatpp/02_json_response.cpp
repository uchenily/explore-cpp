#include "oatpp/network/Server.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp"
#include "oatpp/web/server/HttpConnectionHandler.hpp"

#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)
// Messsage Data-Transfer-Object
class MessageDTO : public oatpp::DTO {
    DTO_INIT(MessageDTO, DTO)

    DTO_FIELD(Int32, statusCode); // status code 字段
    DTO_FIELD(String, message);   // message 字段
};
#include OATPP_CODEGEN_END(DTO)

// 自定义 handler
class Handler : public oatpp::web::server::HttpRequestHandler {
public:
    Handler(const std::shared_ptr<oatpp::data::mapping::ObjectMapper>
                &object_mapper)
        : object_mapper_(object_mapper) {}

public:
    std::shared_ptr<OutgoingResponse>
    handle(const std::shared_ptr<IncomingRequest> &request) override {
        auto message = MessageDTO::createShared();
        message->statusCode = 1024;
        message->message = "hello DTO!";
        return ResponseFactory::createResponse(Status::CODE_200,
                                               message,
                                               object_mapper_);
    }

private:
    std::shared_ptr<oatpp::data::mapping::ObjectMapper> object_mapper_;
};

void run() {
    // 创建 json object mapper
    auto objectMapper
        = oatpp::parser::json::mapping::ObjectMapper::createShared();
    auto router = oatpp::web::server::HttpRouter::createShared();
    // 添加 handler
    router->route("GET", "/hello", std::make_shared<Handler>(objectMapper));
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
