#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE shared_queue_test
#include <boost/test/unit_test.hpp>

#include "qbus/queue.h"
#include <vector>

typedef std::vector<uint8_t> buffer_t;

static buffer_t make_buffer(const size_t size)
{
    buffer_t buffer(size);
    for (size_t i = 0; i < size; ++i)
    {
        buffer[i] = i;
    }
    return buffer;
}

using namespace qbus;

const size_t slice_size = 512;

BOOST_AUTO_TEST_CASE(one_small_message)
{
    pmessage_type pmessage;
    queue::shared_queue queue(slice_size, 32);

    buffer_t buffer = make_buffer(slice_size / 4);
    BOOST_REQUIRE_EQUAL(queue.count(), 0);
    pmessage = queue.add(&buffer[0], buffer.size());
    BOOST_REQUIRE(pmessage);
    BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
    BOOST_REQUIRE_EQUAL(queue.count(), 1);
    pmessage = queue.get();
    BOOST_REQUIRE(pmessage);
    BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
    buffer_t data(pmessage->data_size());
    pmessage->unpack(&data[0]);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(),
        data.begin(), data.end());
    BOOST_REQUIRE_EQUAL(queue.count(), 1);
    queue.pop();
    BOOST_REQUIRE_EQUAL(queue.count(), 0);
    pmessage = queue.get();
    BOOST_REQUIRE(!pmessage);
}

BOOST_AUTO_TEST_CASE(one_large_message)
{
    pmessage_type pmessage;
    queue::shared_queue queue(slice_size, 32);

    buffer_t buffer = make_buffer(slice_size * 3);
    BOOST_REQUIRE_EQUAL(queue.count(), 0);
    pmessage = queue.add(&buffer[0], buffer.size());
    BOOST_REQUIRE(pmessage);
    BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
    BOOST_REQUIRE_EQUAL(queue.count(), 4);
    pmessage = queue.get();
    BOOST_REQUIRE(pmessage);
    BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
    buffer_t data(pmessage->data_size());
    pmessage->unpack(&data[0]);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(),
        data.begin(), data.end());
    BOOST_REQUIRE_EQUAL(queue.count(), 4);
    queue.pop();
    BOOST_REQUIRE_EQUAL(queue.count(), 0);
    pmessage = queue.get();
    BOOST_REQUIRE(!pmessage);
}

BOOST_AUTO_TEST_CASE(one_too_large_message)
{
    pmessage_type pmessage;
    queue::shared_queue queue(slice_size, 32);

    buffer_t buffer = make_buffer(slice_size * 32);
    BOOST_REQUIRE_EQUAL(queue.count(), 0);
    pmessage = queue.add(&buffer[0], buffer.size());
    BOOST_REQUIRE(!pmessage);
}

BOOST_AUTO_TEST_CASE(too_many_small_messages)
{
    pmessage_type pmessage;
    queue::shared_queue queue(slice_size, 32);

    buffer_t buffer = make_buffer(slice_size / 4);
    BOOST_REQUIRE_EQUAL(queue.count(), 0);
    for (size_t i = 0; i < 32; ++i)
    {
        pmessage = queue.add(&buffer[0], buffer.size());
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        BOOST_REQUIRE_EQUAL(queue.count(), i + 1);
    }

    pmessage = queue.add(&buffer[0], buffer.size());
    BOOST_REQUIRE(!pmessage);

    for (size_t i = 0; i < 32; ++i)
    {
        queue.pop();
        BOOST_REQUIRE_EQUAL(queue.count(), 31 - i);
    }
}

BOOST_AUTO_TEST_CASE(client_and_server)
{
    pmessage_type pmessage;

    buffer_t memory(queue::queue::static_size(slice_size, 32));
    queue::shared_queue server_queue(&memory[0], slice_size, 32);
    queue::shared_queue client_queue(&memory[0]);

    buffer_t buffer = make_buffer(slice_size / 4);
    for (size_t i = 0; i < 32; ++i)
    {
        server_queue.add(&buffer[0], buffer.size());
        BOOST_REQUIRE_EQUAL(server_queue.count(), i + 1);
        BOOST_REQUIRE_EQUAL(client_queue.count(), i + 1);
    }
    for (size_t i = 0; i < 32; ++i)
    {
        pmessage = client_queue.get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        client_queue.pop();
        server_queue.clean();
        BOOST_REQUIRE_EQUAL(server_queue.count(), 31 - i);
        BOOST_REQUIRE_EQUAL(client_queue.count(), 31 - i);
    }
    for (size_t i = 0; i < 32; ++i)
    {
        pmessage = server_queue.add(&buffer[0], buffer.size());
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        client_queue.get();
        pmessage = client_queue.get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        BOOST_REQUIRE_EQUAL(server_queue.count(), 1);
        BOOST_REQUIRE_EQUAL(client_queue.count(), 1);
        client_queue.pop();
        server_queue.clean();
        BOOST_REQUIRE_EQUAL(server_queue.count(), 0);
        BOOST_REQUIRE_EQUAL(client_queue.count(), 0);
    }
}

BOOST_AUTO_TEST_CASE(clients_and_server)
{
    pmessage_type pmessage;

    buffer_t memory(queue::queue::static_size(slice_size, 32));
    queue::shared_queue server_queue(&memory[0], slice_size, 32);
    queue::shared_queue client_queue1(&memory[0]);
    queue::shared_queue client_queue2(&memory[0]);

    buffer_t buffer = make_buffer(slice_size / 4);
    for (size_t i = 0; i < 32; ++i)
    {
        server_queue.add(&buffer[0], buffer.size());
        BOOST_REQUIRE_EQUAL(server_queue.count(), i + 1);
        BOOST_REQUIRE_EQUAL(client_queue1.count(), i + 1);
        BOOST_REQUIRE_EQUAL(client_queue2.count(), i + 1);
    }
    for (size_t i = 0; i < 32; ++i)
    {
        pmessage = client_queue1.get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        client_queue1.pop();
        server_queue.clean();
        BOOST_REQUIRE_EQUAL(server_queue.count(), 32 - i);
        BOOST_REQUIRE_EQUAL(client_queue1.count(), 31 - i);
        BOOST_REQUIRE_EQUAL(client_queue2.count(), 32 - i);
        pmessage = client_queue2.get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        client_queue2.pop();
        server_queue.clean();
        BOOST_REQUIRE_EQUAL(server_queue.count(), 31 - i);
        BOOST_REQUIRE_EQUAL(client_queue1.count(), 31 - i);
        BOOST_REQUIRE_EQUAL(client_queue2.count(), 31 - i);
    }
    for (size_t i = 0; i < 32; ++i)
    {
        pmessage = server_queue.add(&buffer[0], buffer.size());
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        client_queue1.get();
        pmessage = client_queue1.get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        BOOST_REQUIRE_EQUAL(server_queue.count(), 1);
        BOOST_REQUIRE_EQUAL(client_queue1.count(), 1);
        BOOST_REQUIRE_EQUAL(client_queue2.count(), 1);
        client_queue1.pop();
        server_queue.clean();
        BOOST_REQUIRE_EQUAL(server_queue.count(), 1);
        BOOST_REQUIRE_EQUAL(client_queue1.count(), 0);
        BOOST_REQUIRE_EQUAL(client_queue2.count(), 1);
        client_queue2.get();
        pmessage = client_queue2.get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        BOOST_REQUIRE_EQUAL(server_queue.count(), 1);
        BOOST_REQUIRE_EQUAL(client_queue1.count(), 0);
        BOOST_REQUIRE_EQUAL(client_queue2.count(), 1);
        client_queue2.pop();
        server_queue.clean();
        BOOST_REQUIRE_EQUAL(server_queue.count(), 0);
        BOOST_REQUIRE_EQUAL(client_queue1.count(), 0);
        BOOST_REQUIRE_EQUAL(client_queue2.count(), 0);
    }
}

