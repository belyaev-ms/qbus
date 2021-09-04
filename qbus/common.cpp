#include "qbus/common.h"
#include <boost/smart_ptr/detail/spinlock.hpp>

namespace qbus
{

bool operator==(const struct timespec& ts1, const struct timespec& ts2)
{
    return ts1.tv_sec == ts2.tv_sec && ts1.tv_nsec == ts2.tv_nsec;
}

bool operator!=(const struct timespec& ts1, const struct timespec& ts2)
{
    return !(ts1 == ts2);
}

bool operator<(const struct timespec& ts1, const struct timespec& ts2)
{
    return ts1.tv_sec < ts2.tv_sec || (ts1.tv_sec == ts2.tv_sec && ts1.tv_nsec < ts2.tv_nsec);
}

bool operator>(const struct timespec& ts1, const struct timespec& ts2)
{
    return ts1.tv_sec > ts2.tv_sec || (ts1.tv_sec == ts2.tv_sec && ts1.tv_nsec > ts2.tv_nsec);
}

bool operator<=(const struct timespec& ts1, const struct timespec& ts2)
{
    return !(ts1 > ts2);
}

bool operator>=(const struct timespec& ts1, const struct timespec& ts2)
{
    return !(ts1 < ts2);
}

struct timespec operator+(const struct timespec& ts1, const struct timespec& ts2)
{
    timespec result;
    result.tv_sec = ts1.tv_sec + ts2.tv_sec;
    result.tv_nsec = ts1.tv_nsec + ts2.tv_nsec;
    if (result.tv_nsec >= 1000000000)
    {
        ++result.tv_sec;
        result.tv_nsec -= 1000000000;
    }
    return result;
}

struct timespec operator-(const struct timespec& ts1, const struct timespec& ts2)
{
    timespec result;
    if (ts1.tv_nsec < ts2.tv_nsec)
    {
        result.tv_sec = ts1.tv_sec - ts2.tv_sec - 1;
        result.tv_nsec = 1000000000 + ts1.tv_nsec - ts2.tv_nsec;
    }
    else
    {
        result.tv_sec = ts1.tv_sec - ts2.tv_sec;
        result.tv_nsec = ts1.tv_nsec - ts2.tv_nsec;
    }
    return result;
}

/**
 * Get the current monotonic time
 * @return the current monotonic time
 */
struct timespec get_monotonic_time()
{
    struct timespec ts = {0, 0};
    while (clock_gettime(CLOCK_MONOTONIC, &ts) != 0);
    return ts;
}

} //namespace qbus
