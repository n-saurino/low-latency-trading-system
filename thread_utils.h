#pragma once
#include <iostream>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <sys/syscall.h>

// Variadic template feature: allows you to create an object with a variable number of types and arguments
template<typename T, typename... A>
inline auto CreateAndStartThread(int core_id, const std::string &name, T &&func, A &&... args) noexcept{
    // initialize atomic boolean variables so no race conditions can occur between threads
    std::atomic<bool> running(false), failed(false);
    // lambda function to try to set thread core affinity (specify a cpu core that a thread runs on)
    auto thread_body = [&]{
        if(core_id >= 0 && !SetThreadCore(core_id)){
            std::cerr << "Failed to set core affinity for " << name << " " << pthread_self() << " to " << core_id << std::endl;
            failed = true;
            return;
        }
        std::cout << "Set core affinity for " << name << " " << pthread_self() << " to " << core_id << std::endl;
        running = true;
         // uses generic programming to indicate that a function "func" will be called
        // with an argument list "args"
        std::forward<T>(func)((std::forward<A>(args))...);
    };
    auto t = new std::thread(thread_body);
    while(!running && !failed){
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
    }
    if(failed){
        // have main thread wait for for child thread to complete execution 
        t->join();
        delete t;
        t = nullptr;
    }
    return t;
}