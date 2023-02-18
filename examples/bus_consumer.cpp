#include "qbus/bus.h"
#include <vector>
#include <iostream>

using namespace qbus;

int main(int argc, char** argv)
{
    const char *name = argc > 1 ? argv[1] : "test";
    pbus_type pbus = bus::make<single_input_bus_type>(name);
    if (pbus->open())
    {
        while (true)
        {
            sleep(1);
            pmessage_type pmessage = pbus->get();
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
                pbus->pop();
            }
            std::cout << std::endl;
        }
    }
    return 0;
}

