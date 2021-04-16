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

BOOST_AUTO_TEST_CASE(basic_test)
{
    const size_t capacity = 1024;
    const queue::id_type id = 1;
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
    BOOST_REQUIRE(queue1.push(tag, &message_buffer[0], message_buffer.size()));
    BOOST_REQUIRE_EQUAL(queue1.count(), 1);
    BOOST_REQUIRE_EQUAL(queue2.count(), 1);
    BOOST_REQUIRE(!queue1.empty());
    BOOST_REQUIRE(!queue2.empty());
    
    {
        pmessage_type pmessage = queue1.get();
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), message_buffer.size());
        BOOST_REQUIRE_EQUAL(pmessage->tag(), tag);
        buffer_t buffer(pmessage->data_size());
        BOOST_REQUIRE_EQUAL(pmessage->unpack(&buffer[0]), buffer.size());
        BOOST_REQUIRE_EQUAL_COLLECTIONS(message_buffer.begin(), message_buffer.end(),
                buffer.begin(), buffer.end());
    }
    
    {
        pmessage_type pmessage = queue2.get();
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), message_buffer.size());
        BOOST_REQUIRE_EQUAL(pmessage->tag(), tag);
        buffer_t buffer(pmessage->data_size());
        BOOST_REQUIRE_EQUAL(pmessage->unpack(&buffer[0]), buffer.size());
        BOOST_REQUIRE_EQUAL_COLLECTIONS(message_buffer.begin(), message_buffer.end(),
                buffer.begin(), buffer.end());
    }
    
    queue1.pop();
    BOOST_REQUIRE_EQUAL(queue1.count(), 0);
    BOOST_REQUIRE_EQUAL(queue2.count(), 1);
    BOOST_REQUIRE(queue1.empty());
    BOOST_REQUIRE(!queue2.empty());
    queue2.pop();
    BOOST_REQUIRE_EQUAL(queue1.count(), 0);
    BOOST_REQUIRE_EQUAL(queue2.count(), 0);
    BOOST_REQUIRE(queue1.empty());
    BOOST_REQUIRE(queue2.empty());
}

BOOST_AUTO_TEST_CASE(push_pop_message_test)
{
    const size_t capacity = 1024;
    const queue::id_type id = 1;
    const size_t message_size = message::base_message::static_capacity(32);
    const size_t header_size = message::base_message::static_size(0);
    BOOST_TEST_MESSAGE("make queue that has a capacity = " << capacity);
    buffer_t queue_buffer = make_buffer(queue::shared_queue::static_size(capacity));
    queue::shared_queue queue(id, &queue_buffer[0], capacity);
    size_t count = capacity / message::base_message::static_size(message_size);
    
    BOOST_TEST_MESSAGE("push " << count << " messages each a size = " << message_size);
    for (size_t i = 0; i < count; ++i)
    {
        BOOST_TEST_MESSAGE("push " << i + 1 << "/" << count << " a message");
        BOOST_REQUIRE_EQUAL(queue.count(), i);
        buffer_t message_buffer = make_buffer(message_size);
        BOOST_REQUIRE(queue.push(i, &message_buffer[0], message_buffer.size()));
        BOOST_REQUIRE_EQUAL(queue.count(), i + 1);
    }
    
    BOOST_TEST_MESSAGE("try to push the " << count + 1 << " message");
    {
        BOOST_REQUIRE_EQUAL(queue.count(), count);
        buffer_t message_buffer = make_buffer(message_size);
        BOOST_REQUIRE(!queue.push(count, &message_buffer[0], message_buffer.size()));
        BOOST_REQUIRE_EQUAL(queue.count(), count);
    }
    
    BOOST_TEST_MESSAGE("pop " << count << " messages");
    for (size_t i = 0; i < count; ++i)
    {
        BOOST_TEST_MESSAGE("pop " << i + 1 << "/" << count << " a message");
        BOOST_REQUIRE_EQUAL(queue.count(), count - i);
        buffer_t message_buffer = make_buffer(message_size);
        pmessage_type pmessage = queue.get();
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), message_buffer.size());
        BOOST_REQUIRE_EQUAL(pmessage->tag(), i);
        buffer_t buffer(pmessage->data_size());
        BOOST_REQUIRE_EQUAL(pmessage->unpack(&buffer[0]), buffer.size());
        BOOST_REQUIRE_EQUAL_COLLECTIONS(message_buffer.begin(), message_buffer.end(),
                buffer.begin(), buffer.end());
        queue.pop();
        BOOST_REQUIRE_EQUAL(queue.count(), count - i - 1);
    }
    
    count = 3 * count / 2;
    BOOST_TEST_MESSAGE("push and pop " << count << " messages");
    for (size_t i = 0; i < count; ++i)
    {
        BOOST_TEST_MESSAGE("push " << i + 1 << " a message");
        buffer_t message_buffer = make_buffer(message_size);
        BOOST_REQUIRE(queue.push(i, &message_buffer[0], message_buffer.size()));
        BOOST_REQUIRE_EQUAL(queue.count(), 1);
        BOOST_REQUIRE(!queue.empty());
        BOOST_TEST_MESSAGE("pop " << i + 1 << " a message");
        pmessage_type pmessage = queue.get();
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), message_buffer.size());
        BOOST_REQUIRE_EQUAL(pmessage->tag(), i);
        buffer_t buffer(pmessage->data_size());
        BOOST_REQUIRE_EQUAL(pmessage->unpack(&buffer[0]), buffer.size());
        BOOST_REQUIRE_EQUAL_COLLECTIONS(message_buffer.begin(), message_buffer.end(),
                buffer.begin(), buffer.end());
        queue.pop();
        BOOST_REQUIRE_EQUAL(queue.count(), 0);
        BOOST_REQUIRE(queue.empty());
    }
    
    const size_t large_split_message_size = capacity - 2 * header_size;
    BOOST_TEST_MESSAGE("push a large split message that has a size = " << large_split_message_size);
    {
        buffer_t message_buffer = make_buffer(large_split_message_size);
        BOOST_REQUIRE(queue.push(0, &message_buffer[0], message_buffer.size()));
        BOOST_REQUIRE_EQUAL(queue.count(), 1);
        BOOST_REQUIRE(!queue.empty());
        BOOST_TEST_MESSAGE("pop the large split message");
        pmessage_type pmessage = queue.get();
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), message_buffer.size());
        BOOST_REQUIRE_EQUAL(pmessage->tag(), 0);
        buffer_t buffer(pmessage->data_size());
        BOOST_REQUIRE_EQUAL(pmessage->unpack(&buffer[0]), buffer.size());
        BOOST_REQUIRE_EQUAL_COLLECTIONS(message_buffer.begin(), message_buffer.end(),
                buffer.begin(), buffer.end());
        queue.pop();
        BOOST_REQUIRE_EQUAL(queue.count(), 0);
        BOOST_REQUIRE(queue.empty());
    }
    
    BOOST_TEST_MESSAGE("push a holly message");
    {
        const size_t size1 = capacity / 2 - header_size - header_size / 2;
        BOOST_TEST_MESSAGE("push a message on the border");
        buffer_t message_buffer1 = make_buffer(size1);
        BOOST_REQUIRE(queue.push(0, &message_buffer1[0], message_buffer1.size()));
        BOOST_REQUIRE_EQUAL(queue.count(), 1);
        BOOST_REQUIRE(!queue.empty());
        BOOST_TEST_MESSAGE("push a holly message");
        const size_t size2 = capacity / 2 - 2 * header_size;
        buffer_t message_buffer2 = make_buffer(size2);
        BOOST_REQUIRE(queue.push(1, &message_buffer2[0], message_buffer2.size()));
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

BOOST_AUTO_TEST_CASE(one_producer_and_one_consumer_test)
{
    pmessage_type pmessage;

    const size_t capacity = 1024;
    const queue::id_type id = 1;
    const size_t message_size = message::base_message::static_capacity(32);
    buffer_t memory(queue::shared_queue::static_size(capacity));
    queue::shared_queue producer_queue(0, &memory[0], capacity);
    queue::shared_queue consumer_queue(&memory[0]);
    buffer_t buffer = make_buffer(message_size);
    size_t count = capacity / message::base_message::static_size(message_size);
    for (size_t i = 0; i < count; ++i)
    {
        BOOST_REQUIRE(producer_queue.push(i, &buffer[0], buffer.size()));
        BOOST_REQUIRE_EQUAL(producer_queue.count(), i + 1);
        BOOST_REQUIRE_EQUAL(consumer_queue.count(), i + 1);
    }

    for (size_t i = 0; i < count; ++i)
    {
        pmessage = consumer_queue.get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        producer_queue.pop();
        consumer_queue.pop();
        BOOST_REQUIRE_EQUAL(producer_queue.count(), 31 - i);
        BOOST_REQUIRE_EQUAL(consumer_queue.count(), 31 - i);
    }
    for (size_t i = 0; i < count; ++i)
    {
        BOOST_REQUIRE(producer_queue.push(0, &buffer[0], buffer.size()));
        consumer_queue.get();
        pmessage = consumer_queue.get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        BOOST_REQUIRE_EQUAL(producer_queue.count(), 1);
        BOOST_REQUIRE_EQUAL(consumer_queue.count(), 1);
        producer_queue.pop();
        consumer_queue.pop();
        BOOST_REQUIRE_EQUAL(producer_queue.count(), 0);
        BOOST_REQUIRE_EQUAL(consumer_queue.count(), 0);
    }
}

BOOST_AUTO_TEST_CASE(one_producer_and_many_consumers_test)
{
    pmessage_type pmessage;

    const size_t capacity = 1024;
    const queue::id_type id = 1;
    const size_t message_size = message::base_message::static_capacity(32);
    buffer_t memory(queue::shared_queue::static_size(capacity));
    queue::shared_queue producer_queue(0, &memory[0], capacity);
    queue::shared_queue consumer_queue1(&memory[0]);
    queue::shared_queue consumer_queue2(&memory[0]);

    buffer_t buffer = make_buffer(message_size);
    size_t count = capacity / message::base_message::static_size(message_size);
    for (size_t i = 0; i < count; ++i)
    {
        BOOST_REQUIRE(producer_queue.push(0, &buffer[0], buffer.size()));
        BOOST_REQUIRE_EQUAL(producer_queue.count(), i + 1);
        BOOST_REQUIRE_EQUAL(consumer_queue1.count(), i + 1);
        BOOST_REQUIRE_EQUAL(consumer_queue2.count(), i + 1);
    }
    for (size_t i = 0; i < count; ++i)
    {
        pmessage = consumer_queue1.get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        consumer_queue1.pop();
        BOOST_REQUIRE_EQUAL(producer_queue.count(), count - i);
        BOOST_REQUIRE_EQUAL(consumer_queue1.count(), count - i - 1);
        BOOST_REQUIRE_EQUAL(consumer_queue2.count(), count - i);
        pmessage = consumer_queue2.get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        consumer_queue2.pop();
        BOOST_REQUIRE_EQUAL(producer_queue.count(), count - i);
        BOOST_REQUIRE_EQUAL(consumer_queue1.count(), count - i - 1);
        BOOST_REQUIRE_EQUAL(consumer_queue2.count(), count - i - 1);
        pmessage = producer_queue.get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        producer_queue.pop();
        BOOST_REQUIRE_EQUAL(producer_queue.count(), count - i - 1);
        BOOST_REQUIRE_EQUAL(consumer_queue1.count(), count - i - 1);
        BOOST_REQUIRE_EQUAL(consumer_queue2.count(), count - i - 1);
    }
    for (size_t i = 0; i < count; ++i)
    {
        BOOST_REQUIRE(producer_queue.push(0, &buffer[0], buffer.size()));
        pmessage = consumer_queue1.get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        BOOST_REQUIRE_EQUAL(producer_queue.count(), 1);
        BOOST_REQUIRE_EQUAL(consumer_queue1.count(), 1);
        BOOST_REQUIRE_EQUAL(consumer_queue2.count(), 1);
        consumer_queue1.pop();
        BOOST_REQUIRE_EQUAL(producer_queue.count(), 1);
        BOOST_REQUIRE_EQUAL(consumer_queue1.count(), 0);
        BOOST_REQUIRE_EQUAL(consumer_queue2.count(), 1);
        pmessage = consumer_queue2.get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        BOOST_REQUIRE_EQUAL(producer_queue.count(), 1);
        BOOST_REQUIRE_EQUAL(consumer_queue1.count(), 0);
        BOOST_REQUIRE_EQUAL(consumer_queue2.count(), 1);
        consumer_queue2.pop();
        BOOST_REQUIRE_EQUAL(producer_queue.count(), 1);
        BOOST_REQUIRE_EQUAL(consumer_queue1.count(), 0);
        BOOST_REQUIRE_EQUAL(consumer_queue2.count(), 0);
        pmessage = producer_queue.get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        BOOST_REQUIRE_EQUAL(producer_queue.count(), 1);
        BOOST_REQUIRE_EQUAL(consumer_queue1.count(), 0);
        BOOST_REQUIRE_EQUAL(consumer_queue2.count(), 0);
        producer_queue.pop();
        BOOST_REQUIRE_EQUAL(producer_queue.count(), 0);
        BOOST_REQUIRE_EQUAL(consumer_queue1.count(), 0);
        BOOST_REQUIRE_EQUAL(consumer_queue2.count(), 0);
    }
}
