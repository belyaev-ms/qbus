#ifndef QBUS_IPC_TEST_H
#define QBUS_IPC_TEST_H

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <fstream>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/format.hpp>
#include <vector>
#include <string>

const size_t count = 10;
const std::string test_producer = "./test_producer";
const std::string test_consumer = "./test_consumer";
const std::string test_producer_log = test_producer + ".log";
const std::string test_consumer_log_ = test_consumer + ".%d.log";

const std::string test_consumer_log(const size_t i)
{
    return boost::str(boost::format(test_consumer_log_) % i);
}

void run_process(const std::string& s)
{
    BOOST_TEST_MESSAGE("\trun process '" << s << "' ...");
    const int result = std::system((s + QBUS_IPC_TEST_PARAM).c_str());
    BOOST_TEST_MESSAGE("\tfinish process '" << s << "' " << result);
}

void clean_logs()
{
    for (size_t i = 0; i < count; ++i)
    {
        remove(test_consumer_log(i).c_str());
    }
    remove(test_producer_log.c_str());
}

std::vector<std::string> get_messages(const std::string& filename)
{
    std::vector<std::string> result;
    std::ifstream ifs(filename.c_str());
    std::string s;
    if (ifs.good())
    {
        while (!ifs.eof())
        {
            std::getline(ifs, s);
            result.push_back(s);
        }
    }
    for (std::vector<std::string>::iterator it = result.begin(); it != result.end(); ++it)
    {
        *it = std::string(std::find(it->rbegin(), it->rend(), ':').base(), it->end());
    }
    if (result.size() < 4)
    {
        result.clear();
    }
    else
    {
        result.resize(result.size() - 2);
        result.erase(result.begin());
    }
    return result;
}

BOOST_AUTO_TEST_CASE(one_server_and_few_clients)
{
    struct mutex_remove
    {
        mutex_remove()
        { 
            boost::interprocess::named_mutex::remove("test_producer");
            boost::interprocess::named_mutex::remove("test_consumer");
            clean_logs();
        }
        ~mutex_remove()
        { 
            boost::interprocess::named_mutex::remove("test_consumer");
            boost::interprocess::named_mutex::remove("test_producer");
        }
    } remover;
    
    boost::thread_group threads;
    boost::shared_ptr<boost::thread> pthread;
    boost::interprocess::named_mutex server_mutex(boost::interprocess::create_only, "test_producer");
    boost::interprocess::named_mutex client_mutex(boost::interprocess::create_only, "test_consumer");
    {
        boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(client_mutex);
        BOOST_TEST_MESSAGE("start process '" + test_producer + "':");
        {
            boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(server_mutex);
            const std::string s = test_producer + " > " + test_producer_log;
            pthread = boost::make_shared<boost::thread>(boost::bind(&run_process, s));
            sleep(1);
            BOOST_TEST_MESSAGE("start processes '" + test_consumer + "':");
            {
                for (size_t i = 0; i < count; ++i)
                {
                    const std::string s = test_consumer + " > " + test_consumer_log(i);
                    boost::thread *pth = new boost::thread(boost::bind(&run_process, s));
                    threads.add_thread(pth);
                }
            }
        }
        sleep(1);
    }
    {
        boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(server_mutex);
        BOOST_TEST_MESSAGE("wait for the processes '" + test_consumer + "' to finish:");
        threads.join_all();
    }
    BOOST_TEST_MESSAGE("wait for the process '" + test_producer + "' to finish:");
    pthread->join();
    
    const std::vector<std::string> server_messages = get_messages(test_producer_log);
    BOOST_REQUIRE_EQUAL(server_messages.size(), 1024);
    for (size_t i = 0; i < count; ++i)
    {
        const std::vector<std::string> client_messages = get_messages(test_consumer_log(i));
        BOOST_REQUIRE_EQUAL(client_messages.size(), server_messages.size());
        BOOST_REQUIRE_EQUAL_COLLECTIONS(server_messages.begin(), server_messages.end(),
            client_messages.begin(), client_messages.end());
    }
}

#endif /* QBUS_IPC_TEST_H */

