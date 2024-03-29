#ifndef QBUS_QUEUE_H
#define QBUS_QUEUE_H

#include "qbus/message.h"
#include "qbus/common.h"
#include "qbus/service_message.h"
#include <stddef.h>
#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace qbus
{

namespace queue
{

typedef uint32_t id_type;
typedef uint32_t pos_type;
typedef message::tag_type tag_type;
typedef message::pmessage_type pmessage_type;

/**
 * The base queue
 */
class base_queue
{
public:
    typedef std::pair<pos_type, size_t> region_type;
    typedef std::pair<pmessage_type, pos_type> message_desc_type;
    explicit base_queue(void *ptr);
    base_queue(const id_type qid, void *ptr, const size_t cpct);
    virtual ~base_queue();
    bool push(const tag_type tag, const void *data, const size_t sz); ///< push new message to the queue
    const pmessage_type get() const; ///< get the next message
    bool pop(); ///< remove the next message
    id_type id() const; ///< get the identifier of the queue
    size_t capacity() const; ///< get the capacity of the queue
    size_t keepalive_timeout() const; ///< get the keep alive timeout
    void keepalive_timeout(const size_t value); ///< set the keep alive timeout
    virtual size_t count() const; ///< get the count of messages
    bool empty() const; ///< check the queue is empty 
    void clear(); ///< clear the queue
    size_t clean(); ///< collect garbage
    virtual size_t size() const; ///< get the size of the queue
    static size_t static_size(const size_t cpct)
    {
        return HEADER_SIZE + cpct;
    }
protected:
    enum
    {
        ID_OFFSET         = 0,
        ID_SIZE           = sizeof(id_type),
        CAPACITY_OFFSET   = ID_OFFSET + ID_SIZE,
        CAPACITY_SIZE     = sizeof(uint32_t),
        TIMEOUT_OFFSET    = CAPACITY_OFFSET + CAPACITY_SIZE,
        TIMEOUT_SIZE      = sizeof(uint32_t),
        COUNT_OFFSET      = TIMEOUT_OFFSET + TIMEOUT_SIZE,
        COUNT_SIZE        = sizeof(uint32_t),
        HEAD_OFFSET       = COUNT_OFFSET + COUNT_SIZE,
        HEAD_SIZE         = sizeof(pos_type),
        TAIL_OFFSET       = HEAD_OFFSET + HEAD_SIZE,
        TAIL_SIZE         = HEAD_SIZE,
        DATA_OFFSET       = TAIL_OFFSET + TAIL_SIZE,
        HEADER_SIZE       = DATA_OFFSET
    };
    typedef std::pair<size_t, size_t> garbage_info_type; ///< the pair of : { number of cleaned messages, its total size }
    void id(const id_type value); ///< set the identifier of the queue
    virtual pos_type head() const; /// get the head of the queue
    void head(const pos_type value); /// set the head of the queue
    pos_type tail() const; /// get the tail of the queue
    void tail(const pos_type value); /// set the tail of the queue
    void count(const size_t value); ///< set the count of messages
    void *data(const pos_type pos = 0) const; ///< get the pointer to data region of the queue
    virtual garbage_info_type clean_messages(); ///< collect garbage
    virtual message_desc_type push_message(const void *data, const size_t size) = 0; ///< push new message to the queue
    virtual message_desc_type get_message() const = 0; ///< get a message from the queue
    virtual void pop_message(const message_desc_type& message_desc) = 0; ///< pop a message from the queue
    virtual pmessage_type make_message(void *ptr, const size_t cpct) const = 0; ///< make an empty message
    virtual pmessage_type make_message(void *ptr) const = 0; ///< make an empty message
    virtual region_type get_free_region(region_type *pprev_region = NULL) const; ///< get the next free region
    virtual region_type get_busy_region(region_type *pprev_region = NULL) const; ///< get the next busy region
private:
    base_queue();
    base_queue(const base_queue&);
    base_queue& operator=(const base_queue&);
    bool do_push(const tag_type tag, const void *data, const size_t sz); ///< push new message to the queue
    void capacity(const size_t capacity); ///< set the capacity of of the queue
#ifdef QBUS_TEST_ENABLED    
    region_type get_real_busy_region(region_type *pprev_region = NULL) const; ///< get the real next busy region
    void print() const; ///< print the information of the queue structure
#endif
private:
    uint8_t *m_ptr; ///< the pointer to the raw queue
    mutable message_desc_type m_message_desc; ///< description of the currently pulled message
};

typedef base_queue queue_type;
typedef boost::shared_ptr<queue_type> pqueue_type;

/**
 * The simple queue
 */
class simple_queue : public base_queue
{
    friend class message::message<simple_queue>;
public:
    typedef message::message<simple_queue> message_type;
    explicit simple_queue(void *ptr);
    simple_queue(const id_type qid, void *ptr, const size_t cpct);
protected:
    virtual message_desc_type push_message(const void *data, const size_t size); ///< push new message to the queue
    virtual message_desc_type get_message() const; ///< get a message from the queue
    virtual void pop_message(const message_desc_type& message_desc); ///< pop a message from the queue
    virtual pmessage_type make_message(void *ptr, const size_t cpct) const; ///< make an empty message
    virtual pmessage_type make_message(void *ptr) const; ///< make an empty message
};

/**
 * The base shared queue that has a lot of readers
 */
class base_shared_queue : public base_queue
{
    friend class message::message<base_shared_queue>;
public:
    typedef message::message<base_shared_queue> message_type;
    explicit base_shared_queue(void *ptr);
    base_shared_queue(const id_type qid, void *ptr, const size_t cpct);
    virtual size_t count() const; ///< get the count of messages
    virtual size_t size() const; ///< get the size of the queue 
    static size_t static_size(const size_t cpct)
    {
        return HEADER_SIZE + base_queue::static_size(cpct);
    }
protected:
    enum
    {
        SUBS_COUNT_OFFSET = 0,
        SUBS_COUNT_SIZE   = sizeof(uint32_t),
        COUNTER_OFFSET    = SUBS_COUNT_OFFSET + SUBS_COUNT_SIZE,
        COUNTER_SIZE      = sizeof(uint32_t),
        HEADER_SIZE       = COUNTER_OFFSET + COUNTER_SIZE,
    };
    size_t subscriptions_count() const; ///< get the count of subscriptions
    void subscriptions_count(const size_t value); ///< set the count of subscriptions
    size_t inc_subscriptions_count(); ///< increase the count of subscriptions
    size_t dec_subscriptions_count(); ///< reduce the count of subscriptions
    uint32_t counter() const; ///< get the counter of pushed messages
    void counter(const uint32_t value); ///< set the counter of pushed messages
    virtual pos_type head() const; /// get the head of the queue
    virtual garbage_info_type clean_messages(); ///< collect garbage
    virtual message_desc_type push_message(const void *data, const size_t size); ///< push new message to the queue
    virtual message_desc_type get_message() const; ///< get a message from the queue
    virtual void pop_message(const message_desc_type& message_desc); ///< pop a message from the queue
    virtual pmessage_type make_message(void *ptr, const size_t cpct) const; ///< make an empty message
    virtual pmessage_type make_message(void *ptr) const; ///< make an empty message
private:
    uint8_t *m_ptr; ///< the pointer to the raw queue
protected:
    pos_type m_head; ///< the self head of the queue
    uint32_t m_counter; ///< the counter of popped messages
};

/**
 * The shared queue that has a lot of readers and writers
 */
class shared_queue : public base_shared_queue
{
public:
    explicit shared_queue(void *ptr);
    shared_queue(const id_type qid, void *ptr, const size_t cpct);
    virtual ~shared_queue();
};

/**
 * The shared queue that is used only for write operation
 */
class unreadable_shared_queue : public base_shared_queue
{
public:
    explicit unreadable_shared_queue(void *ptr);
    unreadable_shared_queue(const id_type qid, void *ptr, const size_t cpct);
protected:
    virtual garbage_info_type clean_messages(); ///< collect garbage
};

/**
 * Create a queue
 * @param qid the identifier of the queue
 * @param ptr the pointer to the header of the queue
 * @param cpct the capacity of the queue
 * @param pqueue the parent queue
 * @return the queue
 */
template <typename Queue>
inline pqueue_type create(const id_type qid, void *ptr, const size_t cpct,
    pqueue_type pqueue = pqueue_type())
{
    QBUS_UNUSED(pqueue);
    return boost::make_shared<Queue>(qid, ptr, cpct);
}

/**
 * Open a queue
 * @param ptr the pointer to the header of the queue
 * @param pqueue the parent queue
 * @return the queue
 */
template <typename Queue>
inline pqueue_type open(void *ptr, pqueue_type pqueue = pqueue_type())
{
    QBUS_UNUSED(pqueue);
    return boost::make_shared<Queue>(ptr); 
}

/**
 * The shared queue that has a lot of readers and writers and supports
 * dynamic subscriber connection and subscriber disconnection
 */
class smart_shared_queue : public base_shared_queue
{
    friend pqueue_type create<smart_shared_queue>(const id_type qid, void *ptr, 
        const size_t cpct, pqueue_type pqueue);
    friend pqueue_type open<smart_shared_queue>(void *ptr, pqueue_type pqueue);
    typedef message::service_message<message_type> service_message_type;
    typedef service_message_type::code_type service_code_type;
public:
    explicit smart_shared_queue(void *ptr);
    smart_shared_queue(const id_type qid, void *ptr, const size_t cpct);
    virtual ~smart_shared_queue();
    virtual size_t size() const; ///< get the size of the queue 
    static size_t static_size(const size_t cpct)
    {
        return HEADER_SIZE + base_shared_queue::static_size(cpct);
    }
protected:
    smart_shared_queue(void *ptr, pqueue_type pqueue);
    smart_shared_queue(const id_type qid, void *ptr, const size_t cpct, pqueue_type pqueue);
    enum
    {
        FREE_SPACE_OFFSET = 0,
        FREE_SPACE_SIZE   = sizeof(uint32_t),
        HEADER_SIZE       = FREE_SPACE_OFFSET + FREE_SPACE_SIZE,
    };
    void initialize(); ///< initialize the queue
    size_t free_space() const; ///< get the free space of the queue
    void free_space(const size_t value); ///< set the free space of the queue
    size_t inc_free_space(const size_t value); ///< increase the free space of the queue
    size_t dec_free_space(const size_t value); ///< reduce the the free space of the queue
    virtual garbage_info_type clean_messages(); ///< collect garbage
    virtual message_desc_type push_message(const void *data, const size_t size); ///< push new message to the queue
    virtual message_desc_type get_message() const; ///< get a message from the queue
    void push_service_message(service_code_type code); ///< push a service message to the queue
    virtual region_type get_free_region(region_type *pprev_region = NULL) const; ///< get the next free region
private:
    enum state_type
    {
        ST_UNKNOWN,
        ST_PUSH_SPECIAL_MESSAGE
    };
    uint8_t *m_ptr; ///< the pointer to the raw queue
    state_type m_state; ///< the current state of the queue
};

template < >
inline pqueue_type create<smart_shared_queue>(const id_type qid, void *ptr, 
    const size_t cpct, pqueue_type pqueue)
{
    boost::shared_ptr<smart_shared_queue> result(new smart_shared_queue(qid, ptr, cpct, pqueue));
    return result;
}

template < >
inline pqueue_type open<smart_shared_queue>(void *ptr, pqueue_type pqueue)
{
    boost::shared_ptr<smart_shared_queue> result(new smart_shared_queue(ptr, pqueue));
    return result;
}

} //namespace queue

typedef queue::pqueue_type pqueue_type;

} //namespace qbus

#endif /* QBUS_QUEUE_H */

