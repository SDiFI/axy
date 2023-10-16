#include <fmt/core.h>

#include <CLI/CLI.hpp>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <string>

#include "CLI/Validators.hpp"
#include "internal_use_only/config.h"
#include "src/axy/logging.h"
#include "src/axy/server.h"
#include "src/axy/speech-service.h"

int main(int argc, char* argv[]) {
  try {
    CLI::App app{"Asynchronous/ASR proxy for SDiFI"};
    app.option_defaults()->always_capture_default();
    app.set_version_flag("--version", std::string{axy::project_version});

    std::string log_level = "info";
    app.add_option("--log-level", log_level)
        ->check(CLI::IsMember({"trace", "debug", "info", "warn", "error"}));

    axy::Server::Options server_opts;
    app.add_option("--listen-address", server_opts.listen_address);
    app.add_option("--backend-speech-server-address",
                   server_opts.backend_speech_server_address);
    app.add_flag("--backend-speech-server-use-tls",
                 server_opts.backend_speech_server_use_tls);
    app.add_option("--redis-address", server_opts.redis_address);

    CLI11_PARSE(app, argc, argv);

    axy::SetLogLevel(log_level);
    axy::RegisterLibraryLogHandlers();

    axy::Server server{server_opts};

    AXY_LOG_INFO("Server listening on {}", server_opts.listen_address);
    server.Wait();
  } catch (const std::exception& e) {
    AXY_LOG_ERROR(e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
