#include <fmt/core.h>

#include <CLI/CLI.hpp>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <thread>

#include "CLI/Validators.hpp"
#include "internal_use_only/config.h"
#include "src/axy/logging.h"
#include "src/axy/server.h"
#include "src/axy/speech-service.h"

std::atomic_flag g_stop_flag = ATOMIC_FLAG_INIT;

std::string signum_to_string(int signum) {
  switch (signum) {
    case SIGINT:
      return "SIGINT";
    case SIGTERM:
      return "SIGTERM";
    default:
      return fmt::format("Unknown signal {}", signum);
  }
}

void handle_signal(int signum) {
  AXY_LOG_DEBUG("Handling signal {}", signum_to_string(signum));
  if (signum == SIGINT || signum == SIGTERM) {
    AXY_LOG_INFO("Shutting down gracefully...");
    g_stop_flag.test_and_set();
    g_stop_flag.notify_all();
  }
}

int main(int argc, char* argv[]) {
  try {
    CLI::App app{"Asynchronous/ASR proxy for SDiFI"};
    app.option_defaults()->always_capture_default();
    app.set_version_flag("--version", std::string{axy::project_version});

    std::string log_level = "info";
    app.add_option("--log-level", log_level)
        ->check(CLI::IsMember({"trace", "debug", "info", "warn", "error"}))
        ->ignore_case();

    axy::Server::Options server_opts;
    app.add_option("--listen-address", server_opts.listen_address);
    app.add_option("--backend-speech-server-address",
                   server_opts.backend_speech_server_address,
                   "gRPC server that provides the "
                   "`tiro.speech.v1alpha.Speech` service.");
    app.add_flag("--backend-speech-server-use-tls",
                 server_opts.backend_speech_server_use_tls);
    app.add_option("--redis-address", server_opts.redis_address,
                   "The server will write conversation events to streams with "
                   "keys 'sdifi/conversation/{conv_id}' where {conv_id} is the "
                   "conversation ID.");
    app.add_option("--shutdown-timeout-seconds", server_opts.shutdown_timeout,
                   "Deadline for graceful shutdown.");

    CLI11_PARSE(app, argc, argv);

    axy::SetLogLevel(log_level);
    axy::RegisterLibraryLogHandlers();

    axy::Server server{server_opts};

    AXY_LOG_INFO("Server listening on {}", server_opts.listen_address);

    std::signal(SIGTERM, handle_signal);
    std::signal(SIGINT, handle_signal);

    std::jthread down_thread{[&server]() {
      g_stop_flag.wait(false);
      server.Shutdown();
    }};

    server.Wait();

  } catch (const std::exception& e) {
    AXY_LOG_ERROR(e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
