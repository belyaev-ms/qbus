#include "qbus/connector.h"
#include <iostream>

using namespace qbus;

int main(int argc, char** argv)
{
    const char *name = argc > 1 ? argv[1] : "test";
    pconnector_type pconnector = connector::make<out_multiout_connector_type>(name);
    if (pconnector->create(512, 32))
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
            pconnector->add(&s[0], s.size() + 1, timeout);
        }
    }
    return 0;
}

