#include "qbus/common.h"
#include "qbus/connector.h"
#ifdef QBUS_ZMQ_ENABLED
#include <zmq.hpp>
#endif
#include <unistd.h>
#include <ctime>
#include <sstream>
#include <iostream>
#include <boost/lexical_cast.hpp>

using namespace qbus;

struct config_type
{
    bool verbose = false;
    bool forced = false;
    size_t count = 0;
    std::string engine;
    std::string address = "logger";
};

typedef int (*client_func)(const config_type& config);

struct client_list_type
{
    std::string name;
    client_func func;
};

class base_client
{
public:
    base_client(const config_type& config) :
        m_config(config)
    {
    }

protected:
    const config_type m_config;
};

#ifdef QBUS_ZMQ_ENABLED
class zmq_client : public base_client
{
public:
    zmq_client(const config_type& config ) :
        base_client(config),
        m_sender(m_context, zmq::socket_type::push)
    {
        const std::string address = "ipc://" + config.address;
        m_sender.connect(address);
    }

    ~zmq_client()
    {
        sleep(1);
    }

    void send(const std::string& record)
    {
        zmq::message_t message(record);
        while(!m_sender.send(message, zmq::send_flags::none));
    }

private:
    zmq::context_t m_context;
    zmq::socket_t m_sender;
};
#endif

template <typename Connector>
class qbus_client : public base_client
{
public:
    qbus_client(const config_type& config) :
        base_client(config),
        m_pconnector(connector::make<Connector>(config.address))
    {
        if (!m_pconnector->open())
        {
            throw 2;
        }
        m_timeout.tv_sec = 1;
        m_timeout.tv_nsec = 0;
    }

    void send(const std::string& record)
    {
        while(!m_pconnector->push(0, &record[0], record.size() + 1, m_timeout));
    }

private:
    pconnector_type m_pconnector;
    struct timespec m_timeout;
};

template <typename Client>
int base_client_cycle(const config_type& config)
{
    Client client(config);
    std::srand(std::time(NULL));
    const int pid = getpid();
    struct timespec period;
    period.tv_sec = 1;
    period.tv_nsec = 0;
    struct timespec ts_prev = qbus::get_monotonic_time();
    size_t id = 0;
    size_t msg_count = 0;
    size_t count = config.count;
    do
    {
        if (!config.forced)
        {
            size_t timeout = std::rand() % 1000000;
            usleep(timeout);
        }
        std::ostringstream ss;
        ss << pid << ":log:" << ++id;
        const std::string message = ss.str();
        if (config.verbose)
        {
            std::cout << message << std::endl;
        }
        client.send(message);
        ++msg_count;
        const struct timespec ts_curr = qbus::get_monotonic_time();
        const struct timespec dt = ts_curr - ts_prev;
        if (dt >= period)
        {
            const double delta = dt.tv_sec + 0.000000001 * dt.tv_nsec;
            const size_t mps = msg_count / delta;
            ts_prev = ts_curr;
            msg_count = 0;
            std::cout << mps << " MPS" << std::endl;
        }
    } while (count == 0 || --count > 0);
    return 0;
}

#ifdef QBUS_ZMQ_ENABLED
int zmq_client_cycle(const config_type& config)
{
    return base_client_cycle<zmq_client>(config);
}
#endif

template <typename Connector>
int qbus_client_cycle(const config_type& config)
{
    try
    {
        return base_client_cycle<qbus_client<Connector> >(config);
    }
    catch (int err)
    {
        return err;
    }
    return 0;
}

int main(int argc, char** argv)
{
    config_type config;
    const char *options = "?fve:c:a:";
    int opt;
    while ((opt = getopt(argc, argv, options)) != -1)
    {
        switch (opt)
        {
            case '?':
                std::cout << "-?\t\t help;" << std::endl;
                std::cout << "-v\t\t print log message on the screen;" << std::endl;
                std::cout << "-e {ENGINENAME}\t\t name of logger engine;" << std::endl;
                return 1;
            case 'v':
                config.verbose = true;
                break;
            case 'f':
                config.forced = true;
                break;
            case 'e':
                config.engine = optarg;
                break;
            case 'a':
                config.address = optarg;
                break;
            case 'c':
                try
                {
                    config.count = boost::lexical_cast<size_t>(optarg);
                }
                catch (...)
                {
                    std::cerr << "error: wrong value of message count" << std::endl;
                }
                break;
        }
    }

    client_list_type clients[] =
    {
#ifdef QBUS_ZMQ_ENABLED
        { "zmq", zmq_client_cycle },
#endif
        { "qbus", qbus_client_cycle<single_output_connector_type> },
        { "qbuss", qbus_client_cycle<connector::safe_connector<
            connector::output_connector<connector::single_bidirectional_connector_type >,
            connector::sharable_spinlocker_interface> > },
        { "", NULL }
    };

    for (size_t i = 0; i < sizeof(clients) / sizeof(clients[0]); ++i)
    {
        if (clients[i].func == NULL)
        {
            std::cerr << "error: wrong name of logger" << std::endl;
            break;
        }
        if (clients[i].name == config.engine)
        {
            return clients[i].func(config);
        }
    }

    return 1;
}

