#include "src/axy/server.h"

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/server_builder.h>

#include <chrono>
#include <cstdlib>
#include <memory>
#include <stdexcept>

#include "src/axy/logging.h"
#include "src/axy/speech-service.h"

namespace axy {

Server::Server(Options opts)
    : opts_{std::move(opts)},
      redis_{std::make_shared<sw::redis::Redis>(opts_.redis_address)},
      event_cb_service_{redis_},
      backend_speech_channel_{grpc::CreateChannel(
          opts_.backend_speech_server_address,
          [&]() {
            AXY_LOG_INFO("Connecting to speech service: '{}'",
                         opts_.backend_speech_server_address);
            if (opts_.backend_speech_server_address ==
                "speech.googleapis.com:443") {
              return grpc::GoogleDefaultCredentials();
            } else if (opts_.backend_speech_server_use_tls) {
              return grpc::SslCredentials({});
            } else {
              return grpc::InsecureChannelCredentials();
            }
          }())},
      speech_cb_service_{[&]() -> std::unique_ptr<SpeechService> {
        if (opts_.backend_speech_server_address ==
            "speech.googleapis.com:443") {
          auto quota_project = std::getenv("GOOGLE_CLOUD_QUOTA_PROJECT");
          if (quota_project == nullptr) {
            throw ServerError{
                "You need to specify a quota project to use Google Cloud "
                "Speech. Set the environment variable "
                "'GOOGLE_CLOUD_QUOTA_PROJECT'"};
          }
          return std::make_unique<SpeechServiceImpl<GoogleSpeechTypes>>(
              backend_speech_channel_, redis_,
              std::map<std::string, std::string>{
                  {"x-goog-user-project", quota_project}});
        }
        return std::make_unique<SpeechServiceImpl<TiroSpeechTypes>>(
            backend_speech_channel_, redis_);
      }()},
      grpc_server_{[&]() {
        grpc::EnableDefaultHealthCheckService(true);
        grpc::ServerBuilder server_builder{};
        server_builder.RegisterService(speech_cb_service_.get())
            .RegisterService(&event_cb_service_)
            .AddListeningPort(opts_.listen_address,
                              grpc::InsecureServerCredentials());
        return server_builder.BuildAndStart();
      }()} {
  if (grpc_server_ == nullptr) {
    throw ServerError{
        fmt::format("Could not build gRPC server listening on '{}'.",
                    opts_.listen_address)};
  }

  if (!backend_speech_channel_->WaitForConnected(
          std::chrono::system_clock::now() + opts_.backend_speech_wait_delay)) {
    throw ServerError{fmt::format(
        "Could not connect to backend speech server '{}' after {}.",
        opts_.backend_speech_server_address, opts_.backend_speech_wait_delay)};
  }
}

void Server::Wait() { grpc_server_->Wait(); }

void Server::Shutdown() {
  AXY_LOG_INFO("Shutting down with a deadline of {}", opts_.shutdown_timeout);
  if (grpc_server_ != nullptr) {
    grpc_server_->Shutdown(std::chrono::system_clock::now() +
                           opts_.shutdown_timeout);
  }
}

}  // namespace axy
