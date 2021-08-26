#include "qbus/connector.h"
#include <vector>
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
   
    if (pconnector->open())
    {
        boost::interprocess::named_mutex mutex(boost::interprocess::open_only, "test_consumer");
        struct timespec timeout = { 0, 0 };
        timeout.tv_sec = 1;
        struct timespec ts = { 0, 0 };
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);
            ts = get_monotonic_time();
            std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: open " << argc << std::endl;
        }
        while (true)
        {
            pmessage_type pmessage = pconnector->get(timeout);
            if (!pmessage)
            {
                ts = get_monotonic_time();
                std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: close" << std::endl;
                boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);
                return 0;
            }
            const size_t size = pmessage->data_size();
            if (size > 0)
            {
                std::vector<char> s(size);
                pmessage->unpack(&s[0]);
                ts = get_monotonic_time();
                std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: " << &s[0] << std::endl;
            }
            if (!pconnector->pop(timeout))
            {
                ts = get_monotonic_time();
                std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: abort" << std::endl;
                boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);
                return -1;
            }
        }
    }
    return 0;
}

