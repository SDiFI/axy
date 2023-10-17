#ifndef AXY_SRC_AXY_SERVER_H_
#define AXY_SRC_AXY_SERVER_H_

#include <grpcpp/channel.h>
#include <grpcpp/server.h>

#include <memory>
#include <stdexcept>
#include <string>

#include "src/axy/speech-service.h"

namespace axy {

struct ServerError : public std::runtime_error {
  explicit ServerError(const std::string& msg) : std::runtime_error(msg) {}
};

class Server final {
 public:
  struct Options {
    std::string listen_address = "localhost:50051";
    bool backend_speech_server_use_tls = true;
    std::string backend_speech_server_address = "speech.tiro.is:443";
    std::chrono::seconds backend_speech_wait_delay{10};
    std::string redis_address = "tcp://localhost:6379";
    std::chrono::seconds shutdown_timeout{60};
  };

  explicit Server(Options opts);
  void Wait();
  void Shutdown();
  ~Server() { Shutdown(); }

 private:
  Options opts_;
  std::shared_ptr<grpc::Channel> backend_speech_channel_;
  axy::SpeechServiceImpl speech_cb_service_;
  std::unique_ptr<grpc::Server> grpc_server_;
};

}  // namespace axy

#endif  // AXY_SRC_AXY_SERVER_H_
