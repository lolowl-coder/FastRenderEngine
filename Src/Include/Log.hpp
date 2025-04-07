#pragma once

#include <spdlog/spdlog.h>

#define LOG_TRACE(...) for(auto& l : Log::mLoggers) l->trace(__VA_ARGS__)
#define LOG_DEBUG(...) for(auto& l : Log::mLoggers) l->debug(__VA_ARGS__)
#define LOG_INFO(...) for(auto& l : Log::mLoggers) l->info(__VA_ARGS__)
#define LOG_WARNING(...) for(auto& l : Log::mLoggers) l->warn(__VA_ARGS__)
#define LOG_ERROR(...) for(auto& l : Log::mLoggers)\
{\
    spdlog::set_pattern("%^[%H:%M:%S %z] [%L]%$ [%!:%#] %v");\
    SPDLOG_LOGGER_ERROR(l, __VA_ARGS__);\
    spdlog::set_pattern("%^[%H:%M:%S %z] [%L]%$ %v");\
}

struct Log
{
    static void initialize(bool logToConsole, bool logToFile);
    static void shutdown() { LOG_INFO("shutdown"); spdlog::shutdown(); }
    static std::vector<std::shared_ptr<spdlog::logger>> mLoggers;
};