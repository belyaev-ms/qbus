#include "qbus/connector.h"
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

using namespace qbus;

int main(int argc, char** argv)
{
    const char *name = "test";
    pconnector_type pconnector;
    const size_t option = boost::lexical_cast<size_t>(argv[1]);
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
    
    if (pconnector->create(0, 32 * 512))
    {
        boost::interprocess::named_mutex mutex_producer(boost::interprocess::open_only, "test_producer");
        boost::interprocess::named_mutex mutex_consumer(boost::interprocess::open_only, "test_consumer");
        struct timespec timeout = { 0, 0 };
        timeout.tv_sec = 5;
        struct timespec ts = { 0, 0 };
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex_producer);
            ts = get_monotonic_time();
            std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: create " << argc << std::endl;
        }
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex_consumer);
        }
        struct timespec dt = get_monotonic_time();
        for (size_t i = 0; i < 1024; ++i)
        {
            const std::string s = "the test message " + boost::lexical_cast<std::string>(i);
            ts = get_monotonic_time();
            std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: " << s << std::endl;
            if (!pconnector->push(1, &s[0], s.size() + 1, timeout))
            {
                ts = get_monotonic_time();
                std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: break" << std::endl;
                boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex_producer);
                return -1;
            }
            // !!! for option 5, the 'get' operation is required to clear service messages
            if (option < 3 || option == 5 && pconnector->get())
            {
                if (!pconnector->pop(timeout))
                {
                    ts = get_monotonic_time();
                    std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: abort" << std::endl;
                    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex_producer);
                    return -1;
                }
            }
        }
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex_producer);
            ts = get_monotonic_time();
            dt = ts - dt;
            std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: close " 
                << dt.tv_sec << "." << dt.tv_nsec << std::endl;
            std::cerr << "wr dt = " << dt.tv_sec << "." << dt.tv_nsec << std::endl;
        }
    }
    return 0;
}