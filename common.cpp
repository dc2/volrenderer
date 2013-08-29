#include "common.h"

void print_binary(uint8_t a)
{
    bitset<sizeof(a)*8> x(a);

    cout << x << endl;
}

void print_binary(unsigned a)
{
    bitset<sizeof(a)*8> x(a);

    cout << x << endl;
}


void StopWatch::start()
{
    mStart = now();
}

double StopWatch::stop()
{
    return (chrono::duration_cast<chrono::duration<double>>(now() - mStart)).count();
}

chrono::steady_clock::time_point StopWatch::now()
{
    return chrono::steady_clock::now();
}
