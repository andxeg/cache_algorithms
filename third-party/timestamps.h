#include <ctime>
#include <sstream>
#include <iostream>


template <typename T>
std::string ToString(T val)
{
    std::stringstream stream;
    stream << val;
    return stream.str();
}


struct tm * print_current_data_and_time(const std::string & message) {
    time_t t = time(0);
    struct tm * now = localtime(&t);
    std::string result = ToString<int>(now->tm_year + 1900) + std::string("-") +
                         ToString<int>(now->tm_mon + 1) + std::string("-") +
                         ToString<int>(now->tm_mday ) + std::string(".") +
                         ToString<int>(now->tm_hour ) + std::string(":") +
                         ToString<int>(now->tm_min ) + std::string(":") +
                         ToString<int>(now->tm_sec );
    std::cout << message << " Current time -> " << result << std::endl;

    return now;
}
