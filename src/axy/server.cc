#include "src/axy/server.h"

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/server_builder.h>

#include <chrono>
#include <memory>
#include <stdexcept>

namespace axy {

Server::Server(Options opts)
    : opts_{std::move(opts)},
      backend_speech_channel_{
          grpc::CreateChannel(opts_.backend_speech_server_address,
                              [&]() {
                                if (opts_.backend_speech_server_use_tls) {
                                  return grpc::SslCredentials({});
                                } else {
                                  return grpc::InsecureChannelCredentials();
                                }
                              }())},
      speech_cb_service_{backend_speech_channel_},
      grpc_server_{[&]() {
        gpr_set_log_verbosity(gpr_log_severity::GPR_LOG_SEVERITY_DEBUG);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();
        grpc::EnableDefaultHealthCheckService(true);
        grpc::ServerBuilder server_builder{};
        server_builder.RegisterService(&speech_cb_service_)
            .AddListeningPort(opts_.listen_address,
                              grpc::InsecureServerCredentials());
        return server_builder.BuildAndStart();
      }()},
      redis_client_{opts_.redis_address}

{
  if (grpc_server_ == nullptr) {
    throw ServerError{
        fmt::format("Could not build gRPC server listening on '{}'.",
                    opts_.listen_address)};
  }

  if (!backend_speech_channel_->WaitForConnected(
          std::chrono::system_clock::now() +
          opts_.backend_speech_wait_delay_)) {
    throw ServerError{fmt::format(
        "Could not connect to backend speech server '{}' after {}.",
        opts_.backend_speech_server_address, opts_.backend_speech_wait_delay_)};
  }
}

void Server::Wait() { grpc_server_->Wait(); }

void Server::Shutdown() {
  if (grpc_server_ != nullptr) {
    grpc_server_->Shutdown();
  }
}

}  // namespace axy
