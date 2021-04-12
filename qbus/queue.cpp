#include "qbus/queue.h"
#include <stdlib.h>
#include <string.h>
#include <boost/interprocess/detail/atomic.hpp>
#ifdef QBUS_TEST_ENABLED
#include <iostream>
#endif

namespace qbus
{
namespace queue
{

//==============================================================================
//  base_queue
//==============================================================================
/**
 * Constructor
 */
base_queue::base_queue() :
    m_ptr(NULL),
    m_owner(false)
{
}

/**
 * Constructor
 * @param sz the size of a slice
 * @param cnt the count of slices
 */
base_queue::base_queue(const size_t sz, const size_t cnt) :
    m_ptr(NULL),
    m_owner(true)
{
    // create a temporary header on the stack
    {
        uint8_t tmp[HEADER_SIZE] = { 0 };
        m_ptr = tmp;
        slice_size(sz);
        slice_count(cnt);
        m_ptr = reinterpret_cast<uint8_t*>(malloc(size()));
    }
    slice_size(sz);
    slice_count(cnt);
    clear();
    slice_size(sz);
    slice_count(cnt);
}

/**
 * Constructor
 * @param ptr the pointer to the header of the queue
 */
base_queue::base_queue(void *ptr) :
    m_ptr(reinterpret_cast<uint8_t*>(ptr)),
    m_owner(false)
{
}

/**
 * Constructor
 * @param ptr the pointer to the header of the queue
 * @param sz the size of a slice
 * @param cnt the count of slices
 */
base_queue::base_queue(void *ptr, const size_t sz, const size_t cnt) :
    m_ptr(reinterpret_cast<uint8_t*>(ptr)),
    m_owner(false)
{
    clear();
    slice_size(sz);
    slice_count(cnt);
}

/**
 * Destructor
 */
//virtual
base_queue::~base_queue()
{
    if (m_owner && m_ptr != NULL)
    {
        free(m_ptr);
        m_ptr = NULL;
    }
}

/**
 * Clear the queue
 */
void base_queue::clear()
{
    if (m_ptr != NULL)
    {
        head(0);
        tail(0);
        count(0);
    }
}

/**
 * Collect garbage
 */
//virtual
void base_queue::clean()
{
}

/**
 * Get the size of the queue
 * @return the size of the queue
 */
//virtual
const size_t base_queue::size() const
{
    return HEADER_SIZE + slice_size() * slice_count();
}

/**
 * Add data to the queue
 * @param data the pointer to the data
 * @param sz the size of the data
 * @return the message
 */
const pmessage_type base_queue::add(const void *data, const size_t sz)
{
    clean();
    if (sz <= (slice_count() - count()) * (slice_size() - message::HEADER_SIZE))
    {
        const pmessage_type pmessage = make();
        if (pmessage)
        {
            pmessage->pack(data, sz);
            return pmessage;
        }
    }
    return pmessage_type();
}

/**
 * Make a empty message
 * @return the empty message
 */
const pmessage_type base_queue::make()
{
    if (count() < slice_count())
    {
        const size_t tl = tail();
        tail(next_index(tl));
        count(base_queue::count() + 1);
        return get_message(tl);
    }
    return pmessage_type();
}

/**
 * Get a message
 * @param index the index of the message
 * @param cnt the count of messages
 * @return the message
 */
const pmessage_type base_queue::do_get(const pos_type index, const size_t cnt)
{
    if (cnt > 0)
    {
        pmessage_type pmessage = get_message(index);
        if (pmessage && !(pmessage->flags() & message::FLG_TAIL))
        {
            ///@todo get rid of recursion
            pmessage->attach(do_get(next_index(index), cnt - 1));
        }
        return pmessage;
    }
    return pmessage_type();
}

/**
 * Get the next message
 * @return the next message
 */
const pmessage_type base_queue::get()
{
    return get(head());
}

/**
 * Remove the next message
 */
void base_queue::pop()
{
    pop(head());
}

/**
 * Get a message that has the index
 * @param index the index of the message
 * @return the message
 */
const pmessage_type base_queue::get(const pos_type index)
{
    ///@todo the first message must have FLG_HEAD
    return do_get(index, count());
}

/**
 * Take a message
 * @param index the index of the message
 * @return the message
 */
//virtual
const pmessage_type base_queue::take_message(const pos_type index)
{
    return get_message(index);
}

/**
 * Remove a message that has the index
 * @param index the index of the message
 */
void base_queue::pop(const pos_type index)
{
    ///@todo the first message must have FLG_HEAD
    size_t cnt = count();
    if (cnt > 0)
    {
        pos_type pos = index;
        pmessage_type pmessage = take_message(pos);
        while (--cnt > 0 && pmessage &&
            !(pmessage->flags() & message::FLG_TAIL))
        {
            pos = next_index(pos);
            pmessage = take_message(pos);
        }
        pos = next_index(pos);
        remove(index, pos);
        do_after_remove(count() - cnt);
    }
}

/**
 * Get the size of a slice
 * @return the size of a slice
 */
const size_t base_queue::slice_size() const
{
    return NULL == m_ptr ? 0 :
        *reinterpret_cast<const uint32_t*>(m_ptr + SLICESIZE_OFFSET);
}

/**
 * Set the size of a slice
 * @param size the size of a slice
 */
void base_queue::slice_size(const size_t size)
{
    *reinterpret_cast<uint32_t*>(m_ptr + SLICESIZE_OFFSET) = size;
}

/**
 * Get the count of slices
 * @return the count of slices
 */
const size_t base_queue::slice_count() const
{
    return *reinterpret_cast<const uint32_t*>(m_ptr + SLICECOUNT_OFFSET);
}

/**
 * Set the count of slices
 * @param cnt the count of slices
 */
void base_queue::slice_count(const size_t cnt)
{
    *reinterpret_cast<uint32_t*>(m_ptr + SLICECOUNT_OFFSET) = cnt;
}

/**
 * Get the count of messages
 * @return the count of messages
 */
//virtual
const size_t base_queue::count() const
{
    return *reinterpret_cast<const uint32_t*>(m_ptr + COUNT_OFFSET);
}

/**
 * Set the count of messages
 * @param cnt the count of messages
 */
void base_queue::count(const size_t cnt)
{
    *reinterpret_cast<uint32_t*>(m_ptr + COUNT_OFFSET) = cnt;
}

/**
 * Get the head of the queue
 * @return the head of the queue
 */
//virtual
const pos_type base_queue::head() const
{
    return *reinterpret_cast<const pos_type*>(m_ptr + HEAD_OFFSET);
}

/**
 * Set the head of the queue
 * @param head the head of the queue
 */
void base_queue::head(const pos_type head)
{
    *reinterpret_cast<pos_type*>(m_ptr + HEAD_OFFSET) = head;
}

/**
 * Get the tail of the queue
 * @return the tail of the queue
 */
const pos_type base_queue::tail() const
{
    return *reinterpret_cast<const pos_type*>(m_ptr + TAIL_OFFSET);
}

/**
 * Set the tail of the queue
 * @param tl the tail of the queue
 */
void base_queue::tail(const pos_type tl)
{
    *reinterpret_cast<pos_type*>(m_ptr + TAIL_OFFSET) = tl;
}

/**
 * Get the pointer to the index-st slice
 * @param i the position of the slice
 * @return the pointer to the index-st slice
 */
//virtual
void *base_queue::slice(const pos_type index) const
{
    return index > slice_count() ? NULL : m_ptr + DATA_OFFSET + slice_size() * index;
}

/**
 * Remove the messages in the range [beg, end)
 * @param beg the begin of the range
 * @param end the end of the range
 * @return the result of the removing
 */
//virtual
const bool base_queue::remove(const pos_type beg, const pos_type end)
{
    if (beg == head())
    {
        head(end);
        return true;
    }
    if (end == tail())
    {
        tail(beg);
        return true;
    }
    return false;
}

/**
 * Do something after removing
 * @param cnt the count of removed messages
 */
//virtual
void base_queue::do_after_remove(const size_t cnt)
{
    count(count() - cnt);
}

//==============================================================================
//  simple_queue
//==============================================================================
/**
 * Constructor
 * @param size the size of a slice
 * @param count the count of slices
 */
simple_queue::simple_queue(const size_t size, const size_t count) :
    base_queue(size, count)
{
}

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
 * @param ptr the pointer to the header of the queue
 * @param size the size of a slice
 * @param count the count of slices
 */
simple_queue::simple_queue(void *ptr, const size_t size, const size_t count) :
    base_queue(ptr, size, count)
{
}

/**
 * Get a message
 * @param index the index of the message
 * @return the next message
 */
//virtual
const pmessage_type simple_queue::get_message(const pos_type index)
{
    return boost::make_shared<message_type>(this, slice(index));
}

/**
 * Get the index of the next slice
 */
//virtual
const pos_type simple_queue::next_index(const pos_type index) const
{
    return (index + 1) % slice_count();
}

//==============================================================================
//  Queue
//==============================================================================
/**
 * Constructor
 * @param sz the size of a slice
 * @param cnt the count of slices
 */
queue::queue(const size_t sz, const size_t cnt) :
    base_queue(sz - slice_type::static_size(), cnt)
{
    init();
}

/**
 * Constructor
 * @param ptr the pointer to the header of the queue
 */
queue::queue(void *ptr) :
    base_queue(ptr)
{
}

/**
 * Constructor
 * @param ptr the pointer to the header of the queue
 * @param sz the size of a slice
 * @param cnt the count of slices
 */
queue::queue(void *ptr, const size_t sz, const size_t cnt) :
    base_queue(ptr, sz - slice_type::static_size(), cnt)
{
    init();
}

/**
 * Get the size of the queue
 * @return the size of the queue
 */
//virtual
const size_t queue::size() const
{
    return HEADER_SIZE + (slice_size() + slice_type::static_size()) * slice_count();
}

/**
 * Initialize the queue
 */
void queue::init()
{
    const size_t count = slice_count();
    {
        slice_type slice(*this, 0);
        slice.prev(count - 1);
        slice.next(1);
    }
    for (size_t i = 1; i < count - 1; ++i)
    {
        slice_type slice(*this, i);
        slice.prev(i - 1);
        slice.next(i + 1);
    }
    {
        slice_type slice(*this, count - 1);
        slice.prev(count - 2);
        slice.next(0);
    }
}

/**
 * Get a message
 * @param index the index of the message
 * @return the message
 */
//virtual
const pmessage_type queue::get_message(const pos_type index)
{
    return boost::make_shared<message_type>(this, slice(index));
}

/**
 * Get the index of the next slice
 */
//virtual
const pos_type queue::next_index(const pos_type index) const
{
    const slice_type slice(*this, index);
    return slice.next();
}

/**
 * Get the pointer to the index-st slice
 * @param i the position of the slice
 * @return the pointer to the index-st slice
 */
//virtual
void *queue::slice(const pos_type index) const
{
    uint8_t *ptr = reinterpret_cast<uint8_t*>(base_queue::slice(index));
    return NULL == ptr ? NULL : ptr + slice_type::static_size() * (index + 1);
}

/**
 * Remove the messages in the range [beg, end)
 * @param beg the begin of the range
 * @param end the end of the range
 * @param count the count of messages to be removed
 * @return the result of the removing
 */
//virtual
const bool queue::remove(const pos_type beg, const pos_type end)
{
    if (!base_queue::remove(beg, end))
    {
        // extract the range [beg; end) from the queue
        slice_type slice_beg(*this, beg);
        slice_type slice_end(*this, end);
        const pos_type beg_prev = slice_beg.prev();
        const pos_type end_prev = slice_end.prev();
        slice_type(*this, beg_prev).next(end);
        slice_end.prev(beg_prev);
        // include the range before the tail of the queue
        const pos_type tl = tail();
        slice_type slice_tail(*this, tl);
        const pos_type tail_prev = slice_tail.prev();
        slice_type(*this, tail_prev).next(beg);
        slice_beg.prev(tail_prev);
        slice_type(*this, end_prev).next(tl);
        slice_tail.prev(end_prev);
        tail(beg);
    }
#ifdef QBUS_TEST_ENABLED
    std::cout << "{" << std::endl;
    pos_type i = head();
    size_t count = 0;
    do
    {
        slice_type slice(*this, i);
        std::cout << "\t" << slice.prev() << " <- " << i << " -> " << slice.next() << std::endl;
        i = slice.next();
        ++count;
    } while(i != head());
    std::cout << "} : " << count << std::endl;
    assert(count == slice_count());
    std::cout << "{" << std::endl;
    i = head();
    count = 0;
    do
    {
        slice_type slice(*this, i);
        std::cout << "\t" << slice.prev() << " <- " << i << " -> " << slice.next() << std::endl;
        i = slice.prev();
        ++count;
    } while(i != head());
    assert(count == slice_count());
    std::cout << "} : " << count << std::endl;
#endif
    return true;
}

//==============================================================================
//  Queue::slice_type
//==============================================================================
/**
 * Constructor
 * @param queue the queue
 * @param index the index of the slice
 */
queue::slice_type::slice_type(const queue& queue, const pos_type index) :
    m_ptr(NULL)
{
    uint8_t *ptr = reinterpret_cast<uint8_t*>(queue.slice(index));
    m_ptr = NULL == ptr ? NULL : ptr - OVERHEAD_SIZE;
}

/**
 * Get the index of the previous slice
 * @return the index of the previous slice
 */
const pos_type queue::slice_type::prev() const
{
    return *reinterpret_cast<const pos_type*>(m_ptr + PREV_OFFSET);
}

/**
 * Set the index of the previous slice
 * @param pos the index of the previous slice
 */
void queue::slice_type::prev(const pos_type pos)
{
    *reinterpret_cast<pos_type*>(m_ptr + PREV_OFFSET) = pos;
}

/**
 * Get the index of the next slice
 * @return the index of the next slice
 */
const pos_type queue::slice_type::next() const
{
    return *reinterpret_cast<const pos_type*>(m_ptr + NEXT_OFFSET);
}

/**
 * Set the index of the next slice
 * @param pos the index of the next slice
 */
void queue::slice_type::next(const pos_type pos)
{
    *reinterpret_cast<pos_type*>(m_ptr + NEXT_OFFSET) = pos;
}

//static
size_t queue::slice_type::static_size()
{
    return OVERHEAD_SIZE;
}

//==============================================================================
//  shared_queue
//==============================================================================
/**
 * Constructor
 * @param size the size of a slice
 * @param count the count of slices
 */
shared_queue::shared_queue(const size_t size, const size_t count) :
    base_queue(size, count),
    m_head(-1),
    m_count(0)
{
    init();
}

/**
 * Constructor
 * @param ptr the pointer to the header of the queue
 */
shared_queue::shared_queue(void *ptr) :
    base_queue(ptr),
    m_head(-1),
    m_count(0)
{
    inc_subscriptions_count();
}

/**
 * Constructor
 * @param ptr the pointer to the header of the queue
 * @param size the size of a slice
 * @param count the count of slices
 */
shared_queue::shared_queue(void *ptr, const size_t size, const size_t count) :
    base_queue(ptr, size, count),
    m_head(-1),
    m_count(0)
{
    init();
}

/**
 * Destructor
 */
//virtual
shared_queue::~shared_queue()
{
    dec_subscriptions_count();
}

/**
 * Get the size of the queue
 * @return the size of the queue
 */
//virtual
const size_t shared_queue::size() const
{
    return HEADER_SIZE + (slice_size() + slice_type::static_size()) * slice_count();
}

/**
 * Initialize the queue
 */
void shared_queue::init()
{
    subscriptions_count(0);
    const size_t count = slice_count();
    for (size_t i = 0; i < count; ++i)
    {
        slice_type slice(*this, i);
        slice.ref_count(0);
    }
}

/**
 * Get the count of subscriptions
 * @return the count of subscriptions
 */
const size_t shared_queue::subscriptions_count() const
{
    const size_t count = *reinterpret_cast<uint32_t*>(m_ptr + SUBS_COUNT_OFFSET);
    return count > 0 ? count : 1;
}

/**
 * Set the count of subscriptions
 * @param count the count of subscriptions
 */
void shared_queue::subscriptions_count(const size_t count)
{
    *reinterpret_cast<uint32_t*>(m_ptr + SUBS_COUNT_OFFSET) = count;
}

/**
 * Increase the count of subscriptions
 * @return the count of subscriptions
 */
const size_t shared_queue::inc_subscriptions_count()
{
    return ++(*reinterpret_cast<uint32_t*>(m_ptr + SUBS_COUNT_OFFSET));
}

/**
 * Reduce the count of subscriptions
 * @return the count of subscriptions
 */
const size_t shared_queue::dec_subscriptions_count()
{
    return --(*reinterpret_cast<uint32_t*>(m_ptr + SUBS_COUNT_OFFSET));
}

/**
 * Get a message
 * @param index the index of the message
 * @return the message
 */
//virtual
const pmessage_type shared_queue::get_message(const pos_type index)
{
    return boost::make_shared<message_type>(this, slice(index));
}

/**
 * Take a message
 * @param index the index of the message
 * @return the message
 */
//virtual
const pmessage_type shared_queue::take_message(const pos_type index)
{
    const pmessage_type pmessage = base_queue::take_message(index);
    slice_type slice(*this, index);
    slice.inc_ref_count();
    return pmessage;
}

/**
 * Get the index of the next slice
 */
//virtual
const pos_type shared_queue::next_index(const pos_type index) const
{
    return (index + 1) % slice_count();
}

/**
 * Get the head of the queue
 * @return the head of the queue
 */
//virtual
const pos_type shared_queue::head() const
{
    return m_head != pos_type(-1) ? m_head : base_queue::head();
}

/**
 * Get the count of messages
 * @return the count of messages
 */
//virtual
const size_t shared_queue::count() const
{
    return base_queue::count() - m_count;
}

/**
 * Remove the messages in the range [beg, end)
 * @param beg the begin of the range
 * @param end the end of the range
 * @return the result of the removing
 */
//virtual
const bool shared_queue::remove(const pos_type beg, const pos_type end)
{
//    if (m_head != pos_type(-1) && head() == end)
//    {
//        m_head = -1;
//        return false;
//    }
    m_head = end;
    return false;
}

/**
 * Get the pointer to the index-st slice
 * @param i the position of the slice
 * @return the pointer to the index-st slice
 */
//virtual
void *shared_queue::slice(const pos_type index) const
{
    return index > slice_count() ? NULL : m_ptr +
        DATA_OFFSET + (slice_size() + slice_type::static_size()) * index +
        slice_type::static_size();
}

/**
 * Collect garbage
 */
//virtual
void shared_queue::clean()
{
    size_t cnt = count();
    if (cnt > 0)
    {
        size_t hidden_count = 0;
        const pos_type head = base_queue::head();
        pos_type pos = head;
        slice_type slice(*this, pos);
        while (slice.ref_count() >= subscriptions_count() && cnt-- > 0)
        {
            pos = next_index(pos);
            slice.ref_count(0);
            slice = slice_type(*this, pos);
            ++hidden_count;
        }
        if (hidden_count > 0 && base_queue::remove(head, pos))
        {
            m_count = base_queue::count() - cnt;
        }
    }
}

/**
 * Do something after remove
 * @param count the count of removed messages
 */
//virtual
void shared_queue::do_after_remove(const size_t count)
{
    m_count += count;
}

//==============================================================================
//  shared_queue::slice_type
//==============================================================================
/**
 * Constructor
 * @param queue the queue
 * @param index the index of the slice
 */
shared_queue::slice_type::slice_type(const shared_queue& queue, const pos_type index) :
    m_ptr(NULL)
{
    uint8_t *ptr = reinterpret_cast<uint8_t*>(queue.slice(index));
    m_ptr = NULL == ptr ? NULL : ptr - OVERHEAD_SIZE;
}

/**
 * Get the count of references
 * @return the count of references
 */
const size_t shared_queue::slice_type::ref_count() const
{
    return *reinterpret_cast<const uint32_t*>(m_ptr + REF_COUNT_OFFSET);
}

/**
 * Set the count of references
 * @param count the index of the previous slice
 */
void shared_queue::slice_type::ref_count(const size_t count)
{
    *reinterpret_cast<uint32_t*>(m_ptr + REF_COUNT_OFFSET) = count;
}

/**
 * Increase the count of references
 * @return the count of references
 */
const size_t shared_queue::slice_type::inc_ref_count()
{
    return boost::interprocess::ipcdetail::atomic_inc32(reinterpret_cast<uint32_t*>(m_ptr + REF_COUNT_OFFSET)) + 1;
}

//static
size_t shared_queue::slice_type::static_size()
{
    return OVERHEAD_SIZE;
}

} //namespace queue
} //namespace qbus