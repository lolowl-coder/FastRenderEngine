#include "FileSystem/FileSystem.hpp"
#include "Log.hpp"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

std::vector<std::shared_ptr<spdlog::logger>> Log::mLoggers;

void Log::initialize(bool logToConsole, bool logToFile)
{
    const auto level = spdlog::level::trace;
    if(logToConsole)
    {
        mLoggers.push_back(spdlog::stdout_color_mt("console"));
        mLoggers.back()->set_level(level);
    }
    if(logToFile)
    {
        //Max size - 5 Mb, count of logs - 3
        FS;
        mLoggers.push_back(spdlog::rotating_logger_mt("file", fs.getDocumentsDir() + "/log.txt", 1048576 * 5, 3));
        mLoggers.back()->set_level(level);
    }
    spdlog::set_pattern("%^[%H:%M:%S %z] [%L]%$ %v");\
}