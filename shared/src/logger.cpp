



#include <viper/common/logger.h>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/pattern_formatter.h>

#include <filesystem>
#include <vector>
#include <mutex>

namespace viper {
namespace log {

namespace {

std::once_flag g_initFlag;
bool g_initialized = false;


std::shared_ptr<spdlog::logger> createLogger(
    const char* name,
    const std::vector<spdlog::sink_ptr>& sinks)
{
    auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
    logger->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] %v");
    logger->flush_on(spdlog::level::warn);
    return logger;
}

} 

void init(const std::string& logDir, spdlog::level::level_enum level) {
    std::call_once(g_initFlag, [&]() {
        try {
            
            std::filesystem::create_directories(logDir);

            
            std::vector<spdlog::sink_ptr> sinks;

            
            auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            consoleSink->set_level(level);
            sinks.push_back(consoleSink);

            
            auto filePath = (std::filesystem::path(logDir) / "viper.log").string();
            auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                filePath, 5 * 1024 * 1024, 3);
            fileSink->set_level(spdlog::level::debug); 
            sinks.push_back(fileSink);

            
            const char* loggerNames[] = {
                LOGGER_BOOT,
                LOGGER_RUNTIME,
                LOGGER_JNI,
                LOGGER_MC,
                LOGGER_IPC,
                LOGGER_CLIENT,
            };

            for (const auto* name : loggerNames) {
                auto logger = createLogger(name, sinks);
                logger->set_level(level);
                spdlog::register_logger(logger);
            }

            
            spdlog::set_default_logger(spdlog::get(LOGGER_RUNTIME));
            spdlog::set_level(level);

            g_initialized = true;

        } catch (const spdlog::spdlog_ex& ex) {
            
            fprintf(stderr, "[VIPER] FATAL: Logger initialization failed: %s\n", ex.what());
        }
    });
}

void shutdown() {
    if (g_initialized) {
        spdlog::shutdown();
        g_initialized = false;
    }
}

std::shared_ptr<spdlog::logger> get(const char* name) {
    auto logger = spdlog::get(name);
    if (!logger) {
        
        logger = spdlog::default_logger();
    }
    return logger;
}

} 
} 
