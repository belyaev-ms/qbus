#include "dmux/connector.h"
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

using namespace dmux;

int main(int argc, char** argv)
{
    const char *name = argc > 1 ? argv[1] : "test";
    pconnector_type pconnector = connector::make<out_multiout_connector_type>(name);
    if (pconnector->create(512, 32))
    {
        boost::interprocess::named_mutex mutex(boost::interprocess::open_only, "test_server");
        struct timespec timeout = { 0, 0 };
        timeout.tv_sec = 5;
        struct timespec ts = { 0, 0 };
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);
            ts = get_monotonic_time();
            std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: create" << std::endl;
        }
        for (size_t i = 0; i < 1024; ++i)
        {
            const std::string s = "the test message " + boost::lexical_cast<std::string>(i);
            ts = get_monotonic_time();
            std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: " << s << std::endl;
            if (!pconnector->add(&s[0], s.size() + 1, timeout))
            {
                ts = get_monotonic_time();
                std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: break" << std::endl;
                boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);
                return -1;
            }
        }
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);
            ts = get_monotonic_time();
            std::cout << ts.tv_sec << "." << ts.tv_nsec << "\t: close" << std::endl;
        }
    }
    return 0;
}