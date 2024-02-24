#include "thread_utils.h"

/* Dummy example of multithreading and thread management/pinning a thread to a specific core */

auto DummyFunction(int a, int b, bool sleep){
    std::cout << "DummyFunction(" << a << "," << b << ")" << std::endl;
    std::cout << "DummyFunction output=" << a + b << std::endl;
    if(sleep){
        std::cout << "DummyFunction sleeping..." << std::endl;
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(5s);
    }
    std::cout << "DummyFunction done." << std::endl;
}

int main(int, char**){
    using namespace Common;
    auto t1 = CreateAndStartThread(-1, "DummyFunction1", DummyFunction, 12, 21, false);
    auto t2 = CreateAndStartThread(1, "DummyFunction2", DummyFunction, 15, 51, true);
    
    std::cout << "main waiting for threads to be done." << std::endl;
    t1->join();
    t2->join();
    std::cout << "main exiting." << std::endl;
    return 0;
}