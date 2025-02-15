#include "http.pb.h"
#include <brpc/restful.h>
#include <brpc/server.h>
#include <butil/logging.h>
#include <gflags/gflags.h>
#include <json2pb/pb_to_json.h>

DEFINE_int32(port, 8010, "TCP Port of this server");
DEFINE_int32(idle_timeout_s,
             -1,
             "Connection will be closed if there is no "
             "read/write operations during the last `idle_timeout_s'");

DEFINE_string(certificate, "cert.pem", "Certificate file path to enable SSL");
DEFINE_string(private_key, "key.pem", "Private key file path to enable SSL");
DEFINE_string(ciphers, "", "Cipher suite used for SSL connections");

namespace example {

// Service with static path.
class HttpServiceImpl : public HttpService {
public:
    HttpServiceImpl() = default;
    ~HttpServiceImpl() override = default;
    void Echo(google::protobuf::RpcController *rpc_controller,
              const HttpRequest * /*request*/,
              HttpResponse * /*response*/,
              google::protobuf::Closure *done) override {
        // This object helps you to call done->Run() in RAII style. If you need
        // to process the request asynchronously, pass done_guard.release().
        brpc::ClosureGuard done_guard(done);

        auto cntl = dynamic_cast<brpc::Controller *>(rpc_controller);

        // optional: set a callback function which is called after response is
        // sent and before cntl/req/res is destructed.
        cntl->set_after_rpc_resp_fn([](auto &&PH1, auto &&PH2, auto &&PH3) {
            return HttpServiceImpl::CallAfterRpc(
                std::forward<decltype(PH1)>(PH1),
                std::forward<decltype(PH2)>(PH2),
                std::forward<decltype(PH3)>(PH3));
        });

        // Fill response.
        cntl->http_response().set_content_type("text/plain");
        butil::IOBufBuilder os;
        os << "queries:";
        for (brpc::URI::QueryIterator it
             = cntl->http_request().uri().QueryBegin();
             it != cntl->http_request().uri().QueryEnd();
             ++it) {
            os << ' ' << it->first << '=' << it->second;
        }
        os << "\nbody: " << cntl->request_attachment() << '\n';
        os.move_to(cntl->response_attachment());
    }

    // optional
    static void CallAfterRpc(brpc::Controller * /*controller*/,
                             const google::protobuf::Message *req,
                             const google::protobuf::Message *resp) {
        // at this time res is already sent to client, but cntl/req/res is not
        // destructed
        std::string req_str;
        std::string res_str;
        json2pb::ProtoMessageToJson(*req, &req_str, nullptr);
        json2pb::ProtoMessageToJson(*resp, &res_str, nullptr);
        LOG(INFO) << "req:" << req_str << " res:" << res_str;
    }
};

} // namespace example

auto main(int argc, char *argv[]) -> int {
    // Parse gflags. We recommend you to use gflags as well.
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);

    // Generally you only need one Server.
    brpc::Server server;

    example::HttpServiceImpl http_svc;
    // Add services into server. Notice the second parameter, because the
    // service is put on stack, we don't want server to delete it, otherwise
    // use brpc::SERVER_OWNS_SERVICE.
    if (server.AddService(&http_svc, brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add http_svc";
        return -1;
    }

    // Start the server.
    brpc::ServerOptions options;
    options.idle_timeout_sec = FLAGS_idle_timeout_s;
    // options.mutable_ssl_options()->default_cert.certificate =
    // FLAGS_certificate;
    // options.mutable_ssl_options()->default_cert.private_key =
    // FLAGS_private_key;
    // options.mutable_ssl_options()->ciphers = FLAGS_ciphers;
    if (server.Start(FLAGS_port, &options) != 0) {
        LOG(ERROR) << "Fail to start HttpServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}
