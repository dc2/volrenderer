#ifndef COMMON_H
#define COMMON_H

#include <bitset>
#include <iostream>
#include <chrono>

#include <cstdint>

#include <QDebug>

#ifdef __WIN32
#include "windows_compat.h"
#endif

#include "Formats/DDSLoader.h"
#include "Formats/RawLoader.h"

#ifdef USE_DICOM
#include "Formats/DicomLoader.h"
#endif


using namespace std;

template<char... digits>
struct conv2bin;

template<char high, char... digits>
struct conv2bin<high, digits...> {
    static_assert(high == '0' || high == '1', "no bin num!");
    static int const value = (high - '0') * (1 << sizeof...(digits)) +
                             conv2bin<digits...>::value;
};

template<char high>
struct conv2bin<high> {
    static_assert(high == '0' || high == '1', "no bin num!");
    static int const value = (high - '0');
};


template<char... digits>
constexpr int operator "" _b() {
    return conv2bin<digits...>::value;
}

void print_binary(uint8_t a);
void print_binary(unsigned a);

class StopWatch {
public:
    void start();
    double stop();
    
private:
    chrono::steady_clock::time_point now();
    chrono::steady_clock::time_point mStart;
};

#endif // COMMON_H
