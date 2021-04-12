#ifndef QBUS_COMMON_H
#define QBUS_COMMON_H

#include <time.h>

namespace qbus
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

} //namespace qbus

#endif /* QBUS_COMMON_H */

