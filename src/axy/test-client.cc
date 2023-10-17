#include <google/rpc/status.pb.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <sdifi/speech/v1alpha/speech.grpc.pb.h>

#include <CLI/CLI.hpp>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

std::string GetFileContents(const std::string &filename) {
  std::string contents;
  std::ifstream is{filename, std::ios::ate};
  is.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  auto size = is.tellg();
  is.seekg(0);
  contents.resize(size, '\0');
  is.read(&contents[0], size);
  return contents;
}

void PrintErrorWithDetails(const grpc::Status &stat,
                           std::ostream &os = std::cerr) {
  os << "Couldn't send Recognize request" << '\n'
     << "Message: " << stat.error_message() << '\n';

  if (!stat.error_details().empty()) {
    os << "... Details:\n";
    google::rpc::Status status_details;
    status_details.ParseFromString(stat.error_details());
    for (const auto &detail : status_details.details()) {
      os << "Unknown error detail type: \n" << detail.Utf8DebugString() << '\n';
    }
  }
}

int main(int argc, char *argv[]) {
  CLI::App app{"Test client for Axy"};
  app.option_defaults()->always_capture_default();

  std::string wave_filename;
  app.add_option("wave_filename", wave_filename,
                 "Audio file (WAVE s16le). Use - for stdin.")
      ->required()
      ->option_text(" ");

  std::string conversation_id;
  app.add_option("conversation_id", conversation_id,
                 "Conversation ID for Masdif")
      ->required()
      ->option_text(" ");

  std::int32_t sample_rate_hertz = 16000;
  app.add_option("-r,--sample-rate-hertz", sample_rate_hertz,
                 "Sample rate in hertz");

  std::string language_code = "is-IS";
  app.add_option("-l,--language-code", language_code, "Language code");

  std::string server_address = "localhost:50051";
  app.add_option("--server-address", server_address);

  bool single_utterance = false;
  app.add_flag("--single-utterance", single_utterance);

  CLI11_PARSE(app, argc, argv);

  auto creds = grpc::InsecureChannelCredentials();
  auto channel = grpc::CreateChannel(server_address, creds);
  auto stub = sdifi::speech::v1alpha::SpeechService::NewStub(channel);

  sdifi::speech::v1alpha::RecognitionConfig config;
  config.set_encoding(sdifi::speech::v1alpha::RecognitionConfig::LINEAR16);
  config.set_sample_rate_hertz(sample_rate_hertz);
  config.set_language_code(language_code);
  config.set_enable_automatic_punctuation(true);

  std::cerr << config.Utf8DebugString() << '\n';

  grpc::ClientContext ctx;
  ctx.set_wait_for_ready(true);
  auto stream = stub->StreamingRecognize(&ctx);

  std::jthread reader{[&stream]() {
    try {
      sdifi::speech::v1alpha::StreamingRecognizeResponse res;
      stream->WaitForInitialMetadata();
      while (stream->Read(&res)) {
        std::cerr << res.Utf8DebugString();
        if (res.results_size() > 0) {
          if (res.results(0).is_final()) {
            std::cout << '\r' << res.results(0).alternatives(0).transcript()
                      << '\n';
          } else {
            std::string partial = "";
            for (const auto &partial_result : res.results()) {
              partial += partial_result.alternatives(0).transcript();
            }
            std::cout << "\r" << partial;
          }
        }
      }
      std::cerr << "done reading\n";
    } catch (const std::exception &e) {
      std::cerr << "reader failure\n";
      std::cerr << e.what() << '\n';
      exit(EXIT_FAILURE);
    }
  }};

  std::ifstream wave_stream{wave_filename == "-" ? "/dev/stdin" : wave_filename,
                            std::ios::binary};
  if (!wave_stream.is_open()) {
    std::cerr << "Could not open stream from '" << wave_filename << "'\n";
    return EXIT_FAILURE;
  }
  sdifi::speech::v1alpha::StreamingRecognizeRequest req;
  req.mutable_streaming_config()->mutable_config()->CopyFrom(config);
  req.mutable_streaming_config()->set_interim_results(true);
  req.mutable_streaming_config()->set_conversation(conversation_id);
  req.mutable_streaming_config()->set_single_utterance(single_utterance);

  if (!stream->Write(req)) {
    std::cerr << "Initial write failed\n";
    return EXIT_FAILURE;
  }

  const std::size_t content_chunk_size_bytes = 1024 * 2;
  req.mutable_audio_content()->resize(content_chunk_size_bytes);
  while (
      !wave_stream
           .read(req.mutable_audio_content()->data(), content_chunk_size_bytes)
           .eof()) {
    if (!stream->Write(req)) {
      std::cerr << "Write failed\n";
      break;
    }
  }

  stream->WritesDone();
  if (grpc::Status stat = stream->Finish(); !stat.ok()) {
    PrintErrorWithDetails(stat);
    return EXIT_FAILURE;
  }

  return 0;
}
