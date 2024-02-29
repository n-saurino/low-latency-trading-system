#include "logging.h"

int main(int, char**){
    /*
    std::cout << "C++ standard version: " << __cplusplus << std::endl;
    #if __cplusplus == 202002L
    std::cout << "C++20\n";
    #elif __cplusplus == 201703L
    std::cout << "C++17\n";
    #elif __cplusplus == 201402L
        std::cout << "C++14\n";
    #elif __cplusplus == 201103L
        std::cout << "C++11\n";
    #elif __cplusplus == 199711L
        std::cout << "C++98\n";
    #else
        std::cout << "pre-standard C++\n";
    #endif
    */
    using namespace Common;

    char c = 'd';
    int i = 3;
    unsigned long ul = 65;
    float f = 3.4;
    double d = 34.56;
    const char* s = "test C-string";
    
    std::string ss = "test string";
    
    Logger logger("logging_example.log");
    logger.Log("Logging a char:% an int:% and an unsigned:%\n", c, i, ul);
    logger.Log("Logging a C-string:'%'\n", s);
    logger.Log("Logging a string:'%'\n", ss);

    return 0;
}