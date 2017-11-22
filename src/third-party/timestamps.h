#pragma once

#include <ctime>
#include <sstream>
#include <iomanip>
#include <iostream>


int comparisonWithEpsilon(const size_t &first, const size_t &second, const size_t &epsilon) {
    if (abs(first - second) <= epsilon)
        return 0;

    if (first > second)
        return 1;
    else
        return -1;
}


template <typename T>
T convertFromStringTo(std::string str) {
    T val;
    std::stringstream stream(str);
    stream >> val;
    return val;
}


template <typename T>
std::string ToString(T val)
{
    std::stringstream stream;
    stream << val;
    return stream.str();
}


struct tm * print_current_data_and_time(const std::string &message) {
    time_t t = time(0);
    struct tm * now = localtime(&t);
    std::string str_time = ToString<int>(now->tm_year + 1900) + std::string("-") +
                         ToString<int>(now->tm_mon + 1) + std::string("-") +
                         ToString<int>(now->tm_mday ) + std::string(".") +
                         ToString<int>(now->tm_hour ) + std::string(":") +
                         ToString<int>(now->tm_min ) + std::string(":") +
                         ToString<int>(now->tm_sec );

    std::cout << std::setw(20) << str_time << " | " << message << std::endl;
    return now;
}
