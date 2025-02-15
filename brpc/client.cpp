#include "echo.pb.h"
#include <brpc/channel.h>
#include <butil/logging.h>
#include <butil/time.h>
#include <gflags/gflags.h>

// DEFINE_string(attachment, "XXX", "Carry this along with requests");
DEFINE_string(protocol,
              // "baidu_std", // 可以添加attachment
              "h2:grpc",
              "Protocol type. Defined in src/brpc/options.proto");
DEFINE_string(connection_type,
              "",
              "Connection type. Available values: single, pooled, short");
DEFINE_string(server, "0.0.0.0:8000", "IP Address of server");
DEFINE_string(load_balancer, "", "The algorithm for load balancing");
DEFINE_int32(timeout_ms, 100, "RPC timeout in milliseconds");
DEFINE_int32(max_retry, 3, "Max retries(not including the first RPC)");
DEFINE_int32(interval_ms, 1000, "Milliseconds between consecutive requests");

auto main(int argc, char *argv[]) -> int {
    // Parse gflags. We recommend you to use gflags as well.
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);

    // A Channel represents a communication line to a Server. Notice that
    // Channel is thread-safe and can be shared by all threads in your program.
    brpc::Channel channel;

    // Initialize the channel, NULL means using default options.
    brpc::ChannelOptions options;
    options.protocol = FLAGS_protocol;
    options.connection_type = FLAGS_connection_type;
    options.timeout_ms = FLAGS_timeout_ms /*milliseconds*/;
    options.max_retry = FLAGS_max_retry;
    if (channel.Init(FLAGS_server.c_str(),
                     FLAGS_load_balancer.c_str(),
                     &options)
        != 0) {
        LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }

    // Normally, you should not call a Channel directly, but instead construct
    // a stub Service wrapping it. stub can be shared by all threads as well.
    example::EchoService_Stub stub(&channel);

    // Send a request and wait for the response every 1 second.
    int log_id = 0;
    while (!brpc::IsAskedToQuit()) {
        // We will receive response synchronously, safe to put variables
        // on stack.
        example::EchoRequest  request;
        example::EchoResponse response;
        brpc::Controller      controller;

        request.set_message("hello world");

        controller.set_log_id(log_id++); // set by user
        // Set attachment which is wired to network directly instead of
        // being serialized into protobuf messages.
        // controller.request_attachment().append(FLAGS_attachment);

        // Because `done'(last parameter) is NULL, this function waits until
        // the response comes back or error occurs(including timedout).
        stub.Echo(&controller, &request, &response, nullptr);
        if (!controller.Failed()) {
            LOG(INFO) << "Received response from " << controller.remote_side()
                      << " to " << controller.local_side() << ": "
                      << response.message()
                      // << " (attached=" << controller.response_attachment()
                      // << ")"
                      << " latency=" << controller.latency_us() << "us";
        } else {
            LOG(WARNING) << controller.ErrorText();
        }
        usleep(FLAGS_interval_ms * 1000L);
    }

    LOG(INFO) << "EchoClient is going to quit";
    return 0;
}
