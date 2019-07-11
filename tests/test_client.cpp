#include "dmux/connector.h"
#include <vector>
#include <iostream>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

using namespace dmux;

int main(int argc, char** argv)
{
    const char *name = argc > 1 ? argv[1] : "test";
    pconnector_type pconnector = connector::make<in_multiout_connector_type>(name);
    if (pconnector->open())
    {
        boost::interprocess::named_mutex mutex(boost::interprocess::open_only, "test_client");
        struct timespec timeout = { 0, 0 };
        timeout.tv_sec = 1;
        struct timespec ts = { 0, 0 };
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);
            ts = get_monotonic_time();
            std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: open" << std::endl;
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
            pconnector->pop(timeout);
        }
    }
    return 0;
}

