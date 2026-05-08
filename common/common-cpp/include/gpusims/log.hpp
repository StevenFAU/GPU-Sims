#pragma once

// Thin wrapper around spdlog so common-cpp consumers don't need to include
// spdlog headers directly. The default logger is created on first use.

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace gpusims {

// Initialize the default logger. Safe to call multiple times.
// Sets pattern to "[hh:mm:ss.SSS] [level] message".
void initLogger();

// Convenience wrappers that route to the default logger.
template <typename... Args>
inline void logTrace(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::trace(fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void logDebug(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::debug(fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void logInfo(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::info(fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void logWarn(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::warn(fmt, std::forward<Args>(args)...);
}
template <typename... Args>
inline void logError(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::error(fmt, std::forward<Args>(args)...);
}

}  // namespace gpusims
