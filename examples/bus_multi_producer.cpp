#include "qbus/bus.h"
#include <iostream>

using namespace qbus;

int main(int argc, char** argv)
{
    const char *name = argc > 1 ? argv[1] : "test";
    pbus_type pbus = bus::make<multi_bidirectional_bus_type>(name);
    bus::specification_type spec;
    spec.id = 1;
    spec.keepalive_timeout = 0;
    spec.min_capacity = 64;
    spec.max_capacity = 8 * 512;
    spec.capacity_factor = 10;
    if (pbus->create(spec))
    {
        struct timespec timeout = { 0, 0 };
        timeout.tv_sec = 1;
        while (true)
        {
            std::cout << name << ":> ";
            std::string s;
            std::getline(std::cin, s);
            if ("q" == s || "quit" == s || "exit" == s)
            {
                break;
            }
            pbus->push(0, &s[0], s.size() + 1, timeout);
            pbus->pop();
        }
    }
    return 0;
}

