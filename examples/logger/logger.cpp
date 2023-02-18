#include "qbus/common.h"
#include "qbus/connector.h"
#ifdef QBUS_ZMQ_ENABLED
#include <zmq.hpp>
#endif
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/unordered_map.hpp>

using namespace qbus;

struct config_type
{
    bool verbose = false;
    bool has_check = false;
    size_t buffer_size = 1024 * 1024;
    std::string filename;
    std::string engine;
    std::string address = "logger";
};

typedef int (*logger_func)(const config_type& config);

struct logger_list_type
{
    std::string name;
    logger_func func;
};

class base_logger
{
public:
    base_logger(const config_type& config) :
        m_config(config)
    {
    }

    const std::string& record() const
    {
        return m_record;
    }

    bool validate() const
    {
        static boost::unordered_map<int, size_t> counters;
        std::vector<std::string> nodes;
        boost::split(nodes, m_record, boost::is_any_of(":"));
        if (nodes.size() == 3)
        {
            try
            {
                int pid = boost::lexical_cast<int>(nodes[0]);
                size_t counter = boost::lexical_cast<size_t>(nodes[2]);
                if (counters.count(pid) != 0)
                {
                    return counter == ++counters[pid];
                }
                counters[pid] = counter;
                return true;
            }
            catch (...)
            {
            }
        }
        return false;
    }

protected:
    const config_type m_config;
    std::string m_record;
};

#ifdef QBUS_ZMQ_ENABLED
class zmq_logger : public base_logger
{
public:
    zmq_logger(const config_type& config ) :
        base_logger(config),
        m_receiver(m_context, zmq::socket_type::pull)
    {
        const std::string address = "ipc://" + config.address;
        m_receiver.bind(address.c_str());
    }
    
    bool receive()
    {
        if (m_receiver.recv(m_message))
        {
            m_record = m_message.to_string();
            return true;
        }
        return false;
    }

private:
    zmq::context_t m_context;
    zmq::socket_t m_receiver;
    zmq::message_t m_message;
};
#endif

template <typename Connector>
class qbus_logger : public base_logger
{
public:
    qbus_logger(const config_type& config) :
        base_logger(config),
        m_pconnector(connector::make<Connector>(config.address))
    {
        if (!m_pconnector->create(0, config.buffer_size))
        {
            throw 2;
        }
        m_timeout.tv_sec = 1;
        m_timeout.tv_nsec = 0;
    }

    bool receive()
    {
        pmessage_type m_pmessage = m_pconnector->get(m_timeout);
        if (m_pmessage)
        {
            const size_t size = m_pmessage->data_size();
            if (size > 0)
            {
                m_record.resize(size - 1);
                m_pmessage->unpack(&m_record[0]);
                m_pconnector->pop(m_timeout);
                return true;
            }
        }
        return false;
    }

private:
    pconnector_type m_pconnector;
    std::string m_message;
    struct timespec m_timeout;
};

template <typename Logger>
int base_logger_cycle(const config_type& config)
{
    Logger logger(config);
    struct timespec period;
    period.tv_sec = 1;
    period.tv_nsec = 0;
    struct timespec ts_prev = qbus::get_monotonic_time();
    size_t msg_count = 0;
    while (true)
    {
        if (logger.receive())
        {
            if (config.verbose)
            {
                std::cout << logger.record() << std::endl;
            }
            if (config.has_check && !logger.validate())
            {
                std::cout << "WRONG: " << logger.record() << std::endl;
            }
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
        }
    }
    return 0;
}

#ifdef QBUS_ZMQ_ENABLED
int zmq_logger_cycle(const config_type& config)
{
    return base_logger_cycle<zmq_logger>(config);
}
#endif

template <typename Connector>
int qbus_logger_cycle(const config_type& config)
{
    try
    {
        return base_logger_cycle<qbus_logger<Connector> >(config);
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

    const char *options = "?vtf:b:e:a:";
    int opt;
    while ((opt = getopt(argc, argv, options)) != -1)
    {
        switch (opt)
        {
            case '?':
                std::cout << "-?\t\t help;" << std::endl;
                std::cout << "-v\t\t print log message on the screen;" << std::endl;
                std::cout << "-t\t\t check log message;" << std::endl;
                std::cout << "-e {ENGINENAME}\t\t name of logger engine;" << std::endl;
                std::cout << "-f {FILENAME}\t\t save log message into the file;" << std::endl;
                std::cout << "-b {SIZE}\t\t buffer size in MB (default 1 MB)." << std::endl;
                std::cout << "-a {PORT}\t\t address." << std::endl;
                return 1;
            case 'v':
                config.verbose = true;
                break;
            case 't':
                config.has_check = true;
                break;
            case 'e':
                config.engine = optarg;
                break;
            case 'a':
                config.address = optarg;
                break;
            case 'b':
                try
                {
                    config.buffer_size = 1024 * 1024 * boost::lexical_cast<size_t>(optarg);
                }
                catch (...)
                {
                    std::cerr << "error: wrong value of buffer size" << std::endl;
                }
                break;
        }
    }

    logger_list_type loggers[] =
    {
#ifdef QBUS_ZMQ_ENABLED
        { "zmq", zmq_logger_cycle },
#endif
        { "qbus", qbus_logger_cycle<single_input_connector_type> },
        { "qbuss", qbus_logger_cycle<connector::safe_connector<
            connector::input_connector<connector::single_bidirectional_connector_type >,
            connector::sharable_spinlocker_interface> > },
        { "", NULL }
    };

    for (size_t i = 0; i < sizeof(loggers) / sizeof(loggers[0]); ++i)
    {
        if (loggers[i].func == NULL)
        {
            std::cerr << "error: wrong name of logger" << std::endl;
            break;
        }
        if (loggers[i].name == config.engine)
        {
            return loggers[i].func(config);
        }
    }

    return 1;
}



