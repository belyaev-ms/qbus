#include "qbus/connector.h"
#include <iostream>
#include <vector>

using namespace qbus;

typedef connector::safe_connector<
    connector::bidirectional_connector<connector::simple_connector<queue::smart_shared_queue> >,
    connector::sharable_spinlocker_with_sharable_pop_interface> smart_connector_type;

int main(int argc, char** argv)
{
    const char *name = "test";
    pconnector_type pconnector = connector::make<smart_connector_type>(name);
    if (argc == 1 ? pconnector->create(0, 512) : pconnector->open())
    {
        struct timespec timeout = { 0, 0 };
        timeout.tv_sec = 0;
        timeout.tv_nsec = 1000000;
        std::string s;
        while (true)
        {
            std::cout << name << ":> ";
            std::string s;
            std::cin >> s;
            if ("q" == s || "quit" == s || "exit" == s)
            {
                break;
            }
            pconnector->push(7, &s[0], s.size() + 1, timeout);
            while (true)
            {
                pmessage_type pmessage = pconnector->get(timeout);
                if (!pmessage)
                {
                    break;
                }
                std::cout << name << ":< ";
                const size_t size = pmessage->data_size();
                if (size > 0)
                {
                    std::vector<char> s(size);
                    pmessage->unpack(&s[0]);
                    std::cout << &s[0];
                }
                pconnector->pop(timeout);
                std::cout << std::endl;
            }
        }
    }
    return 0;
}

