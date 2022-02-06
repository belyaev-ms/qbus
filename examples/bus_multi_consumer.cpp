#include "qbus/bus.h"
#include <vector>
#include <iostream>

using namespace qbus;

int main(int argc, char** argv)
{
    const char *name = argc > 1 ? argv[1] : "test";
    pbus_type pbus = bus::make<multi_input_bus_type>(name);
    if (pbus->open())
    {
        struct timespec timeout = { 0, 0 };
        timeout.tv_sec = 1;
        while (true)
        {
            pmessage_type pmessage = pbus->get(timeout);
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
                pbus->pop(timeout);
            }
            std::cout << std::endl;
        }
    }
    return 0;
}

