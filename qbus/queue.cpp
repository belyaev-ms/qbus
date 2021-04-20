#include "qbus/queue.h"
#include <stdlib.h>
#include <string.h>
#include <boost/make_shared.hpp>
#include <boost/interprocess/detail/atomic.hpp>

namespace qbus
{
namespace queue
{

//==============================================================================
//  base_queue
//==============================================================================
/**
 * Constructor
 * @param ptr the pointer to the header of the queue
 */
base_queue::base_queue(void *ptr) :
    m_ptr(reinterpret_cast<uint8_t*>(ptr))
{
}

/**
 * Constructor
 * @param qid the identifier of the queue
 * @param ptr the pointer to the header of the queue
 * @param cpct the capacity of the queue
 */
base_queue::base_queue(const id_type qid, void *ptr, const size_t cpct) :
    m_ptr(reinterpret_cast<uint8_t*>(ptr))
{
    id(qid);
    capacity(cpct);
    clear();
}

/**
 * Destructor
 */
//virtual
base_queue::~base_queue()
{
}

/**
 * Get the identifier of the queue
 * @return the identifier of the queue
 */
id_type base_queue::id() const
{
    return *reinterpret_cast<const id_type*>(m_ptr + ID_OFFSET);
}

/**
 * Set the identifier of the queue
 * @param value the identifier of the queue
 */
void base_queue::id(const id_type value)
{
    *reinterpret_cast<id_type*>(m_ptr + ID_OFFSET) = value;
}

/**
 * Get the capacity of the queue
 * @return the capacity of the queue
 */
size_t base_queue::capacity() const
{
    return *reinterpret_cast<const uint32_t*>(m_ptr + CAPACITY_OFFSET);
}

/**
 * Set the capacity of of the queue
 * @param value the capacity of the queue
 */
void base_queue::capacity(const size_t value)
{
    *reinterpret_cast<uint32_t*>(m_ptr + CAPACITY_OFFSET) = value;
}

/**
 * Get the count of messages
 * @return the count of messages
 */
//virtual
size_t base_queue::count() const
{
    return *reinterpret_cast<const uint32_t*>(m_ptr + COUNT_OFFSET);
}

/**
 * Set the count of messages
 * @param value the count of messages
 */
void base_queue::count(const size_t value)
{
    *reinterpret_cast<uint32_t*>(m_ptr + COUNT_OFFSET) = value;
}

/**
 * Check the queue is empty 
 * @return 
 */
bool base_queue::empty() const
{
    return count() == 0;
}

/**
 * Get the head of the queue
 * @return the head of the queue
 */
//virtual
pos_type base_queue::head() const
{
    return *reinterpret_cast<const pos_type*>(m_ptr + HEAD_OFFSET);
}

/**
 * Set the head of the queue
 * @param value the head of the queue
 */
void base_queue::head(const pos_type value)
{
    *reinterpret_cast<pos_type*>(m_ptr + HEAD_OFFSET) = value % capacity();
}

/**
 * Get the tail of the queue
 * @return the tail of the queue
 */
pos_type base_queue::tail() const
{
    return *reinterpret_cast<const pos_type*>(m_ptr + TAIL_OFFSET);
}

/**
 * Set the tail of the queue
 * @param value the tail of the queue
 */
void base_queue::tail(const pos_type value)
{
    *reinterpret_cast<pos_type*>(m_ptr + TAIL_OFFSET) = value % capacity();
}

/**
 * Get pointer to data region of the queue
 * @param pos the position in the queue
 * @return pointer of data region of the queue
 */
void *base_queue::data(const pos_type pos) const
{
    return m_ptr + DATA_OFFSET + pos;
}

/**
 * Clear the queue
 */
void base_queue::clear()
{
    head(0);
    tail(0);
    count(0);
}

/**
 * Get the size of the queue 
 * @return the size of the queue 
 */
//virtual 
size_t base_queue::size() const
{
    return static_size(capacity());
}

/**
 * Collect garbage
 */
//virtual 
void base_queue::clean()
{
}

/**
 * Push data to the queue
 * @param tag the tag of the message
 * @param data the data of the message
 * @param size the size of the message
 * @return the execution result
 */
bool base_queue::push(const tag_type tag, const void *data, const size_t size)
{
    clean();
    message_desc_type message_desc = push_message(data, size);
    if (message_desc.first)
    {
        message_desc.first->tag(tag);
        tail(message_desc.second);
        count(base_queue::count() + 1);
        return true;
    }
    return false;
}

/**
 * Get the next free region
 * @param pprev_region the pointer to the previous free region
 * @return the next free region
 */
typename base_queue::region_type base_queue::get_free_region(region_type *pprev_region) const
{
    const size_t cpct = capacity();
    const pos_type hd = base_queue::head();
    if (NULL == pprev_region)
    {
        const pos_type tl = tail();
        return ((base_queue::count() == 0) || hd < tl) ?
            region_type(tl, cpct - tl) : 
            region_type(tl, hd - tl);
    }
    else
    {
        const pos_type tl = (pprev_region->first + pprev_region->second) % cpct;
        return hd < tl ?
            region_type(tl, cpct - tl) : 
            region_type(tl, hd - tl);
    }
}

/**
 * Get the next busy region
 * @param pprev_region the pointer to the previous busy region
 * @return the next busy region
 */
typename base_queue::region_type base_queue::get_busy_region(region_type *pprev_region) const
{
    const size_t cpct = capacity();
    const pos_type tl = tail();
    if (NULL == pprev_region)
    {
        const pos_type hd = head();
        return (empty() || hd < tl) ?
            region_type(hd, tl - hd) : 
            region_type(hd, cpct - hd);
    }
    else
    {
        const pos_type hd = (pprev_region->first + pprev_region->second) % cpct;
        return hd < tl ?
            region_type(hd, tl - hd) : 
            region_type(hd, cpct - hd);
    }
}

/**
 * Get the next message
 * @return the next message
 */
const pmessage_type base_queue::get() const
{
    if (count() > 0)
    {
        pmessage_type pmessage = get_message().first;
        assert(pmessage);
        return pmessage;
    }
    return pmessage_type();
}

/**
 * Remove the next message
 */
void base_queue::pop()
{
    const size_t cnt = count();
    if (cnt > 0)
    {
        const message_desc_type message_desc = get_message();
        pop_message(message_desc);
    }
}

//==============================================================================
//  simple_queue
//==============================================================================
/**
 * Constructor
 * @param ptr the pointer to the header of the queue
 */
simple_queue::simple_queue(void *ptr) :
    base_queue(ptr)
{
}

/**
 * Constructor
 * @param qid the identifier of the queue
 * @param ptr the pointer to the header of the queue
 * @param cpct the capacity of the queue
 */
simple_queue::simple_queue(const id_type qid, void *ptr, const size_t cpct) :
    base_queue(qid, ptr, cpct)
{
}

/**
 * Push new message to the queue
 * @param data the data of the message
 * @param size the size of data
 * @return the the message
 */
//virtual
typename simple_queue::message_desc_type simple_queue::push_message(const void *data, const size_t size)
{
    return message_type::static_make_message(*this, data, size);
}

/**
 * Get a message from the queue
 * @return the the message
 */
//virtual
typename simple_queue::message_desc_type simple_queue::get_message() const
{
    return message_type::static_get_message(*this);
}

/**
 * Pop a message from the queue
 * @param message_desc the description of the message
 */
//virtual 
void simple_queue::pop_message(const message_desc_type& message_desc)
{
    message_desc.first->inc_counter();
    head(message_desc.second);
    count(count() - 1); ///< !!!! dec count && inc count
}

/**
 * Make an empty message
 * @param ptr the pointer to raw message
 * @param cpct the capacity of the message
 * @return the empty message
 */
//virtual 
pmessage_type simple_queue::make_message(void *ptr, const size_t cpct) const
{
    return boost::make_shared<message_type>(ptr, cpct);
}

/**
 * Make an empty message
 * @param ptr the pointer to raw message
 * @return the empty message
 */
//virtual 
pmessage_type simple_queue::make_message(void *ptr) const
{
    return boost::make_shared<message_type>(ptr);
}

//==============================================================================
//  base_shared_queue
//==============================================================================
/**
 * Constructor
 * @param ptr the pointer to the header of the queue
 */
base_shared_queue::base_shared_queue(void *ptr) :
    base_queue(reinterpret_cast<uint8_t*>(ptr) + HEADER_SIZE),
    m_ptr(reinterpret_cast<uint8_t*>(ptr)),
    m_head(-1)
{
    m_counter = counter();
}

/**
 * Constructor
 * @param qid the identifier of the queue
 * @param ptr the pointer to the header of the queue
 * @param cpct the capacity of the queue
 */
base_shared_queue::base_shared_queue(const id_type qid, void *ptr, const size_t cpct) :
    base_queue(qid, reinterpret_cast<uint8_t*>(ptr) + HEADER_SIZE, cpct),
    m_ptr(reinterpret_cast<uint8_t*>(ptr)),
    m_head(-1),
    m_counter(0)
{
    counter(0);
}

/**
 * Get the count of subscriptions
 * @return the count of subscriptions
 */
size_t base_shared_queue::subscriptions_count() const
{
    return boost::interprocess::ipcdetail::atomic_read32(reinterpret_cast<uint32_t*>(m_ptr + SUBS_COUNT_OFFSET));
}

/**
 * Set the count of subscriptions
 * @param value the count of subscriptions
 */
void base_shared_queue::subscriptions_count(const size_t value)
{
    boost::interprocess::ipcdetail::atomic_write32(reinterpret_cast<uint32_t*>(m_ptr + SUBS_COUNT_OFFSET), value);
}

/**
 * Increase the count of subscriptions
 * @return the count of subscriptions
 */
size_t base_shared_queue::inc_subscriptions_count()
{
    return boost::interprocess::ipcdetail::atomic_inc32(reinterpret_cast<uint32_t*>(m_ptr + SUBS_COUNT_OFFSET)) + 1;
}

/**
 * Reduce the count of subscriptions
 * @return the count of subscriptions
 */
size_t base_shared_queue::dec_subscriptions_count()
{
    return boost::interprocess::ipcdetail::atomic_dec32(reinterpret_cast<uint32_t*>(m_ptr + SUBS_COUNT_OFFSET)) - 1;
}

/**
 * Get the counter of pushed messages
 * @return the counter of pushed messages
 */
uint32_t base_shared_queue::counter() const
{
    return *reinterpret_cast<const uint32_t*>(m_ptr + COUNTER_OFFSET);
}

/**
 * Set the count of subscriptions
 * @param value the count of subscriptions
 */
void base_shared_queue::counter(const uint32_t value)
{
    *reinterpret_cast<uint32_t*>(m_ptr + COUNTER_OFFSET) = value;
}

/**
 * Get the size of the queue 
 * @return the size of the queue 
 */
//virtual 
size_t base_shared_queue::size() const
{
    return static_size(capacity());
}

/**
 * Get the head of the queue
 * @return the head of the queue
 */
//virtual
pos_type base_shared_queue::head() const
{
    return m_head != pos_type(-1) ? m_head : base_queue::head();
}

/**
 * Get the count of messages
 * @return the count of messages
 */
//virtual
size_t base_shared_queue::count() const
{
    return counter() - m_counter;
}

template <typename T>
class rollback
{
public:
    rollback(T& value) : 
        m_value(value),
        m_store(value)
    {}
    ~rollback()
    {
        m_value = m_store;
    }
private:
    T& m_value;
    T m_store;
};

/**
 * Collect garbage
 */
//virtual 
void base_shared_queue::clean()
{
    size_t cnt = base_queue::count();
    if (cnt > 0)
    {
        rollback<pos_type> head(m_head);
        rollback<uint32_t> counter(m_counter);
        m_head = pos_type(-1);
        --m_counter;
        while (cnt-- > 0)
        {
            message_desc_type message_desc = get_message();
            if (!message_desc.first || message_desc.first->counter() < subscriptions_count())
            {
                break;
            }
            base_queue::head(message_desc.second);
            base_queue::count(cnt);
        }
    }
}

/**
 * Push new message to the queue
 * @param data the data of the message
 * @param size the size of data
 * @return the the message
 */
//virtual
typename base_shared_queue::message_desc_type base_shared_queue::push_message(const void *data, const size_t size)
{
    message_desc_type message_desc = message_type::static_make_message(*this, data, size);
    if (message_desc.first)
    {
        counter(counter() + 1);
    }
    return message_desc;
}

/**
 * Get a message from the queue
 * @return the the message
 */
//virtual
typename base_shared_queue::message_desc_type base_shared_queue::get_message() const
{
    return message_type::static_get_message(*this);
}

/**
 * Pop a message from the queue
 * @param message_desc the description of the message
 */
//virtual 
void base_shared_queue::pop_message(const message_desc_type& message_desc)
{
    message_desc.first->inc_counter();
    m_head = message_desc.second % capacity();
    ++m_counter;
}

/**
 * Make an empty message
 * @param ptr the pointer to raw message
 * @param cpct the capacity of the message
 * @return the empty message
 */
//virtual 
pmessage_type base_shared_queue::make_message(void *ptr, const size_t cpct) const
{
    return boost::make_shared<message_type>(ptr, cpct);
}

/**
 * Make an empty message
 * @param ptr the pointer to raw message
 * @return the empty message
 */
//virtual 
pmessage_type base_shared_queue::make_message(void *ptr) const
{
    return boost::make_shared<message_type>(ptr);
}

//==============================================================================
//  shared_queue
//==============================================================================
/**
 * Constructor
 * @param ptr the pointer to the header of the queue
 */
shared_queue::shared_queue(void *ptr) : 
    base_shared_queue(ptr)
{
    inc_subscriptions_count();
}

/**
 * Constructor
 * @param qid the identifier of the queue
 * @param ptr the pointer to the header of the queue
 * @param cpct the capacity of the queue
 */
shared_queue::shared_queue(const id_type qid, void *ptr, const size_t cpct) : 
    base_shared_queue(qid, ptr, cpct)
{
    subscriptions_count(1);
}

/**
 * Destructor
 */
//virtual
shared_queue::~shared_queue()
{
    dec_subscriptions_count();
}

//==============================================================================
//  unreadable_shared_queue
//==============================================================================
/**
 * Constructor
 * @param ptr the pointer to the header of the queue
 */
unreadable_shared_queue::unreadable_shared_queue(void *ptr) :
    base_shared_queue(ptr)
{
}

/**
 * Constructor
 * @param qid the identifier of the queue
 * @param ptr the pointer to the header of the queue
 * @param cpct the capacity of the queue
 */
unreadable_shared_queue::unreadable_shared_queue(const id_type qid, void *ptr, const size_t cpct) :
    base_shared_queue(qid, ptr, cpct)
{
}

} //namespace queue

} //namespace qbus