#pragma once

#include "DTO/message.hpp"

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

#include OATPP_CODEGEN_BEGIN(ApiController)
class MyController : public oatpp::web::server::api::ApiController {
public:
    MyController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper))
        : oatpp::web::server::api::ApiController(objectMapper) {}

public:
    ENDPOINT("GET", "/hello", root) {
        auto dto = MessageDTO::createShared();
        dto->statusCode = 200;
        dto->message = "hello world!";
        return createDtoResponse(Status::CODE_200, dto);
    }
};
#include OATPP_CODEGEN_END(ApiController)
