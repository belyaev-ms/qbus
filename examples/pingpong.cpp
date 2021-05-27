#include "qbus/connector.h"
#include <vector>
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#define SDEBUG(msg) //do { std::cout << msg; } while (0);

using namespace qbus;

/**
 * Get monotonic time
 * @return monotonic time
 */
size_t time_us()
{
#if __APPLE__
    return mach_absolute_time();
#else
    struct timespec res = {0};
	clock_gettime(CLOCK_MONOTONIC, &res);
    return res.tv_sec * 1000000 + res.tv_nsec / 1000;
#endif
}

const std::vector<char> MESSAGE { 'P', 'I', 'N', 'G', '\0' };

bool qbus_client(const std::string& name)
{
    const struct timespec ts = { 10, 0 };
    SDEBUG("qbus::client ");
    pconnector_type pconnector = connector::make<single_output_connector_type>(name);
    if (pconnector->open())
    {
        SDEBUG("open" << std::endl);
        size_t t = time_us();
        for (int i = 0; i < 1024; ++i)
        {
            pconnector->push(0, &MESSAGE[0], MESSAGE.size(), ts);
            SDEBUG(":> " << i << "\t" << &MESSAGE[0] << std::endl);
        }
        size_t dt = time_us() - t;
        std::cout << "dt = " << dt << std::endl;
        return true;
    }
    SDEBUG("fail" << std::endl);
    return false;
}

bool qbus_server(const std::string& name)
{
    pconnector_type pconnector = connector::make<single_input_connector_type>(name.c_str());
    const struct timespec ts = { 600, 0 };
    SDEBUG("qbus::server ");
    if (pconnector->create(0, 1024))
    {
        SDEBUG("open" << std::endl);
        size_t t = 0;
        for (int i = 0; i < 1024; ++i)
        {
            SDEBUG(":> " << i << "\t");
            pmessage_type pmessage = pconnector->get(ts);
            if (!pmessage)
            {
                break;
            }
            const size_t size = pmessage->data_size();
            if (size > 0)
            {
                if (0 == t)
                {
                    t = time_us();
                }
                std::vector<char> s(size);
                pmessage->unpack(&s[0]);
                SDEBUG(&s[0]);
            }
            pconnector->pop(ts);
            SDEBUG(std::endl);
        }
        size_t dt = time_us() - t;
        std::cout << "dt = " << dt << std::endl;
        return true;
    }
    SDEBUG("fail" << std::endl);
    return false;
}

bool unix_client(const std::string& name)
{
    SDEBUG("unix::client ");
    struct sockaddr_un unix_addr;
    memset(&unix_addr, 0, sizeof(struct sockaddr_un));
    unix_addr.sun_family = AF_UNIX;
    strcpy(unix_addr.sun_path, name.c_str());
    int sd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sd != -1 && connect(sd, (struct sockaddr *)&unix_addr, SUN_LEN(&unix_addr)) != -1)
    {
        SDEBUG("open" << std::endl);
        size_t t = time_us();
        for (int i = 0; i < 1024; ++i)
        {
            if (send(sd, &MESSAGE[0], MESSAGE.size(), 0) == -1)
            {
                close(sd);
                return false;
            }
            SDEBUG(":> " << i << "\t" << &MESSAGE[0] << std::endl);
        }
        size_t dt = time_us() - t;
        std::cout << "dt = " << dt << std::endl;
        close(sd);
        return true;
    }
    SDEBUG("fail" << std::endl);
    return false;
}

bool unix_server(const std::string& name)
{
    SDEBUG("unix::server ");
    struct sockaddr_un unix_addr;
    memset(&unix_addr, 0, sizeof(struct sockaddr_un));
    unix_addr.sun_family = AF_UNIX;
    strcpy(unix_addr.sun_path, name.c_str());
    unlink(unix_addr.sun_path);
    int sd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sd != -1 && bind(sd, (struct sockaddr *)&unix_addr, SUN_LEN(&unix_addr)) != -1 &&
        listen(sd, 10) != -1)
    {
        int csd = accept(sd, NULL, NULL);
        int length = MESSAGE.size();
        if (csd != -1 && setsockopt(csd, SOL_SOCKET, SO_RCVLOWAT, (char *)&length, sizeof(length)) != -1)
        {
            SDEBUG("open" << std::endl);
            size_t t = time_us();
            for (int i = 0; i < 1024; ++i)
            {
                SDEBUG(":> " << i << "\t");
                std::vector<char> buffer(MESSAGE.size());
                if (recv(csd, &buffer[0], buffer.size(), 0) == -1)
                {
                    close(csd);
                    close(sd);
                    return false;
                }
                SDEBUG(&buffer[0] << std::endl);
            }
            size_t dt = time_us() - t;
            std::cout << "dt = " << dt << std::endl;
            close(csd);
        }
        close(sd);
        return true;
    }
    SDEBUG("fail" << std::endl);
    return false;
}

int main(int argc, char** argv)
{
    std::string name = "qbus_test";
    bool is_client = false;
    bool is_qbus = true;
    if (argc > 1)
    {
        const char *options = "cun:";
        int opt;
        while ((opt = getopt(argc, argv, options)) != -1)
        {
            switch (opt)
            {
                case 'c':
                    is_client = true;
                    break;
                case 'u':
                    is_qbus = false;
                    break;
                case 'n':
                    name = optarg;
                    break;
            }
        }
    }
    
    if (is_qbus)
    {
        if (is_client)
        {
            qbus_client(name);
        }
        else
        {
            qbus_server(name);
        }
    }
    else
    {
        if (is_client)
        {
            unix_client(name);
        }
        else
        {
            unix_server(name);
        }
    }
    
    return 0;
}


