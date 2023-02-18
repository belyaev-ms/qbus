#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE    smart_shared_queue_test
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

class sid_mocker
{
public:
    sid_mocker() :
        m_sid(static_count++)
    {
        message::init_get_sid(sid_mocker::static_get_sid);
        mock();
    }
    void mock() const
    {
        static_sid = m_sid;
    }
private:
    message::sid_type m_sid;
    static message::sid_type static_count;
    static message::sid_type static_sid;
    static message::sid_type static_get_sid()
    {
        return static_sid;
    }
};

//static
message::sid_type sid_mocker::static_count = 1;
//static
message::sid_type sid_mocker::static_sid = 0;

template <typename Queue>
class sid_mocker_queue : protected sid_mocker, public Queue
{
public:
    typedef Queue base_type;
    typedef queue::id_type id_type;
    typedef message::tag_type tag_type;
    explicit sid_mocker_queue(void *ptr) :
        base_type(ptr)
    {}
    sid_mocker_queue(const id_type qid, void *ptr, const size_t cpct) :
        base_type(qid, ptr, cpct)
    {}
    virtual ~sid_mocker_queue()
    {
        mock();
    }
    bool push(const tag_type tag, const void *data, const size_t sz)
    {
        mock();
        return base_type::push(tag, data, sz);
    }
    const pmessage_type get() const
    {
        mock();
        return base_type::get();
    }
    bool pop()
    {
        mock();
        return base_type::pop();
    }
    using Queue::subscriptions_count;
};

typedef sid_mocker_queue<queue::smart_shared_queue> test_queue;

BOOST_AUTO_TEST_CASE(basic_test)
{
    const size_t capacity = 1024;
    const queue::id_type id = 1;
    buffer_t queue_buffer = make_buffer(test_queue::static_size(capacity));
    test_queue queue1(id, &queue_buffer[0], capacity);
    test_queue queue2(&queue_buffer[0]);
    
    BOOST_REQUIRE_EQUAL(queue1.id(), id);
    BOOST_REQUIRE_EQUAL(queue2.id(), id);
    BOOST_REQUIRE_EQUAL(queue1.capacity(), capacity);
    BOOST_REQUIRE_EQUAL(queue2.capacity(), capacity);
    BOOST_REQUIRE_EQUAL(queue1.size(), queue_buffer.size());
    BOOST_REQUIRE_EQUAL(queue2.size(), queue_buffer.size());
    BOOST_REQUIRE_EQUAL(queue1.count(), 1); // there is a service message of queue2
    BOOST_REQUIRE_EQUAL(queue2.count(), 0);
    BOOST_REQUIRE(!queue1.empty()); // there is a service message of queue2
    BOOST_REQUIRE(queue2.empty());
    BOOST_REQUIRE(!queue1.get()); // but this service message is unreadable
    BOOST_REQUIRE_EQUAL(queue1.count(), 0);
    BOOST_REQUIRE(queue1.empty());
    
    const queue::tag_type tag = 2;
    buffer_t message_buffer = make_buffer(32);
    BOOST_REQUIRE(queue1.push(tag, &message_buffer[0], message_buffer.size()));
    BOOST_REQUIRE_EQUAL(queue1.count(), 1);
    BOOST_REQUIRE_EQUAL(queue2.count(), 1);
    BOOST_REQUIRE(!queue1.empty());
    BOOST_REQUIRE(!queue2.empty());
    
    {
        // own messages are unreadable
        BOOST_REQUIRE(!queue1.get());
    }
    
    {
        pmessage_type pmessage = queue2.get();
        BOOST_REQUIRE(pmessage.get());
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
    const size_t reserved_size = 4 * message::base_message::static_size(1);
    const size_t capacity = 1024 + reserved_size;
    const queue::id_type id = 1;
    const size_t message_size = message::base_message::static_capacity(32);
    const size_t header_size = message::base_message::static_size(0);
    BOOST_TEST_MESSAGE("make queue that has a capacity = " << capacity);
    buffer_t queue_buffer = make_buffer(test_queue::static_size(capacity));
    test_queue queue1(id, &queue_buffer[0], capacity);
    test_queue queue2(&queue_buffer[0]);
    BOOST_REQUIRE(!queue1.get()); // service message is unreadable
    BOOST_REQUIRE(!queue2.get()); // service message is unreadable
    BOOST_REQUIRE(queue1.empty());
    BOOST_REQUIRE(queue2.empty());

    size_t count = (capacity - reserved_size) / message::base_message::static_size(message_size);
    
    BOOST_TEST_MESSAGE("push " << count << " messages each a size = " << message_size);
    for (size_t i = 0; i < count; ++i)
    {
        BOOST_TEST_MESSAGE("push " << i + 1 << "/" << count << " a message");
        BOOST_REQUIRE_EQUAL(queue2.count(), i);
        buffer_t message_buffer = make_buffer(message_size);
        BOOST_REQUIRE(queue1.push(i, &message_buffer[0], message_buffer.size()));
        BOOST_REQUIRE_EQUAL(queue2.count(), i + 1);
        BOOST_REQUIRE(!queue1.get());
    }
    BOOST_TEST_MESSAGE("try to push the " << count + 1 << " message");
    {
        BOOST_REQUIRE_EQUAL(queue2.count(), count);
        buffer_t message_buffer = make_buffer(message_size);
        BOOST_REQUIRE(!queue1.push(count, &message_buffer[0], message_buffer.size()));
        BOOST_REQUIRE_EQUAL(queue2.count(), count);
    }
    
    BOOST_TEST_MESSAGE("pop " << count << " messages");
    for (size_t i = 0; i < count; ++i)
    {
        BOOST_TEST_MESSAGE("pop " << i + 1 << "/" << count << " a message");
        BOOST_REQUIRE_EQUAL(queue2.count(), count - i);
        buffer_t message_buffer = make_buffer(message_size);
        pmessage_type pmessage = queue2.get();
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), message_buffer.size());
        BOOST_REQUIRE_EQUAL(pmessage->tag(), i);
        buffer_t buffer(pmessage->data_size());
        BOOST_REQUIRE_EQUAL(pmessage->unpack(&buffer[0]), buffer.size());
        BOOST_REQUIRE_EQUAL_COLLECTIONS(message_buffer.begin(), message_buffer.end(),
                buffer.begin(), buffer.end());
        queue2.pop();
        BOOST_REQUIRE_EQUAL(queue2.count(), count - i - 1);
    }

    BOOST_REQUIRE(queue2.empty());
    count = 3 * count / 2;
    BOOST_TEST_MESSAGE("push and pop " << count << " messages");
    for (size_t i = 0; i < count; ++i)
    {
        BOOST_TEST_MESSAGE("push " << i + 1 << " a message");
        buffer_t message_buffer = make_buffer(message_size);
        BOOST_REQUIRE(queue1.push(i, &message_buffer[0], message_buffer.size()));
        BOOST_REQUIRE(!queue1.get());
        BOOST_REQUIRE_EQUAL(queue2.count(), 1);
        BOOST_REQUIRE(!queue2.empty());
        BOOST_TEST_MESSAGE("pop " << i + 1 << " a message");
        pmessage_type pmessage = queue2.get();
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), message_buffer.size());
        BOOST_REQUIRE_EQUAL(pmessage->tag(), i);
        buffer_t buffer(pmessage->data_size());
        BOOST_REQUIRE_EQUAL(pmessage->unpack(&buffer[0]), buffer.size());
        BOOST_REQUIRE_EQUAL_COLLECTIONS(message_buffer.begin(), message_buffer.end(),
                buffer.begin(), buffer.end());
        queue2.pop();
        BOOST_REQUIRE_EQUAL(queue2.count(), 0);
        BOOST_REQUIRE(queue2.empty());
    }
    
    const size_t large_split_message_size = capacity - reserved_size - 2 * header_size;
    BOOST_TEST_MESSAGE("push a large split message that has a size = " << large_split_message_size);
    {
        buffer_t message_buffer = make_buffer(large_split_message_size);
        BOOST_REQUIRE(queue1.push(0, &message_buffer[0], message_buffer.size()));
        BOOST_REQUIRE(!queue1.get());
        BOOST_REQUIRE_EQUAL(queue2.count(), 1);
        BOOST_REQUIRE(!queue2.empty());
        BOOST_TEST_MESSAGE("pop the large split message");
        pmessage_type pmessage = queue2.get();
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), message_buffer.size());
        BOOST_REQUIRE_EQUAL(pmessage->tag(), 0);
        buffer_t buffer(pmessage->data_size());
        BOOST_REQUIRE_EQUAL(pmessage->unpack(&buffer[0]), buffer.size());
        BOOST_REQUIRE_EQUAL_COLLECTIONS(message_buffer.begin(), message_buffer.end(),
                buffer.begin(), buffer.end());
        queue2.pop();
        BOOST_REQUIRE_EQUAL(queue2.count(), 0);
        BOOST_REQUIRE(queue2.empty());
    }
}

BOOST_AUTO_TEST_CASE(one_producer_and_one_consumer_test)
{
    pmessage_type pmessage;

    const size_t reserved_size = 4 * message::base_message::static_size(1);
    const size_t capacity = 1024 + reserved_size;
    const queue::id_type id = 1;
    const size_t message_size = message::base_message::static_capacity(32);
    buffer_t memory(test_queue::static_size(capacity));
    test_queue producer_queue(id, &memory[0], capacity);
    test_queue consumer_queue(&memory[0]);
    buffer_t buffer = make_buffer(message_size);
    size_t count = (capacity - reserved_size) / message::base_message::static_size(message_size);
    BOOST_REQUIRE(!producer_queue.get()); // service message is unreadable
    BOOST_REQUIRE(!consumer_queue.get()); // service message is unreadable
    BOOST_REQUIRE(producer_queue.empty());
    BOOST_REQUIRE(consumer_queue.empty());
    for (size_t i = 0; i < count; ++i)
    {
        BOOST_REQUIRE(producer_queue.push(i, &buffer[0], buffer.size()));
        BOOST_REQUIRE(!producer_queue.get());
        BOOST_REQUIRE_EQUAL(consumer_queue.count(), i + 1);
    }
    
    BOOST_REQUIRE(producer_queue.empty());
    for (size_t i = 0; i < count; ++i)
    {
        pmessage = consumer_queue.get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        consumer_queue.pop();
        BOOST_REQUIRE_EQUAL(consumer_queue.count(), count - i - 1);
    }
    for (size_t i = 0; i < count; ++i)
    {
        BOOST_REQUIRE(producer_queue.push(0, &buffer[0], buffer.size()));
        BOOST_REQUIRE(!producer_queue.get());
        consumer_queue.get();
        pmessage = consumer_queue.get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        BOOST_REQUIRE_EQUAL(consumer_queue.count(), 1);
        consumer_queue.pop();
        BOOST_REQUIRE_EQUAL(consumer_queue.count(), 0);
    }
}

BOOST_AUTO_TEST_CASE(one_producer_and_many_consumers_test)
{
    pmessage_type pmessage;

    const size_t reserved_size = 6 * message::base_message::static_size(1);
    const size_t capacity = 1024 + reserved_size;
    const queue::id_type id = 1;
    const size_t message_size = message::base_message::static_capacity(32);
    buffer_t memory(test_queue::static_size(capacity));
    test_queue producer_queue(id, &memory[0], capacity);
    test_queue consumer_queue1(&memory[0]);
    test_queue consumer_queue2(&memory[0]);
    buffer_t buffer = make_buffer(message_size);
    size_t count = (capacity - reserved_size) / message::base_message::static_size(message_size);
    BOOST_REQUIRE(!producer_queue.get()); // service message is unreadable
    BOOST_REQUIRE(!consumer_queue1.get()); // service message is unreadable
    BOOST_REQUIRE(!consumer_queue2.get()); // service message is unreadable
    BOOST_REQUIRE(producer_queue.empty());
    BOOST_REQUIRE(consumer_queue1.empty());
    BOOST_REQUIRE(consumer_queue2.empty());

    BOOST_TEST_MESSAGE("push " << count << " messages each a size = " << message_size);
    for (size_t i = 0; i < count; ++i)
    {
        BOOST_TEST_MESSAGE("push " << i + 1 << "/" << count << " a message");
        BOOST_REQUIRE(producer_queue.push(0, &buffer[0], buffer.size()));
        BOOST_REQUIRE(!producer_queue.get());
        BOOST_REQUIRE_EQUAL(consumer_queue1.count(), i + 1);
        BOOST_REQUIRE_EQUAL(consumer_queue2.count(), i + 1);
    }

    for (size_t i = 0; i < count; ++i)
    {
        pmessage = consumer_queue1.get();
        BOOST_REQUIRE_EQUAL(consumer_queue1.count(), count - i);
        BOOST_REQUIRE_EQUAL(consumer_queue2.count(), count - i);
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        consumer_queue1.pop();
        BOOST_REQUIRE_EQUAL(consumer_queue1.count(), count - i - 1);
        BOOST_REQUIRE_EQUAL(consumer_queue2.count(), count - i);
        pmessage = consumer_queue2.get();
        BOOST_REQUIRE_EQUAL(consumer_queue1.count(), count - i - 1);
        BOOST_REQUIRE_EQUAL(consumer_queue2.count(), count - i);
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        consumer_queue2.pop();
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
        BOOST_REQUIRE(!producer_queue.get());
        BOOST_REQUIRE_EQUAL(producer_queue.count(), 0);
        BOOST_REQUIRE_EQUAL(consumer_queue1.count(), 0);
        BOOST_REQUIRE_EQUAL(consumer_queue2.count(), 0);
    }
}

BOOST_AUTO_TEST_CASE(many_producers_and_many_consumers_test)
{
    pmessage_type pmessage;

    const size_t reserved_size = 6 * message::base_message::static_size(1);
    const size_t capacity = 1024 + reserved_size;
    const queue::id_type id = 1;
    const size_t message_size = message::base_message::static_capacity(32);
    buffer_t memory(test_queue::static_size(capacity));
    test_queue queue1(id, &memory[0], capacity);
    test_queue queue2(&memory[0]);
    test_queue queue3(&memory[0]);
    buffer_t buffer = make_buffer(message_size);
    size_t count = (capacity - reserved_size) / message::base_message::static_size(message_size);
    BOOST_REQUIRE(!queue1.get()); // service message is unreadable
    BOOST_REQUIRE(!queue2.get()); // service message is unreadable
    BOOST_REQUIRE(!queue3.get()); // service message is unreadable
    BOOST_REQUIRE(queue1.empty());
    BOOST_REQUIRE(queue2.empty());
    BOOST_REQUIRE(queue3.empty());
    test_queue *queues[] = { &queue1, &queue2, &queue3 };

    BOOST_TEST_MESSAGE("push " << 5 * count << " messages each a size = " << message_size);
    size_t k = 0;
    for (size_t i = 0; i < count; ++i)
    {
        BOOST_TEST_MESSAGE("push " << i + 1 << "/" << count << " a message");
        BOOST_REQUIRE(queues[k]->push(0, &buffer[0], buffer.size()));
        BOOST_REQUIRE(!queues[k]->get());
        BOOST_REQUIRE(queues[k]->empty());
        for (size_t j = 0; j < 3; ++j)
        {
            if (j != k)
            {
                BOOST_REQUIRE_EQUAL(queues[j]->count(), 1);
                pmessage = queues[j]->get();
                BOOST_REQUIRE_EQUAL(queues[j]->count(), 1);
                BOOST_REQUIRE(pmessage);
                BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
                queues[j]->pop();
                BOOST_REQUIRE_EQUAL(queues[j]->count(), 0);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(connect_and_disconect_to_overflow_queue_test)
{
    pmessage_type pmessage;

    const size_t reserved_size = 4 * message::base_message::static_size(1);
    const size_t capacity = 1024 + reserved_size;
    const queue::id_type id = 1;
    const size_t message_size = message::base_message::static_capacity(32);
    buffer_t memory(test_queue::static_size(capacity));
    test_queue queue1(id, &memory[0], capacity);
    test_queue queue2(&memory[0]);
    buffer_t buffer = make_buffer(message_size);
    size_t count = (capacity - reserved_size) / message::base_message::static_size(message_size);
    BOOST_REQUIRE(!queue1.get()); // service message is unreadable
    BOOST_REQUIRE(!queue2.get()); // service message is unreadable
    BOOST_REQUIRE(queue1.empty());
    BOOST_REQUIRE(queue2.empty());

    BOOST_TEST_MESSAGE("push " << count << " messages each a size = " << message_size);
    for (size_t i = 0; i < count; ++i)
    {
        BOOST_TEST_MESSAGE("push " << i + 1 << "/" << count << " a message");
        BOOST_REQUIRE(queue1.push(0, &buffer[0], buffer.size()));
        BOOST_REQUIRE(!queue1.get());
    }

    BOOST_TEST_MESSAGE("error of pushing " << count + 1 << " a message");
    BOOST_REQUIRE(!queue1.push(0, &buffer[0], buffer.size()));

    BOOST_TEST_MESSAGE("connect and disconnect another client to the overflow queue");
    {
        test_queue queue3(&memory[0]);
    }
}

BOOST_AUTO_TEST_CASE(queue_subscriptions_count_test)
{
    const size_t capacity = 1024;
    const queue::id_type id = 1;
    buffer_t queue_buffer = make_buffer(queue::shared_queue::static_size(capacity));
    test_queue queue1(id, &queue_buffer[0], capacity);
    BOOST_REQUIRE_EQUAL(queue1.subscriptions_count(), 1);
    {
        test_queue queue2(&queue_buffer[0]);
        BOOST_REQUIRE_EQUAL(queue1.subscriptions_count(), 2);
        BOOST_REQUIRE_EQUAL(queue2.subscriptions_count(), 2);
    }
    BOOST_REQUIRE_EQUAL(queue1.subscriptions_count(), 1);
}
