#include "echo.pb.h"
#include <brpc/channel.h>
#include <brpc/coroutine.h>
#include <brpc/server.h>
#include <butil/logging.h>
#include <gflags/gflags.h>

DEFINE_int32(port, 8000, "TCP Port of this server");
DEFINE_int32(sleep_us, 1000000, "Server sleep us");
DEFINE_bool(enable_coroutine, true, "Enable coroutine");

using brpc::experimental::Awaitable;
using brpc::experimental::Coroutine;

namespace example {
class EchoServiceImpl : public EchoService {
public:
    EchoServiceImpl() {
        brpc::ChannelOptions options;
        options.timeout_ms = FLAGS_sleep_us / 1000 * 2 + 100;
        options.max_retry = 0;
        CHECK(
            channel_.Init(butil::EndPoint(butil::IP_ANY, FLAGS_port), &options)
            == 0);
    }

    ~EchoServiceImpl() override = default;

    void Echo(google::protobuf::RpcController * /*rpc_controller*/,
              const EchoRequest         *request,
              EchoResponse              *response,
              google::protobuf::Closure *done) override {
        // brpc::Controller* cntl =
        //     static_cast<brpc::Controller*>(cntl_base);

        if (FLAGS_enable_coroutine) {
            Coroutine(EchoAsync(request, response, done), true);
        } else {
            brpc::ClosureGuard done_guard(done);
            bthread_usleep(FLAGS_sleep_us);
            response->set_message(request->message());
        }
    }

    auto EchoAsync(const EchoRequest         *request,
                   EchoResponse              *response,
                   google::protobuf::Closure *done) -> Awaitable<void> {
        brpc::ClosureGuard done_guard(done);
        co_await Coroutine::usleep(FLAGS_sleep_us);
        response->set_message(request->message());
        // co_return;
    }

private:
    brpc::Channel channel_;
};
} // namespace example

auto main(int argc, char *argv[]) -> int {
    bthread_setconcurrency(BTHREAD_MIN_CONCURRENCY);

    // Parse gflags. We recommend you to use gflags as well.
    GFLAGS_NAMESPACE::ParseCommandLineFlags(&argc, &argv, true);
    if (FLAGS_enable_coroutine) {
        GFLAGS_NAMESPACE::SetCommandLineOption("usercode_in_coroutine", "true");
    }

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

    // Start the server.
    brpc::ServerOptions options;
    options.num_threads = BTHREAD_MIN_CONCURRENCY;
    if (server.Start(FLAGS_port, &options) != 0) {
        LOG(ERROR) << "Fail to start EchoServer";
        return -1;
    }

    // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
    server.RunUntilAskedToQuit();
    return 0;
}
