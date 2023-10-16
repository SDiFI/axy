cmake_minimum_required(VERSION 3.24)

include(FetchContent)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        10.1.1
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
  CLI11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG        v2.3.2
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(CLI11)

FetchContent_Declare(
  redis++
  GIT_REPOSITORY https://github.com/sewenew/redis-plus-plus.git
  GIT_TAG        1.3.10
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(redis++)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.2
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/libcpr/cpr.git
  GIT_TAG        1.10.4
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(cpr)

set(SPDLOG_FMT_EXTERNAL ON)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.12.0
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(spdlog)
