#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE queue_test
#include <boost/test/unit_test.hpp>

#include "qbus_way/queue.h"
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

BOOST_AUTO_TEST_CASE(basic_test)
{
    const queue::id_type id = 1;
    const size_t capacity = 1024;
    buffer_t queue_buffer = make_buffer(queue::shared_queue::static_size(capacity));
    queue::shared_queue queue1(id, &queue_buffer[0], capacity);
    queue::shared_queue queue2(&queue_buffer[0]);
    
    BOOST_REQUIRE_EQUAL(queue1.id(), id);
    BOOST_REQUIRE_EQUAL(queue2.id(), id);
    BOOST_REQUIRE_EQUAL(queue1.capacity(), capacity);
    BOOST_REQUIRE_EQUAL(queue2.capacity(), capacity);
    BOOST_REQUIRE_EQUAL(queue1.size(), queue_buffer.size());
    BOOST_REQUIRE_EQUAL(queue2.size(), queue_buffer.size());
    BOOST_REQUIRE_EQUAL(queue1.count(), 0);
    BOOST_REQUIRE_EQUAL(queue2.count(), 0);
    BOOST_REQUIRE(queue1.empty());
    BOOST_REQUIRE(queue2.empty());
    
    const queue::tag_type tag = 2;
    buffer_t message_buffer = make_buffer(32);
    BOOST_REQUIRE(queue1.add(tag, &message_buffer[0], message_buffer.size()));
    BOOST_REQUIRE_EQUAL(queue1.count(), 1);
    BOOST_REQUIRE_EQUAL(queue2.count(), 1);
    BOOST_REQUIRE(!queue1.empty());
    BOOST_REQUIRE(!queue2.empty());
    
    {
        pmessage_type pmessage = queue1.get();
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), message_buffer.size());
        BOOST_REQUIRE_EQUAL(pmessage->tag(), tag);
        buffer_t buffer(pmessage->data_size());
        pmessage->unpack(&buffer[0]);
        BOOST_REQUIRE_EQUAL_COLLECTIONS(message_buffer.begin(), message_buffer.end(),
                buffer.begin(), buffer.end());
    }
    
    {
        pmessage_type pmessage = queue2.get();
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), message_buffer.size());
        BOOST_REQUIRE_EQUAL(pmessage->tag(), tag);
        buffer_t buffer(pmessage->data_size());
        pmessage->unpack(&buffer[0]);
        BOOST_REQUIRE_EQUAL_COLLECTIONS(message_buffer.begin(), message_buffer.end(),
                buffer.begin(), buffer.end());
    }
    
    queue1.pop();
    BOOST_REQUIRE_EQUAL(queue1.count(), 0);
    BOOST_REQUIRE_EQUAL(queue2.count(), 0);
    BOOST_REQUIRE(queue1.empty());
    BOOST_REQUIRE(queue2.empty());
}

BOOST_AUTO_TEST_CASE(add_pop_message_test)
{
    const queue::id_type id = 1;
    const size_t capacity = 1024;
    const size_t message_size = 32;
    size_t data_size = message_size;
    size_t header_size = 0;
    {
        buffer_t queue_buffer = make_buffer(queue::shared_queue::static_size(capacity));
        queue::shared_queue queue(id, &queue_buffer[0], capacity);

        buffer_t message_buffer = make_buffer(data_size);
        BOOST_REQUIRE(queue.add(2, &message_buffer[0], message_buffer.size()));
        pmessage_type pmessage = queue.get();
        header_size = pmessage->size() - message_buffer.size();
        queue.pop();
        BOOST_REQUIRE_EQUAL(queue.count(), 0);
        BOOST_REQUIRE(queue.empty());
    }
    data_size -= header_size;
    
    buffer_t queue_buffer = make_buffer(queue::shared_queue::static_size(capacity));
    queue::shared_queue queue(id, &queue_buffer[0], capacity);
    const size_t count = capacity / message_size;
    
    BOOST_TEST_MESSAGE("capacity = " << capacity << ", message_size = " << message_size);
    for (size_t i = 0; i < count; ++i)
    {
        BOOST_TEST_MESSAGE("add " << i + 1 << "/" << count << " message");
        BOOST_REQUIRE_EQUAL(queue.count(), i);
        buffer_t message_buffer = make_buffer(data_size);
        BOOST_REQUIRE(queue.add(i, &message_buffer[0], message_buffer.size()));
        BOOST_REQUIRE_EQUAL(queue.count(), i + 1);
    }
    {
        BOOST_TEST_MESSAGE("try to add " << count + 1 << " message");
        BOOST_REQUIRE_EQUAL(queue.count(), count);
        buffer_t message_buffer = make_buffer(data_size);
        BOOST_REQUIRE(!queue.add(count, &message_buffer[0], message_buffer.size()));
        BOOST_REQUIRE_EQUAL(queue.count(), count);
    }
    for (size_t i = 0; i < count; ++i)
    {
        BOOST_TEST_MESSAGE("pop " << i + 1 << "/" << count << " message");
        BOOST_REQUIRE_EQUAL(queue.count(), count - i);
        buffer_t message_buffer = make_buffer(data_size);
        pmessage_type pmessage = queue.get();
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), message_buffer.size());
        BOOST_REQUIRE_EQUAL(pmessage->tag(), i);
        buffer_t buffer(pmessage->data_size());
        pmessage->unpack(&buffer[0]);
        BOOST_REQUIRE_EQUAL_COLLECTIONS(message_buffer.begin(), message_buffer.end(),
                buffer.begin(), buffer.end());
        queue.pop();
        BOOST_REQUIRE_EQUAL(queue.count(), count - i - 1);
    }
    for (size_t i = 0; i < 3 * count / 2; ++i)
    {
        BOOST_TEST_MESSAGE("add " << i + 1 << " message");
        buffer_t message_buffer = make_buffer(data_size);
        BOOST_REQUIRE(queue.add(i, &message_buffer[0], message_buffer.size()));
        BOOST_REQUIRE_EQUAL(queue.count(), 1);
        BOOST_REQUIRE(!queue.empty());
        BOOST_TEST_MESSAGE("pop " << i + 1 << " message");
        pmessage_type pmessage = queue.get();
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), message_buffer.size());
        BOOST_REQUIRE_EQUAL(pmessage->tag(), i);
        buffer_t buffer(pmessage->data_size());
        pmessage->unpack(&buffer[0]);
        BOOST_REQUIRE_EQUAL_COLLECTIONS(message_buffer.begin(), message_buffer.end(),
                buffer.begin(), buffer.end());
        queue.pop();
        BOOST_REQUIRE_EQUAL(queue.count(), 0);
        BOOST_REQUIRE(queue.empty());
    }
    {
        const size_t big_size = capacity - 2 * header_size;
        BOOST_TEST_MESSAGE("add a big message that has a size = " << big_size);
        buffer_t message_buffer = make_buffer(big_size);
        BOOST_REQUIRE(queue.add(0, &message_buffer[0], message_buffer.size()));
        BOOST_REQUIRE_EQUAL(queue.count(), 1);
        BOOST_REQUIRE(!queue.empty());
        BOOST_TEST_MESSAGE("pop the big message");
        pmessage_type pmessage = queue.get();
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), message_buffer.size());
        BOOST_REQUIRE_EQUAL(pmessage->tag(), 0);
        buffer_t buffer(pmessage->data_size());
        pmessage->unpack(&buffer[0]);
        BOOST_REQUIRE_EQUAL_COLLECTIONS(message_buffer.begin(), message_buffer.end(),
                buffer.begin(), buffer.end());
        queue.pop();
        BOOST_REQUIRE_EQUAL(queue.count(), 0);
        BOOST_REQUIRE(queue.empty());
    }
    {
        const size_t size1 = capacity / 2 - header_size - header_size / 2;
        BOOST_TEST_MESSAGE("add a message on the border");
        buffer_t message_buffer1 = make_buffer(size1);
        BOOST_REQUIRE(queue.add(0, &message_buffer1[0], message_buffer1.size()));
        BOOST_REQUIRE_EQUAL(queue.count(), 1);
        BOOST_REQUIRE(!queue.empty());
        BOOST_TEST_MESSAGE("add a holly message");
        const size_t size2 = capacity / 2 - 2 * header_size;
        buffer_t message_buffer2 = make_buffer(size2);
        BOOST_REQUIRE(queue.add(1, &message_buffer2[0], message_buffer2.size()));
        BOOST_REQUIRE_EQUAL(queue.count(), 2);
        BOOST_REQUIRE(!queue.empty());
        BOOST_TEST_MESSAGE("pop the holly message");
        {
            pmessage_type pmessage = queue.get();
            BOOST_REQUIRE_EQUAL(pmessage->data_size(), message_buffer1.size());
            BOOST_REQUIRE_EQUAL(pmessage->tag(), 0);
            buffer_t buffer(pmessage->data_size());
            pmessage->unpack(&buffer[0]);
            BOOST_REQUIRE_EQUAL_COLLECTIONS(message_buffer1.begin(), message_buffer1.end(),
                    buffer.begin(), buffer.end());
            queue.pop();
            BOOST_REQUIRE_EQUAL(queue.count(), 1);
        }
        {
            pmessage_type pmessage = queue.get();
            BOOST_REQUIRE_EQUAL(pmessage->data_size(), message_buffer2.size());
            BOOST_REQUIRE_EQUAL(pmessage->tag(), 1);
            buffer_t buffer(pmessage->data_size());
            pmessage->unpack(&buffer[0]);
            BOOST_REQUIRE_EQUAL_COLLECTIONS(message_buffer2.begin(), message_buffer2.end(),
                    buffer.begin(), buffer.end());
            queue.pop();
            BOOST_REQUIRE_EQUAL(queue.count(), 0);
        }
    }
}

