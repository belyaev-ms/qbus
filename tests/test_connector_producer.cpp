#include "qbus/connector.h"
#include <iostream>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

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
            pconnector = connector::make<multi_bidirectional_connector_type>(name);
            break;
        case 2:
            pconnector = connector::make<connector::safe_connector<
                connector::bidirectional_connector<connector::multi_bidirectional_connector_type>,
                connector::sharable_locker_with_sharable_pop_interface> >(name);
            break;
        case 3:
            pconnector = connector::make<multi_output_connector_type>(name);
            break;
        case 4:
            pconnector = connector::make<connector::safe_connector<
                connector::bidirectional_connector<connector::multi_output_connector_type>,
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
#endif
    if (pconnector->create(0, 32 * 512))
    {
#ifdef QBUS_IPC_TEST_GNUPLOT
        ti.second = get_monotonic_time();
        intervals.push_back(ti);
#endif
        boost::interprocess::named_mutex mutex_producer(boost::interprocess::open_only, "test_producer");
        boost::interprocess::named_mutex mutex_consumer(boost::interprocess::open_only, "test_consumer");
        struct timespec timeout = { 0, 0 };
        timeout.tv_sec = 5;
#ifndef QBUS_IPC_TEST_GNUPLOT
        struct timespec ts = { 0, 0 };
#endif
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex_producer);
#ifndef QBUS_IPC_TEST_GNUPLOT
            ts = get_monotonic_time();
            std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: create " << argc << std::endl;
#endif
        }
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex_consumer);
        }
#ifndef QBUS_IPC_TEST_GNUPLOT
        struct timespec dt = get_monotonic_time();
#endif
        for (size_t i = 0; i < 1024; ++i)
        {
            const std::string s = "the test message " + boost::lexical_cast<std::string>(i);
#ifdef QBUS_IPC_TEST_GNUPLOT
            ti.level = 75;
            ti.first = get_monotonic_time();
#else
            ts = get_monotonic_time();
            std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: " << s << std::endl;
#endif
            if (!pconnector->push(1, &s[0], s.size() + 1, timeout))
            {
#ifndef QBUS_IPC_TEST_GNUPLOT
                ts = get_monotonic_time();
                std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: break" << std::endl;
#endif
                boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex_producer);
                return -1;
            }
#ifdef QBUS_IPC_TEST_GNUPLOT
            ti.second = get_monotonic_time();
            intervals.push_back(ti);
            ti.level = 50;
            ti.first = ti.second;
#endif
            // !!! for option 5, the 'get' operation is required to clear service messages
            if (option < 3 || option == 5 && pconnector->get())
            {
                if (!pconnector->pop(timeout))
                {
#ifndef QBUS_IPC_TEST_GNUPLOT
                    ts = get_monotonic_time();
                    std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: abort" << std::endl;
#endif
                    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex_producer);
                    return -1;
                }
            }
#ifdef QBUS_IPC_TEST_GNUPLOT
            ti.second = get_monotonic_time();
            intervals.push_back(ti);
#endif
        }
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex_producer);
#ifndef QBUS_IPC_TEST_GNUPLOT
            ts = get_monotonic_time();
            dt = ts - dt;
            std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: close " 
                << dt.tv_sec << "." << dt.tv_nsec << std::endl;
#endif
        }
    }
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
#endif
    return 0;
}