#include "qbus/connector.h"
#include <vector>
#include <iostream>

using namespace qbus;

int main(int argc, char** argv)
{
    const char *name = argc > 1 ? argv[1] : "test";
    pconnector_type pconnector = connector::make<in_multi_connector_type>(name);
    if (pconnector->open())
    {
        struct timespec timeout = { 0, 0 };
        timeout.tv_sec = 1;
        while (true)
        {
            pmessage_type pmessage = pconnector->get(timeout);
            std::cout << name << ":> ";
            if (pmessage)
            {
                const size_t size = pmessage->data_size();
                if (size > 0)
                {
                    std::vector<char> s(size);
                    pmessage->unpack(&s[0]);
                    std::cout << &s[0];
                }
                pconnector->pop(timeout);
            }
            std::cout << std::endl;
        }
    }
    return 0;
}

