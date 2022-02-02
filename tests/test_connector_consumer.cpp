#include "qbus/connector.h"
#include <vector>
#include <iostream>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

//#define QBUS_IPC_TEST_GNUPLOT

using namespace qbus;

double to_double(const struct timespec& ts)
{
    return ts.tv_sec + 0.000000001 * ts.tv_nsec;
}

int main(int argc, char** argv)
{
#ifdef QBUS_IPC_TEST_GNUPLOT
    struct time_interval_type
    {
        size_t level;
        struct timespec first;
        struct timespec second;
    };
    typedef std::vector<time_interval_type> time_interval_list;
    time_interval_list intervals;
    intervals.reserve(4096);
#endif
    const char *name = "test";
    pconnector_type pconnector;
    const size_t option = argc == 1 ? 1 : boost::lexical_cast<size_t>(argv[1]);
#ifdef QBUS_IPC_TEST_GNUPLOT
    const size_t id = argc < 3 ? 0 : boost::lexical_cast<size_t>(argv[2]);
#endif
    switch (option)
    {
        case 1:
        case 3:
            pconnector = connector::make<multi_input_connector_type>(name);
            break;
        case 2:
        case 4:
            pconnector = connector::make<connector::safe_connector<
                connector::input_connector<connector::multi_bidirectional_connector_type>,
                connector::sharable_locker_with_sharable_pop_interface> >(name);
            break;
        case 5:
            pconnector = connector::make<connector::safe_connector<
                connector::bidirectional_connector<connector::simple_connector<queue::smart_shared_queue> >,
                connector::sharable_spinlocker_with_sharable_pop_interface> >(name);
            break;
    }
    assert(pconnector);
#ifdef QBUS_IPC_TEST_GNUPLOT
    time_interval_type ti;
    ti.level = 25;
    ti.first = get_monotonic_time();
    struct timespec dt_get_min = { std::numeric_limits<int>::max(), 0 };
    struct timespec dt_get_max = { 0, 0 };
    struct timespec dt_pop_min = { std::numeric_limits<int>::max(), 0 };
    struct timespec dt_pop_max = { 0, 0 };
#endif
    if (pconnector->open())
    {
#ifdef QBUS_IPC_TEST_GNUPLOT
        ti.second = get_monotonic_time();
        intervals.push_back(ti);
#endif
        boost::interprocess::named_mutex mutex(boost::interprocess::open_only, "test_consumer");
        struct timespec timeout = { 0, 0 };
        timeout.tv_sec = 1;
#ifndef QBUS_IPC_TEST_GNUPLOT
        struct timespec ts = { 0, 0 };
#endif
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);
#ifndef QBUS_IPC_TEST_GNUPLOT
            ts = get_monotonic_time();
            std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: open " << argc << std::endl;
#endif
        }
#ifndef QBUS_IPC_TEST_GNUPLOT
        struct timespec dt = get_monotonic_time();
#endif
        while (true)
        {
#ifdef QBUS_IPC_TEST_GNUPLOT
            ti.level = 75;
            ti.first = get_monotonic_time();
#endif
            pmessage_type pmessage = pconnector->get(timeout);
            if (!pmessage)
            {
#ifndef QBUS_IPC_TEST_GNUPLOT
                ts = get_monotonic_time();
                dt = ts - dt - timeout;
                std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: close" << std::endl;
#endif
                boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);
#ifdef QBUS_IPC_TEST_GNUPLOT
                for (time_interval_list::const_iterator it = intervals.begin();
                    it != intervals.end(); ++it)
                {
                    const double level = it->level / 100.0;
                    const double t0 = to_double(it->first);
                    const double t1 = to_double(it->second);
                    std::cout << boost::format("%0.2f,\t%0.9f") % id % t0 << std::endl;
                    std::cout << boost::format("%0.2f,\t%0.9f") % (id + level) % t0 << std::endl;
                    std::cout << boost::format("%0.2f,\t%0.9f") % (id + level) % t1 << std::endl;
                    std::cout << boost::format("%0.2f,\t%0.9f") % id % t1 << std::endl;
                }
                std::cerr << boost::format("get = [ %0.9f,\t%0.9f ]")
                    % to_double(dt_get_min) % to_double(dt_get_max) << std::endl;
                std::cerr << boost::format("pop = [ %0.9f,\t%0.9f ]")
                    % to_double(dt_pop_min) % to_double(dt_pop_max) << std::endl;
#endif
                return 0;
            }
#ifdef QBUS_IPC_TEST_GNUPLOT
            ti.second = get_monotonic_time();
            intervals.push_back(ti);
            struct timespec dt_get = ti.second - ti.first;
            dt_get_min = dt_get < dt_get_min ? dt_get : dt_get_min;
            dt_get_max = dt_get > dt_get_max ? dt_get : dt_get_max;
            ti.level = 50;
            ti.first = ti.second;
#endif
            const size_t size = pmessage->data_size();
            if (size > 0)
            {
                std::vector<char> s(size);
                pmessage->unpack(&s[0]);
#ifndef QBUS_IPC_TEST_GNUPLOT
                ts = get_monotonic_time();
                std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: " << &s[0] << std::endl;
#endif
            }
            if (!pconnector->pop(timeout))
            {
#ifndef QBUS_IPC_TEST_GNUPLOT
                ts = get_monotonic_time();
                std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: abort" << std::endl;
#endif
                boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);
                return -1;
            }
#ifdef QBUS_IPC_TEST_GNUPLOT
            ti.second = get_monotonic_time();
            intervals.push_back(ti);
            struct timespec dt_pop = ti.second - ti.first;
            dt_pop_min = dt_pop < dt_pop_min ? dt_pop : dt_pop_min;
            dt_pop_max = dt_pop > dt_pop_max ? dt_pop : dt_pop_max;
#endif
        }
    }
    return 0;
}

