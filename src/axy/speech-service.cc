#include "src/axy/speech-service.h"

#include <fmt/core.h>
#include <grpcpp/client_context.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/client_callback.h>
#include <grpcpp/support/status.h>
#include <sdifi/speech/v1alpha/speech.pb.h>
#include <tiro/speech/v1alpha/speech.pb.h>

#include <atomic>
#include <cstdio>
#include <iostream>
#include <memory>

namespace axy {

namespace {

void ConvertRequest(sdifi::speech::v1alpha::StreamingRecognizeRequest& in,
                    tiro::speech::v1alpha::StreamingRecognizeRequest& out) {
  out.Clear();
  if (in.has_streaming_config()) {
    fmt::println(stderr, "conversation {}",
                 in.streaming_config().conversation());

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
        fmt::println(stderr, "Unknown event type, ignoring...");
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
  class ServerReactor
      : public grpc::ServerBidiReactor<
            sdifi::speech::v1alpha::StreamingRecognizeRequest,
            sdifi::speech::v1alpha::StreamingRecognizeResponse> {
   public:
    explicit ServerReactor(grpc::CallbackServerContext* context,
                           tiro::speech::v1alpha::Speech::Stub* stub,
                           int conn_id)
        : conn_id_{conn_id},
          client_ctx_{grpc::ClientContext::FromCallbackServerContext(*context)},
          client_reactor_{new ClientReactor{this}} {
      stub->async()->StreamingRecognize(client_ctx_.get(), client_reactor_);

      StartRead(&req_);

      client_reactor_->StartRead(&client_reactor_->in_resp_);
      client_reactor_->AddHold();
      client_reactor_->StartCall();
    }

    void OnReadDone(bool ok) override {
      if (ok) {
        ConvertRequest(req_, client_reactor_->out_req_);
        client_reactor_->StartWrite(&client_reactor_->out_req_);
      } else {
        client_reactor_->StartWritesDone();
        client_reactor_->RemoveHold();
      }
    }

    void OnDone() override { delete this; }

    void OnCancel() override { Finish(grpc::Status::CANCELLED); }

    void OnWriteDone(bool ok) override {
      if (!ok) {
        fmt::println(stderr, "{}: no more server writes", conn_id_);
        Finish(grpc::Status::CANCELLED);
      }
    }

   private:
    int conn_id_;

    std::unique_ptr<grpc::ClientContext> client_ctx_;

    sdifi::speech::v1alpha::StreamingRecognizeRequest req_;
    sdifi::speech::v1alpha::StreamingRecognizeResponse resp_;

    // TODO(rkjaran): Generalize this client callback reactor for more backends
    class ClientReactor
        : public grpc::ClientBidiReactor<
              tiro::speech::v1alpha::StreamingRecognizeRequest,
              tiro::speech::v1alpha::StreamingRecognizeResponse> {
     public:
      explicit ClientReactor(ServerReactor* server_reactor)
          : server_reactor_{server_reactor} {}

      void OnReadDone(bool ok) override {
        if (ok) {
          ConvertResponse(in_resp_, server_reactor_->resp_);
          server_reactor_->StartWrite(&server_reactor_->resp_);

          StartRead(&in_resp_);
        } else {
          fmt::println(stderr, "{}: no more client reads",
                       server_reactor_->conn_id_);
          server_reactor_->Finish(grpc::Status::OK);
        }
      }

      void OnWriteDone(bool ok) override {
        if (ok) {
          server_reactor_->StartRead(&server_reactor_->req_);
        } else {
          fmt::println(stderr, "{}: client write went bad",
                       server_reactor_->conn_id_);
        }
      }

      void OnDone(const grpc::Status&) override {
        fmt::println(stderr, "{}: client all done", server_reactor_->conn_id_);
        delete this;
      }

     private:
      friend ServerReactor;
      ServerReactor* server_reactor_;

      tiro::speech::v1alpha::StreamingRecognizeResponse in_resp_;
      tiro::speech::v1alpha::StreamingRecognizeRequest out_req_;
    }* client_reactor_;
  };

  static int conn_id = 0;

  // ServerReactor deletes itself once finished.
  return new ServerReactor{context, stub_.get(), ++conn_id};
}

}  // namespace axy
