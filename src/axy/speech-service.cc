#include "src/axy/speech-service.h"

#include <fmt/core.h>
#include <google/protobuf/util/time_util.h>
#include <grpcpp/client_context.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/client_callback.h>
#include <grpcpp/support/status.h>
#include <sdifi/events/v1alpha/event.pb.h>
#include <sdifi/speech/v1alpha/speech.pb.h>
#include <sw/redis++/redis.h>
#include <tiro/speech/v1alpha/speech.grpc.pb.h>
#include <tiro/speech/v1alpha/speech.pb.h>

#include <atomic>
#include <concepts>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "src/axy/logging.h"

namespace axy {

namespace {

auto Convert(
    const tiro::speech::v1alpha::StreamingRecognizeResponse::SpeechEventType&
        in_event_type) {
  switch (in_event_type) {
    using In = tiro::speech::v1alpha::StreamingRecognizeResponse;
    using Out = sdifi::speech::v1alpha::StreamingRecognizeResponse;
    case In::END_OF_SINGLE_UTTERANCE:
      return Out::END_OF_SINGLE_UTTERANCE;
    default:
      return Out::SPEECH_EVENT_UNSPECIFIED;
  }
}

template <typename T>
concept StreamingRecognizeResponse = requires(T r) {
                                       r.speech_event_type();
                                       r.results(0);
                                       {
                                         r.results(0).is_final()
                                         } -> std::same_as<bool>;
                                     };

/** Convert a streaming recognize response to an Event
 *
 * \returns The event type if the conversion was successful
 */
template <StreamingRecognizeResponse T>
std::optional<std::string> ConvertToEvent(
    const std::string& conversation_id, const T& resp,
    sdifi::events::v1alpha::Event& event) {
  auto md = event.mutable_metadata();

  md->mutable_created_at()->CopyFrom(
      google::protobuf::util::TimeUtil::GetCurrentTime());

  auto convo = md->mutable_conversation();
  convo->set_name(conversation_id);

  if (resp.speech_event_type() != T::SPEECH_EVENT_UNSPECIFIED) {
    event.mutable_speech_partial()->set_speech_event_type(
        Convert(resp.speech_event_type()));
  } else if (resp.results_size() > 0) {
    auto result = resp.results(0);
    if (result.alternatives_size() > 0 &&
        !result.alternatives(0).transcript().empty()) {
      if (result.is_final()) {
        event.mutable_speech_final()->set_transcript(
            result.alternatives(0).transcript());
      } else {
        event.mutable_speech_partial()->set_transcript(
            result.alternatives(0).transcript());
      }
    }
  }

  std::string type;
  switch (event.payload_case()) {
    using sdifi::events::v1alpha::Event;
    case Event::kSpeechContent:
      type = event.speech_content().GetTypeName();
      break;
    case Event::kSpeechPartial:
      type = event.speech_partial().GetTypeName();
      break;
    case Event::kSpeechFinal:
      type = event.speech_final().GetTypeName();
      break;
    case Event::PAYLOAD_NOT_SET:
      type = "<unk>";
      break;
    default:
      return std::nullopt;
  }

  return type;
}

void ConvertRequest(sdifi::speech::v1alpha::StreamingRecognizeRequest& in,
                    tiro::speech::v1alpha::StreamingRecognizeRequest& out) {
  out.Clear();
  if (in.has_streaming_config()) {
    auto* out_streaming_config = out.mutable_streaming_config();

    out_streaming_config->set_interim_results(
        in.streaming_config().interim_results());
    out_streaming_config->set_single_utterance(
        in.streaming_config().single_utterance());

    auto& in_config = in.streaming_config().config();
    auto* out_rec_config = out_streaming_config->mutable_config();
    out_rec_config->set_enable_automatic_punctuation(
        in_config.enable_automatic_punctuation());
    if (in_config.language_code().empty()) {
      out_rec_config->set_language_code("is-IS");
    } else {
      out_rec_config->set_language_code(in_config.language_code());
    }
    out_rec_config->set_enable_word_time_offsets(
        in_config.enable_word_time_offsets());
    out_rec_config->set_sample_rate_hertz(in_config.sample_rate_hertz());
    out_rec_config->set_max_alternatives(in_config.max_alternatives());

    switch (in_config.encoding()) {
      using In = sdifi::speech::v1alpha::RecognitionConfig;
      case In::LINEAR16:
        [[fallthrough]];
      case In::ENCODING_UNSPECIFIED:
        [[fallthrough]];
      default:
        out_rec_config->set_encoding(
            tiro::speech::v1alpha::RecognitionConfig::LINEAR16);
    }

  } else if (in.has_audio_content()) {
    out.set_allocated_audio_content(in.release_audio_content());
  }
}

void ConvertResponse(
    const tiro::speech::v1alpha::StreamingRecognizeResponse& in,
    sdifi::speech::v1alpha::StreamingRecognizeResponse& out) {
  out.Clear();
  if (in.has_error()) {
    out.mutable_error()->CopyFrom(in.error());
  } else if (in.speech_event_type() !=
             tiro::speech::v1alpha::StreamingRecognizeResponse::
                 SPEECH_EVENT_UNSPECIFIED) {
    switch (in.speech_event_type()) {
      using Ev = tiro::speech::v1alpha::StreamingRecognizeResponse;
      using OutEv = sdifi::speech::v1alpha::StreamingRecognizeResponse;

      case Ev::END_OF_SINGLE_UTTERANCE:
        out.set_speech_event_type(OutEv::END_OF_SINGLE_UTTERANCE);
        break;

      default:
        AXY_LOG_WARN("Unknown event type, ignoring...");
    }
  } else if (in.results_size() > 0) {
    for (const auto& res : in.results()) {
      auto out_res = out.add_results();
      out_res->set_is_final(res.is_final());

      for (const auto& alt : res.alternatives()) {
        auto out_alt = out_res->add_alternatives();
        out_alt->set_transcript(alt.transcript());

        for (const auto& words : alt.words()) {
          auto out_words = out_alt->add_words();
          out_words->set_word(words.word());
          out_words->mutable_start_time()->CopyFrom(words.start_time());
          out_words->mutable_end_time()->CopyFrom(words.end_time());
        }
      }
    }
  }
}

}  // namespace

grpc::ServerBidiReactor<sdifi::speech::v1alpha::StreamingRecognizeRequest,
                        sdifi::speech::v1alpha::StreamingRecognizeResponse>*
SpeechServiceImpl::StreamingRecognize(grpc::CallbackServerContext* context) {
  // This is a self deleting callback reactor
  class ServerReactor
      : public grpc::ServerBidiReactor<
            sdifi::speech::v1alpha::StreamingRecognizeRequest,
            sdifi::speech::v1alpha::StreamingRecognizeResponse> {
   public:
    explicit ServerReactor(grpc::CallbackServerContext* context,
                           tiro::speech::v1alpha::Speech::Stub* stub,
                           sw::redis::Redis* redis_client)
        : client_reactor_{new ClientReactor{
              this, stub,
              grpc::ClientContext::FromCallbackServerContext(*context),
              redis_client}} {
      StartRead(&req);
      client_reactor_->StartRead(&client_reactor_->in_resp);
      client_reactor_->AddHold();
      client_reactor_->StartCall();
    }

    void OnReadDone(bool ok) override {
      if (ok) {
        if (req.has_streaming_config()) {
          conversation_id_ = req.streaming_config().conversation();
        }
        ConvertRequest(req, client_reactor_->out_req);
        client_reactor_->StartWrite(&client_reactor_->out_req);
      } else {
        client_reactor_->StartWritesDone();
      }
    }

    void OnDone() override {
      AXY_LOG_INFO("{}: server all done.", conversation_id_);
      client_reactor_->server_gone = true;
      client_reactor_->RemoveHold();
      delete this;
    }

    void OnCancel() override {
      AXY_LOG_DEBUG("{}: server cancelled.", conversation_id_);
    }

    void OnWriteDone(bool ok) override {
      if (!ok) {
        fmt::println(stderr, "{}: no more server writes", conversation_id_);
        Finish(grpc::Status::CANCELLED);
      }
    }

   private:
    // TODO(rkjaran): Generalize this client callback reactor for more backends
    class ClientReactor
        : public grpc::ClientBidiReactor<
              tiro::speech::v1alpha::StreamingRecognizeRequest,
              tiro::speech::v1alpha::StreamingRecognizeResponse> {
     public:
      explicit ClientReactor(ServerReactor* server_reactor,
                             tiro::speech::v1alpha::Speech::Stub* stub,
                             std::unique_ptr<grpc::ClientContext> ctx,
                             sw::redis::Redis* redis_client)
          : server_reactor_{server_reactor},
            ctx_{std::move(ctx)},
            redis_client_{redis_client} {
        stub->async()->StreamingRecognize(ctx_.get(), this);
      }

      void OnReadDone(bool ok) override {
        if (ok) {
          if (!server_gone) {
            ConvertResponse(in_resp, server_reactor_->resp);
            server_reactor_->StartWrite(&server_reactor_->resp);
          }

          if (redis_client_ != nullptr) {
            sdifi::events::v1alpha::Event event;

            if (auto type = ConvertToEvent(server_reactor_->conversation_id_,
                                           in_resp, event)) {
              std::string stream_key = fmt::format(
                  "sdifi/conversation/{key}",
                  fmt::arg("key", server_reactor_->conversation_id_));

              AXY_LOG_INFO("Writing message to {}", stream_key);
              std::vector<std::pair<std::string, std::string>> attrs{
                  {":type", type.value()},
                  {":content", event.SerializeAsString()}};
              redis_client_->xadd(stream_key, "*", attrs.begin(), attrs.end());
            }
          }

          StartRead(&in_resp);
        } else {
          AXY_LOG_DEBUG("no more client reads");
          if (!server_gone) {
            server_reactor_->Finish(grpc::Status::OK);
          }
        }
      }

      void OnWriteDone(bool ok) override {
        if (ok && !server_gone) {
          server_reactor_->StartRead(&server_reactor_->req);
        } else {
          AXY_LOG_DEBUG("client write went bad");
        }
      }

      void OnWritesDoneDone(bool ok) override {
        AXY_LOG_DEBUG("client writesdone done");
      }

      void OnDone(const grpc::Status&) override {
        AXY_LOG_INFO("client all done");
        delete this;
      }

     private:
      ServerReactor* server_reactor_;
      std::unique_ptr<grpc::ClientContext> ctx_;
      sw::redis::Redis* redis_client_;

     public:
      std::atomic<bool> server_gone = false;
      tiro::speech::v1alpha::StreamingRecognizeResponse in_resp;
      tiro::speech::v1alpha::StreamingRecognizeRequest out_req;
    };

    ClientReactor* client_reactor_;

   public:
    sdifi::speech::v1alpha::StreamingRecognizeRequest req;
    sdifi::speech::v1alpha::StreamingRecognizeResponse resp;
    std::string conversation_id_ = "<unk>";
  };

  // ServerReactor deletes itself once finished.
  return new ServerReactor{context, stub_.get(), &redis_client_};
}

}  // namespace axy
