#include "dmux/connector.h"
#include <vector>
#include <iostream>

using namespace dmux;

int main(int argc, char** argv)
{
    const char *name = argc > 1 ? argv[1] : "test";
    pconnector_type pconnector = connector::make<in_connector_type>(name);
    if (pconnector->open())
    {
        while (true)
        {
            sleep(1);
            pmessage_type pmessage = pconnector->get();
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
                pconnector->pop();
            }
            std::cout << std::endl;
        }
    }
    return 0;
}

