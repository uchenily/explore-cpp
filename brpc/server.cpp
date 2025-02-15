#include "echo.pb.h"
#include <brpc/server.h>
#include <butil/logging.h>
#include <gflags/gflags.h>
#include <json2pb/pb_to_json.h>

DEFINE_bool(echo_attachment, true, "Echo attachment as well");
DEFINE_int32(port, 8000, "TCP Port of this server");
DEFINE_string(listen_addr,
              "",
              "Server listen address, may be IPV4/IPV6/UDS."
              " If this is set, the flag port will be ignored");
DEFINE_int32(idle_timeout_s,
             -1,
             "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");

// Your implementation of example::EchoService
// Notice that implementing brpc::Describable grants the ability to put
// additional information in /status.
namespace example {
class EchoServiceImpl : public EchoService {
public:
    EchoServiceImpl() = default;
    ~EchoServiceImpl() override = default;

    void Echo(google::protobuf::RpcController *rpc_controller,
              const EchoRequest               *request,
              EchoResponse                    *response,
              google::protobuf::Closure       *done) override {
        // This object helps you to call done->Run() in RAII style. If you need
        // to process the request asynchronously, pass done_guard.release().
        brpc::ClosureGuard done_guard(done);

        auto controller = dynamic_cast<brpc::Controller *>(rpc_controller);

        // optional: set a callback function which is called after response is
        // sent and before cntl/req/res is destructed.
        controller->set_after_rpc_resp_fn(
            [](auto &&PH1, auto &&PH2, auto &&PH3) {
                return EchoServiceImpl::CallAfterRpc(
                    std::forward<decltype(PH1)>(PH1),
                    std::forward<decltype(PH2)>(PH2),
                    std::forward<decltype(PH3)>(PH3));
            });

        // The purpose of following logs is to help you to understand
        // how clients interact with servers more intuitively. You should
        // remove these logs in performance-sensitive servers.
        LOG(INFO) << "Received request[log_id=" << controller->log_id()
                  << "] from " << controller->remote_side() << " to "
                  << controller->local_side() << ": " << request->message()
                  << " (attached=" << controller->request_attachment() << ")";

        // Fill response.
        response->set_message(request->message());

        // You can compress the response by setting Controller, but be aware
        // that compression may be costly, evaluate before turning on.
        // cntl->set_response_compress_type(brpc::COMPRESS_TYPE_GZIP);

        if (FLAGS_echo_attachment) {
            // Set attachment which is wired to network directly instead of
            // being serialized into protobuf messages.
            controller->response_attachment().append(
                // controller->request_attachment()
                "YYY");
        }
    }

    // optional
    static void CallAfterRpc(brpc::Controller * /*cntl*/,
                             const google::protobuf::Message *req,
                             const google::protobuf::Message *res) {
        // at this time res is already sent to client, but cntl/req/res is not
        // destructed
        std::string request;
        std::string response;
        json2pb::ProtoMessageToJson(*req, &request, nullptr);
        json2pb::ProtoMessageToJson(*res, &response, nullptr);
        LOG(INFO) << "req:" << request << " resq:" << response;
    }
};
} // namespace example

auto main(int argc, char *argv[]) -> int {
    // Parse gflags. We recommend you to use gflags as well.
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);

    // Generally you only need one Server.
    brpc::Server server;

    // Instance of your service.
    example::EchoServiceImpl echo_service_impl;

    // Add the service into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use brpc::SERVER_OWNS_SERVICE.
    if (server.AddService(&echo_service_impl, brpc::SERVER_DOESNT_OWN_SERVICE)
        != 0) {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }

    butil::EndPoint endpoint;
    if (!FLAGS_listen_addr.empty()) {
        if (butil::str2endpoint(FLAGS_listen_addr.c_str(), &endpoint) < 0) {
            LOG(ERROR) << "Invalid listen address:" << FLAGS_listen_addr;
            return -1;
        }
    } else {
        endpoint = butil::EndPoint(butil::IP_ANY, FLAGS_port);
    }

    // Start the server.
    brpc::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    if (server.Start(endpoint, &options) != 0) {
        LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}
