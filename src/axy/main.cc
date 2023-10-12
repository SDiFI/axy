#include <fmt/format.h>

#include <CLI/CLI.hpp>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <string>

#include "internal_use_only/config.h"
#include "src/axy/server.h"
#include "src/axy/speech-service.h"

int main(int argc, char* argv[]) {
  try {
    CLI::App app{"Asynchronous/ASR proxy for SDiFI"};
    app.set_version_flag("--version", std::string{axy::project_version});

    axy::Server::Options server_opts;
    app.add_option("--listen-address", server_opts.listen_address)
        ->capture_default_str();
    app.add_option("--backend-speech-server-address",
                   server_opts.backend_speech_server_address)
        ->capture_default_str();
    app.add_option("--backend-speech-sever-use-tls",
                   server_opts.backend_speech_server_use_tls)
        ->capture_default_str();

    app.add_option("--redis-address", server_opts.redis_address)
        ->capture_default_str();

    CLI11_PARSE(app, argc, argv);

    axy::Server server{server_opts};

    fmt::println(stderr, "Server listening on {}", server_opts.listen_address);
    server.Wait();
  } catch (const std::exception& e) {
    fmt::print(stderr, "Error: {}\n", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
