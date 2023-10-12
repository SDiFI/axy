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

# redis++ uses find_library and find_path to locate hiredis instead of
# find_package so we need a few hacks to make this work, i.e. set SOURCE_DIR,
# use include_directories and disable tests in redis++
FetchContent_Declare(
  hiredis
  GIT_REPOSITORY https://github.com/redis/hiredis.git
  GIT_TAG        v1.2.0
  GIT_SHALLOW    TRUE
  OVERRIDE_FIND_PACKAGE
  SOURCE_DIR _deps/hiredis
)

FetchContent_MakeAvailable(hiredis)

# Set include directory so that redis++ can find hiredis
include_directories(${CMAKE_BINARY_DIR}/_deps)

set(REDIS_PLUS_PLUS_BUILD_TEST OFF)

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

FetchContent_Declare(
  readerwriterqueue
  GIT_REPOSITORY https://github.com/cameron314/readerwriterqueue.git
  GIT_TAG        2dee33ae3edd1e454ac34fea0a27017613355eff
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(readerwriterqueue)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
