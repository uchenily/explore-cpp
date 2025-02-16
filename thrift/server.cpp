#include "CalcService.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TServer.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransport.h>

#include <memory>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

namespace example {
class CalcServiceImpl : public CalcServiceIf {
public:
    auto add(const int32_t num1, const int32_t num2) -> int32_t override {
        return num1 + num2;
    }
};
} // namespace example

auto main() -> int {
    int port = 9090;

    auto handler = std::make_shared<example::CalcServiceImpl>();
    auto processor = std::make_shared<example::CalcServiceProcessor>(handler);
    auto serverSocket = std::make_shared<TServerSocket>(port);
    auto transportFactory = std::make_shared<TBufferedTransportFactory>();
    auto protocolFactory = std::make_shared<TBinaryProtocolFactory>();

    TSimpleServer server(processor,
                         serverSocket,
                         transportFactory,
                         protocolFactory);
    server.serve();
    return 0;
}
