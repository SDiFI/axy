#include "src/axy/logging.h"

#include <grpc/support/log.h>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

#include <stdexcept>

namespace axy {

void SetLogLevel(LogLevel level) {
  switch (level) {
    case LogLevel::kError:
      spdlog::set_level(spdlog::level::err);
      break;
    case LogLevel::kWarn:
      spdlog::set_level(spdlog::level::warn);
      gpr_set_log_verbosity(GPR_LOG_SEVERITY_ERROR);
      break;
    case LogLevel::kInfo:
      spdlog::set_level(spdlog::level::info);
      gpr_set_log_verbosity(GPR_LOG_SEVERITY_INFO);
      break;
    case LogLevel::kDebug:
      spdlog::set_level(spdlog::level::debug);
      gpr_set_log_verbosity(GPR_LOG_SEVERITY_DEBUG);
      break;
    case LogLevel::kTrace:
      spdlog::set_level(spdlog::level::trace);
      gpr_set_log_verbosity(GPR_LOG_SEVERITY_DEBUG);
      break;
  }
}

namespace {

LogLevel LogLevelFromStr(const std::string& level) {
  if (level == "trace") {
    return LogLevel::kTrace;
  } else if (level == "debug") {
    return LogLevel::kDebug;
  } else if (level == "info") {
    return LogLevel::kInfo;
  } else if (level == "warn") {
    return LogLevel::kWarn;
  } else if (level == "error") {
    return LogLevel::kError;
  } else {
    throw std::invalid_argument{"invalid log level: " + level};
  }
}

}  // namespace

void SetLogLevel(const std::string& level) {
  SetLogLevel(LogLevelFromStr(level));
}

void RegisterLibraryLogHandlers() {
  switch (spdlog::get_level()) {
    case spdlog::level::trace:
      gpr_set_log_verbosity(GPR_LOG_SEVERITY_DEBUG);
      break;
    case spdlog::level::debug:
      gpr_set_log_verbosity(GPR_LOG_SEVERITY_DEBUG);
      break;
    case spdlog::level::info:
      gpr_set_log_verbosity(GPR_LOG_SEVERITY_INFO);
      break;
    case spdlog::level::warn:
      gpr_set_log_verbosity(GPR_LOG_SEVERITY_ERROR);
      break;
    case spdlog::level::err:
      gpr_set_log_verbosity(GPR_LOG_SEVERITY_ERROR);
      break;
    case spdlog::level::critical:
      gpr_set_log_verbosity(GPR_LOG_SEVERITY_ERROR);
      break;
    case spdlog::level::n_levels:
      [[fallthrough]];
    case spdlog::level::off:
      return;
  }

  gpr_set_log_function([](gpr_log_func_args* args) -> void {
    switch (args->severity) {
      case GPR_LOG_SEVERITY_DEBUG:
        AXY_LOG_DEBUG("gRPC: {} ", args->message);
        break;
      case GPR_LOG_SEVERITY_INFO:
        AXY_LOG_INFO("gRPC: {} ", args->message);
        break;
      case GPR_LOG_SEVERITY_ERROR:
        AXY_LOG_ERROR("gRPC: {} ", args->message);
        break;
    }
  });
}

}  // namespace axy
