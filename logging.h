#pragma once
#include <string>
#include <fstream>
#include <cstdio>
#include "types.h"
#include "macros.h"
#include "lf_queue.h"
#include "thread_utils.h"
#include "time_utils.h"

namespace Common{
constexpr size_t LOG_QUEUE_SIZE = 8 * 1024 * 1024;

enum class LogType: int8_t{
    CHAR = 0,
    INTEGER = 1,
    LONG_INTEGER = 2,
    LONG_LONG_INTEGER = 3,
    UNSIGNED_INTEGER = 4,
    UNSIGNED_LONG_INTEGER = 5,
    UNSIGNED_LONG_LONG_INTEGER = 6,
    FLOAT = 7,
    DOUBLE = 8
}

// struct that will be passed to the lock-free queue in order to log events by a non-critical path thread
stuct LogElement{
    LogType type_ = LogType::CHAR;
    union{
        char c;
        int i;
        long l;
        long long ll;
        unsigned int u;
        unsigned long ul;
        unsigned long long ull;
        float f;
        double d;
    } u_;
};

class Logger{
private:
    const std::string file_name_;
    std::ofstream file_;
    LFQueue<LogElement> queue_;
    std::atomic<bool> running_ = {true};
    std::thread* logger_thread_ = nullptr;

public:
    explicit Logger(const std::string& filename): file_name_(file_name), queue_(LOG_QUEUE_SIZE){
        file_.open(file_name_);
        ASSERT(file_.is_open(), "Could not open log file: " + file_name_);
        logger_thread_ = CreateAndStartThread(-1, "Common/Logger", [this](){FlushQueue();});
        ASSERT(logger_thread_ != nullptr, "Failed to start Logger thread.");
    }

    Logger() = delete;
    Logger(const Logger&) = delete;
    Logger(const Logger&&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(const Logger&&) = delete;

    ~Logger(){
        std::cerr << "Flushing and closing logger for: " << file_name_ << std::endl;
        while(queue_.Size()){
            using namespace std::literals::chrono_literals;
            std::this_thread::sleep_for(1s);
        }
        running_ = false;
        logger_thread_->join();
        file_.close();
    }

    auto FlushQueue() noexcept{
        // need to create a loop to consume every element on the queue, check it's log type, 
        // store the correct variable from the union and write that value to the file
        // then sleep for a millisecond when the lock-free queue is empty

        // keep looping
        while(running_){
            for(auto next = queue_.GetNextToRead(); queue_.Size() && next; next = queue_.GetNextToRead()){
                switch (next->type_){
                    case LogType::CHAR:
                        file_ << next->u_.c;
                        break;
                    
                    case LogType::INTEGER:
                        file_ << next->u_.i;
                        break;

                    case LogType::LONG_INTEGER:
                        file_ << next->u_.l;
                        break;

                    case LogType::LONG_LONG_INTEGER:
                        file_ << next->u_.ll;
                        break;

                    case LogType::UNSIGNED_INTEGER:
                        file << next->u_.u;
                        break;

                    case LogType::UNSIGNED_LONG_INTEGER:
                        file << next->u_.ul;
                        break;

                    case LogType::UNSIGNED_LONG_LONG_INTEGER:
                        file << next->u_.ull;
                        break;

                    case LogType::FLOAT:
                        file << next->u_.f;
                        break;

                    case LogType::DOUBLE:
                        file << next->u_.d;
                        break;

                    default:
                        break;
                }
                queue_.UpdateReadIndex();
                next = queue_.GetNextToRead();
            }
            using namespace std::literals::chrono_literals;
            std::this_thread::sleep_for(1ms);
        }
    }

    auto PushValue(const char* value) noexcept{
        while(*value){
            PushValue(*value);
            ++value;
        }
    }

    auto PushValue(const std::string& value) noexcept{
        PushValue(value.c_str());
    }

    auto PushValue(const int value) noexcept{
        PushValue(LogElement{LogType::INTEGER, {.i = value}});
    }

    auto PushValue(const long value) noexcept{
        PushValue(LogElement{LogType::LONG_INTEGER, {.l = value}});
    }

    auto PushValue(const long long value) noexcept{
        PushValue(LogElement{LogType::LONG_LONG_INTEGER, {.ll = value}});
    }

    auto PushValue(const unsigned value) noexcept{
        PushValue(LogElement{LogType::UNSIGNED_INTEGER, {.u = value}});
    }

    auto PushValue(const unsigned long value) noexcept{
        PushValue(LogElement{LogType::UNSIGNED_LONG_INTEGER, {.ul = value}});
    }

    auto PushValue(const unsigned long long value) noexcept{
        PushValue(LogElement{LogType::UNSIGNED_LONG_LONG_INTEGER, {.ull = value}});
    }

    auto PushValue(const float value) noexcept{
        PushValue(LogElement{LogType::FLOAT, {.f = value}});
    }
    
    auto PushValue(const double value) noexcept{
        PushValue(LogElement{LogType::DOUBLE, {.d = value}});
    }

    // function used by performant thread to push log to lock-free queue
    // Example: log("Integer:% String:% Double:%", int_val, str_val, dbl_val); 
    template< typename T, typename... A>
    auto log(const char* s, const T &value, A... args) noexcept{
        while(*s){
            if(*s == '%'){
                if(UNLIKELY(*(s+1) == '%')){
                    ++s;
                }else{
                    PushValue(value);
                    log(s+1, args);
                    return;
                }
            }
            PushValue(*s++);
        }
        FATAL("Extra arguments provided to log()");
    }
};

}