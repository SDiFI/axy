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

set(SPDLOG_FMT_EXTERNAL ON)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.12.0
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(spdlog)
