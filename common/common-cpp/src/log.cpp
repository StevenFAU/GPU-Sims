#include <gpusims/log.hpp>

#include <atomic>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace gpusims {

namespace {
std::atomic<bool> g_logger_initialized{false};
}

void initLogger() {
    bool expected = false;
    if (!g_logger_initialized.compare_exchange_strong(expected, true)) {
        return;  // already initialized
    }

    auto console = spdlog::stdout_color_mt("gpusims");
    console->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
#ifndef NDEBUG
    console->set_level(spdlog::level::debug);
#else
    console->set_level(spdlog::level::info);
#endif
    spdlog::set_default_logger(console);
    spdlog::flush_on(spdlog::level::warn);
}

}  // namespace gpusims
