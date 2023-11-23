#ifndef AXY_SRC_AXY_SPEECH_SERVICE_CC_
#define AXY_SRC_AXY_SPEECH_SERVICE_CC_

#include <google/cloud/speech/v1/speech_client.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <sdifi/speech/v1alpha/speech.grpc.pb.h>
#include <sdifi/speech/v1alpha/speech.pb.h>
#include <sw/redis++/redis.h>
#include <tiro/speech/v1alpha/speech.grpc.pb.h>
#include <tiro/speech/v1alpha/speech.pb.h>

#include <concepts>
#include <map>
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

struct GoogleSpeechTypes {
  using Speech = google::cloud::speech::v1::Speech;
  using StreamingRecognizeRequest =
      google::cloud::speech::v1::StreamingRecognizeRequest;
  using StreamingRecognizeResponse =
      google::cloud::speech::v1::StreamingRecognizeResponse;
  using RecognitionConfig = google::cloud::speech::v1::RecognitionConfig;
};

using SpeechService = sdifi::speech::v1alpha::SpeechService::CallbackService;

template <GoogleApiCompatibleTypes BackendTypes>
class SpeechServiceImpl final
    : public sdifi::speech::v1alpha::SpeechService::CallbackService {
 public:
  explicit SpeechServiceImpl(
      const std::shared_ptr<grpc::Channel>& speech_server_channel,
      std::shared_ptr<sw::redis::Redis> redis_client,
      std::map<std::string, std::string> extra_headers = {})
      : stub_{BackendTypes::Speech::NewStub(speech_server_channel)},
        redis_client_{std::move(redis_client)},
        extra_headers_{std::move(extra_headers)} {}
  // TODO(rkjaran): Make redis optional? Or add a generic callback interface for
  //   results?

  grpc::ServerBidiReactor<sdifi::speech::v1alpha::StreamingRecognizeRequest,
                          sdifi::speech::v1alpha::StreamingRecognizeResponse>*
  StreamingRecognize(grpc::CallbackServerContext* context) override;

 private:
  std::unique_ptr<typename BackendTypes::Speech::Stub> stub_;
  std::shared_ptr<sw::redis::Redis> redis_client_;
  const std::map<std::string, std::string> extra_headers_;
};

}  // namespace axy

#endif  // AXY_SRC_AXY_SPEECH_SERVICE_CC_
