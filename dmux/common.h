#ifndef COMMON_H
#define COMMON_H

#include <time.h>

namespace dmux
{

const bool operator==(const struct timespec& ts1, const struct timespec& ts2);
const bool operator!=(const struct timespec& ts1, const struct timespec& ts2);
const bool operator<(const struct timespec& ts1, const struct timespec& ts2);
const bool operator>(const struct timespec& ts1, const struct timespec& ts2);
const bool operator<=(const struct timespec& ts1, const struct timespec& ts2);
const bool operator>=(const struct timespec& ts1, const struct timespec& ts2);
const struct timespec operator+(const struct timespec& ts1, const struct timespec& ts2);
const struct timespec operator-(const struct timespec& ts1, const struct timespec& ts2);
const struct timespec get_monotonic_time(); ///< get the current monotonic time

} //namespace dmux


#endif /* COMMON_H */

