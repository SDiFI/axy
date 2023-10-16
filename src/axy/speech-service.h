#ifndef AXY_SRC_AXY_SPEECH_SERVICE_CC_
#define AXY_SRC_AXY_SPEECH_SERVICE_CC_

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <sdifi/speech/v1alpha/speech.grpc.pb.h>
#include <sdifi/speech/v1alpha/speech.pb.h>
#include <sw/redis++/redis.h>
#include <tiro/speech/v1alpha/speech.grpc.pb.h>

#include <memory>
#include <string>

namespace axy {

class SpeechServiceImpl final
    : public sdifi::speech::v1alpha::SpeechService::CallbackService {
 public:
  explicit SpeechServiceImpl(
      const std::shared_ptr<grpc::Channel>& speech_server_channel,
      const std::string& redis_address)
      : stub_{tiro::speech::v1alpha::Speech::NewStub(speech_server_channel)},
        redis_client_{redis_address} {}
  // TODO(rkjaran): Make redis optional? Or add a generic callback interface for
  //   results?

  grpc::ServerBidiReactor<sdifi::speech::v1alpha::StreamingRecognizeRequest,
                          sdifi::speech::v1alpha::StreamingRecognizeResponse>*
  StreamingRecognize(grpc::CallbackServerContext* context) override;

 private:
  std::unique_ptr<tiro::speech::v1alpha::Speech::Stub> stub_;
  sw::redis::Redis redis_client_;
};

}  // namespace axy

#endif  // AXY_SRC_AXY_SPEECH_SERVICE_CC_
