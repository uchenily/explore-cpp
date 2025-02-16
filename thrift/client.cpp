#include "CalcService.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/THttpClient.h>
#include <thrift/transport/TSocket.h>

#include <iostream>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

auto main() -> int {
    const std::string serverAddress = "127.0.0.1";
    const int         port = 9090;

    auto socket = std::make_shared<TSocket>(serverAddress, port);
    auto transport = std::make_shared<TBufferedTransport>(socket);
    auto protocol = std::make_shared<TBinaryProtocol>(transport);
    example::CalcServiceClient client(protocol);

    try {
        transport->open();
        int32_t result = client.add(3, 4);
        std::cout << "Result: " << result << '\n';
        transport->close();
    } catch (TException &tx) {
        std::cout << "ERROR: " << tx.what() << '\n';
    }

    return 0;
}
