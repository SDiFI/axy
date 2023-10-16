#ifndef AXY_SRC_AXY_LOGGING_H_
#define AXY_SRC_AXY_LOGGING_H_

#include <spdlog/spdlog.h>

#include <string>

namespace axy {

#define AXY_LOG_WARN(...) spdlog::warn(__VA_ARGS__)
#define AXY_LOG_INFO(...) spdlog::info(__VA_ARGS__)
#define AXY_LOG_DEBUG(...) spdlog::debug(__VA_ARGS__)
#define AXY_LOG_TRACE(...) spdlog::trace(__VA_ARGS__)
#define AXY_LOG_ERROR(...) spdlog::error(__VA_ARGS__)

enum class LogLevel { kError, kWarn, kInfo, kDebug, kTrace };

void SetLogLevel(LogLevel level);

void SetLogLevel(const std::string& level);

/// This needs to be called after SetLogLevel.
void RegisterLibraryLogHandlers();

}  // namespace axy

#endif  // AXY_SRC_AXY_LOGGING_H_
