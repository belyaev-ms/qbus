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
    SDEBUG("qbus::client ");
    const struct timespec ts = { 10, 0 };
    const std::string name_in = name + "_in";
    const std::string name_out = name + "_out";
    pconnector_type pconnector_out = connector::make<single_output_connector_type>(name_out);
    pconnector_type pconnector_in = connector::make<single_input_connector_type>(name_in);
    if (pconnector_out->open() && pconnector_in->open())
    {
        SDEBUG("open" << std::endl);
        size_t t = time_us();
        for (int i = 0; i < 1024; ++i)
        {
            pconnector_out->push(0, &MESSAGE[0], MESSAGE.size(), ts);
            SDEBUG(":> " << i << "\t" << &MESSAGE[0] << std::endl);
            pmessage_type pmessage = pconnector_in->get(ts);
            if (!pmessage)
            {
                return false;
            }
            const size_t size = pmessage->data_size();
            if (0 == size)
            {
                return false;
            }
            if (0 == t)
            {
                t = time_us();
            }
            std::vector<char> buffer(size);
            pmessage->unpack(&buffer[0]);
            pconnector_in->pop(ts);
            SDEBUG(":< " << i << "\t" << &buffer[0] << std::endl);
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
    SDEBUG("qbus::server ");
    const struct timespec ts = { 10, 0 };
    const std::string name_in = name + "_in";
    const std::string name_out = name + "_out";
    pconnector_type pconnector_out = connector::make<single_output_connector_type>(name_in);
    pconnector_type pconnector_in = connector::make<single_input_connector_type>(name_out);
    if (pconnector_in->create(0, 1024) && pconnector_out->create(0, 1024))
    {
        SDEBUG("open" << std::endl);
        size_t t = 0;
        for (int i = 0; i < 1024; ++i)
        {
            pmessage_type pmessage = pconnector_in->get(ts);
            if (!pmessage)
            {
                return false;
            }
            const size_t size = pmessage->data_size();
            if (0 == size)
            {
                return false;
            }
            if (0 == t)
            {
                t = time_us();
            }
            std::vector<char> buffer(size);
            pmessage->unpack(&buffer[0]);
            pconnector_in->pop(ts);
            SDEBUG(":< " << i << "\t" << &buffer[0] << std::endl);
            pconnector_out->push(0, &buffer[0], MESSAGE.size(), ts);
            SDEBUG(":> " << i << "\t" << &buffer[0] << std::endl);
        }
        size_t dt = time_us() - t;
        std::cout << "dt = " << dt << std::endl;
        return true;
    }
    SDEBUG("fail" << std::endl);
    return false;
}

bool unix_socket_client(const std::string& name)
{
    SDEBUG("unix::client ");
    struct sockaddr_un unix_addr;
    memset(&unix_addr, 0, sizeof(struct sockaddr_un));
    unix_addr.sun_family = AF_UNIX;
    strcpy(unix_addr.sun_path, name.c_str());
    int sd = socket(AF_UNIX, SOCK_STREAM, 0);
    int length = MESSAGE.size();
    if (sd != -1 && connect(sd, (struct sockaddr *)&unix_addr, SUN_LEN(&unix_addr)) != -1 &&
        setsockopt(sd, SOL_SOCKET, SO_RCVLOWAT, (char *)&length, sizeof(length)) != -1)
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
            std::vector<char> buffer(MESSAGE.size());
            if (recv(sd, &buffer[0], buffer.size(), 0) == -1)
            {
                close(sd);
                return false;
            }
            SDEBUG(":< " << i << "\t" << &buffer[0] << std::endl);
        }
        size_t dt = time_us() - t;
        std::cout << "dt = " << dt << std::endl;
        close(sd);
        return true;
    }
    SDEBUG("fail" << std::endl);
    return false;
}

bool unix_socket_server(const std::string& name)
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
                std::vector<char> buffer(MESSAGE.size());
                if (recv(csd, &buffer[0], buffer.size(), 0) == -1)
                {
                    close(csd);
                    close(sd);
                    return false;
                }
                SDEBUG(":< " << i << "\t" << &buffer[0] << std::endl);
                if (send(csd, &buffer[0], buffer.size(), 0) == -1)
                {
                    close(csd);
                    close(sd);
                    return false;
                }
                SDEBUG(":> " << i << "\t" << &buffer[0] << std::endl);
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
    enum service_type
    {
        SERVICE_SERVER,
        SERVICE_CLIENT,
        SERVICE_COUNT
    };
    enum transport_type
    {
        TRANSPORT_QBUS,
        TRANSPORT_UNIX_SOCKET,
        TRANSPORT_COUNT
    };
    service_type service = SERVICE_SERVER;
    transport_type transport = TRANSPORT_QBUS;
    if (argc > 1)
    {
        const char *options = "cun:";
        int opt;
        while ((opt = getopt(argc, argv, options)) != -1)
        {
            switch (opt)
            {
                case 'c':
                    service = SERVICE_CLIENT;
                    break;
                case 'u':
                    transport = TRANSPORT_UNIX_SOCKET;
                    break;
                case 'n':
                    name = optarg;
                    break;
            }
        }
    }
    
    std::function<bool(const std::string)> handlers[TRANSPORT_COUNT][SERVICE_COUNT] =
    {
        { qbus_server, qbus_client },
        { unix_socket_server, unix_socket_client }
    };
    
    handlers[transport][service](name);
    
    return 0;
}


