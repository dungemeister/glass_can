#pragma once
#include <string>
#include <iostream>
#include <sstream>
#include <mutex>
#include <chrono>
#include <iomanip>

namespace logging{


enum class LoggingLevel{
    DEBUG,
    INFO,
    WARNING,
    ERROR,
};

class Logger{
public:
    static Logger& getInstance(){
        static Logger instance;
        return instance;
    }
    Logger(const Logger& other) = delete;
    Logger& operator=(const Logger& other) = delete;

    void log(LoggingLevel log_lvl, const std::string& msg){
        std::unique_lock lock(m_mutex);
        std::cout <<"[" << getTimeStamp() << "][" << getLevelStringView(log_lvl) << "]: " << msg << std::endl;
    }
private:
    Logger() = default;
    std::mutex m_mutex;

    std::string getLevelStringView(LoggingLevel log_lvl){
        switch(log_lvl){
            case LoggingLevel::DEBUG:   return "DEBUG";
            case LoggingLevel::WARNING: return "WARNING";
            case LoggingLevel::ERROR:   return "ERROR";
            case LoggingLevel::INFO:
            default:                    return "INFO";

        }
    }

    std::string getTimeStamp(){
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ) % 1000;

        std::tm buf{};
        localtime_r(&timeT, &buf); // Thread-safe POSIX variant

        std::ostringstream oss;
        oss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S")
            << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();

    }
};

}//logging