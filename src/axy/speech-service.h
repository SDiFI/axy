#ifndef AXY_SRC_AXY_SPEECH_SERVICE_CC_
#define AXY_SRC_AXY_SPEECH_SERVICE_CC_

#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <sdifi/speech/v1alpha/speech.grpc.pb.h>
#include <sdifi/speech/v1alpha/speech.pb.h>
#include <sw/redis++/redis.h>
#include <tiro/speech/v1alpha/speech.grpc.pb.h>
#include <tiro/speech/v1alpha/speech.pb.h>

#include <memory>
#include <string>

namespace axy {

// clang-format off
// clang-format hasn't got support for concepts yet
template <typename T>
concept GoogleApiCompatibleTypes = requires(T) {
  typename T::Speech;
  typename T::StreamingRecognizeRequest;
  typename T::StreamingRecognizeResponse;
  typename T::RecognitionConfig;
};

template <typename T>
concept StreamingRecognizeResponse = requires(T r) {
  r.speech_event_type();
  r.results(0);
  { r.results(0).is_final() } -> std::same_as<bool>;
};
// clang-format on

struct TiroSpeechTypes {
  using Speech = tiro::speech::v1alpha::Speech;
  using StreamingRecognizeRequest =
      tiro::speech::v1alpha::StreamingRecognizeRequest;
  using StreamingRecognizeResponse =
      tiro::speech::v1alpha::StreamingRecognizeResponse;
  using RecognitionConfig = tiro::speech::v1alpha::RecognitionConfig;
};

template <GoogleApiCompatibleTypes BackendTypes>
class SpeechServiceImpl final
    : public sdifi::speech::v1alpha::SpeechService::CallbackService {
 public:
  explicit SpeechServiceImpl(
      const std::shared_ptr<grpc::Channel>& speech_server_channel,
      const std::string& redis_address)
      : stub_{BackendTypes::Speech::NewStub(speech_server_channel)},
        redis_client_{redis_address} {}
  // TODO(rkjaran): Make redis optional? Or add a generic callback interface for
  //   results?

  grpc::ServerBidiReactor<sdifi::speech::v1alpha::StreamingRecognizeRequest,
                          sdifi::speech::v1alpha::StreamingRecognizeResponse>*
  StreamingRecognize(grpc::CallbackServerContext* context) override;

 private:
  std::unique_ptr<typename BackendTypes::Speech::Stub> stub_;
  sw::redis::Redis redis_client_;
};

}  // namespace axy

#endif  // AXY_SRC_AXY_SPEECH_SERVICE_CC_
