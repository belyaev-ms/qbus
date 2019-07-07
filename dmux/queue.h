#ifndef DMUX_QUEUE_H
#define DMUX_QUEUE_H

#include <stddef.h>
#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "dmux/message.h"

namespace dmux
{
namespace queue
{

typedef uint32_t id_type;
typedef uint32_t pos_type;
typedef message::pmessage_type pmessage_type;

/**
 * The base queue
 */
class base_queue
{
public:
    base_queue();
    explicit base_queue(void *ptr);
    base_queue(const size_t sz, const size_t cnt);
    base_queue(void *ptr, const size_t sz, const size_t cnt);
    virtual ~base_queue();
    virtual const size_t size() const; ///< get the size of the queue
    const pmessage_type add(const void *data, const size_t sz); ///< add data to the queue
    const pmessage_type make(); ///< make a empty message
    const pmessage_type get(); ///< get the next message
    void pop(); ///< remove the next message
    const pmessage_type get(const pos_type index); ///< get a message that has the index
    void pop(const pos_type index); ///< remove a message that has the index
    const id_type id() const; ///< get the identifier of the queue
    virtual const size_t count() const; ///< get the count of messages
    const size_t slice_size() const; ///< get the size of a slice
    void clear(); ///< clear the queue
    virtual void clean(); ///< collect garbage
    static size_t static_size(const size_t sz, const size_t cnt)
    {
        return HEADER_SIZE + sz * cnt;
    }
protected:
    enum
    {
        ID_OFFSET         = 0,
        ID_SIZE           = sizeof(id_type),
        SLICESIZE_OFFSET  = ID_OFFSET + ID_SIZE,
        SLICESIZE_SIZE    = sizeof(uint32_t),
        SLICECOUNT_OFFSET = SLICESIZE_OFFSET + SLICESIZE_SIZE,
        SLICECOUNT_SIZE   = sizeof(uint32_t),
        COUNT_OFFSET      = SLICECOUNT_OFFSET + SLICECOUNT_SIZE,
        COUNT_SIZE        = sizeof(uint32_t),
        HEAD_OFFSET       = COUNT_OFFSET + COUNT_SIZE,
        HEAD_SIZE         = sizeof(pos_type),
        TAIL_OFFSET       = HEAD_OFFSET + HEAD_SIZE,
        TAIL_SIZE         = HEAD_SIZE,
        DATA_OFFSET       = TAIL_OFFSET + TAIL_SIZE,
        HEADER_SIZE       = DATA_OFFSET
    };
    void id(const id_type id); ///< set the identifier of the queue
    const size_t slice_count() const; ///< get the count of slices
    virtual const pos_type head() const; /// get the head of the queue
    void head(const pos_type pos); /// get the head of the queue
    const pos_type tail() const; /// get the tail of the queue
    void tail(const pos_type pos); /// get the tail of the queue
    void count(const size_t cnt); ///< set the count of messages
    virtual const pmessage_type get_message(const pos_type index) = 0; ///< get a message
    virtual const pmessage_type take_message(const pos_type index); ///< take a message
    virtual const pos_type next_index(const pos_type index) const = 0; ///< get the index of the next slice
    virtual void *slice(const pos_type index) const; ///< get pointer to the i-st slice
    virtual const bool remove(const pos_type beg, const pos_type end); ///< remove the messages in the range [beg, end)
    virtual void do_after_remove(const size_t cnt); ///< do something after removing
private:
    base_queue(const base_queue&);
    base_queue& operator=(const base_queue&);
    void slice_size(const size_t size); ///< set the size of a slice
    void slice_count(const size_t count); ///< set the count of slices
    const pmessage_type do_get(const pos_type index, const size_t cnt); ///< get a message
protected:
    uint8_t *m_ptr; ///< the pointer to the raw queue
    const bool m_owner; ///< the ownership flag of the raw queue
};

typedef base_queue queue_type;
typedef boost::shared_ptr<queue_type> pqueue_type;

/**
 * The simple queue
 */
class simple_queue : public base_queue
{
public:
    typedef message::message<simple_queue> message_type;
    explicit simple_queue(void *ptr);
    simple_queue(const size_t size, const size_t count);
    simple_queue(void *ptr, const size_t size, const size_t count);
protected:
    virtual const pmessage_type get_message(const pos_type index); ///< get a message
    virtual const pos_type next_index(const pos_type index) const; ///< get the index of the next slice
};

typedef simple_queue simple_queue_t;
typedef boost::shared_ptr<simple_queue_t> psimple_queue_t;

/**
 * The queue that is organized as a circular doubly linked list
 */
class queue : public base_queue
{
public:
    typedef message::message<queue> message_type;
    explicit queue(void *ptr);
    queue(const size_t sz, const size_t cnt);
    queue(void *ptr, const size_t size, const size_t count);
    virtual const size_t size() const; ///< get the size of the queue
    static size_t static_size(const size_t sz, const size_t cnt)
    {
        return HEADER_SIZE + (sz + slice_type::static_size()) * cnt;
    }
protected:
    void init(); ///< initialize the queue
    virtual const pmessage_type get_message(const pos_type index); ///< get a message
    virtual const pos_type next_index(const pos_type index) const; ///< get the index of the next slice
    virtual void *slice(const pos_type index) const; ///< get pointer to the i-st slice
    virtual const bool remove(const pos_type beg, const pos_type end); ///< remove the messages in the range [beg, end)
private:
    /** The extended slice block */
    class slice_type
    {
    public:
        slice_type(const queue& queue, const pos_type index);
        const pos_type prev() const;
        void prev(const pos_type pos);
        const pos_type next() const;
        void next(const pos_type pos);
        static size_t static_size();
    private:
        enum
        {
            PREV_OFFSET   = 0,
            PREV_SIZE     = sizeof(pos_type),
            NEXT_OFFSET   = PREV_OFFSET + PREV_SIZE,
            NEXT_SIZE     = PREV_SIZE,
            OVERHEAD_SIZE = NEXT_OFFSET + NEXT_SIZE
        };
    private:
        uint8_t *m_ptr;
    };
};

/**
 * The shared queue that has a lot of readers
 */
class shared_queue : public base_queue
{
public:
    typedef message::message<shared_queue> message_type;
    explicit shared_queue(void *ptr);
    shared_queue(const size_t size, const size_t count);
    shared_queue(void *ptr, const size_t size, const size_t count);
    virtual ~shared_queue();
    virtual const size_t size() const; ///< get the size of the queue
    virtual const size_t count() const; ///< get the count of messages
    virtual void clean(); ///< collect garbage
    static size_t static_size(const size_t sz, const size_t cnt)
    {
        return HEADER_SIZE + (sz + slice_type::static_size()) * cnt;
    }
protected:
    enum
    {
        SUBS_COUNT_OFFSET = base_queue::DATA_OFFSET,
        SUBS_COUNT_SIZE   = sizeof(uint32_t),
        DATA_OFFSET       = SUBS_COUNT_OFFSET + SUBS_COUNT_SIZE,
        HEADER_SIZE       = DATA_OFFSET
    };
    void init(); ///< initialize the queue
    const size_t subscriptions_count() const; ///< get the count of subscriptions
    void subscriptions_count(const size_t count); ///< set the count of subscriptions
    const size_t inc_subscriptions_count(); ///< increase the count of subscriptions
    const size_t dec_subscriptions_count(); ///< reduce the count of subscriptions
    virtual const pos_type head() const; /// get the head of the queue
    virtual const pmessage_type get_message(const pos_type index); ///< get a message
    virtual const pmessage_type take_message(const pos_type index); ///< take a message
    virtual const pos_type next_index(const pos_type index) const; ///< get the index of the next slice
    virtual const bool remove(const pos_type beg, const pos_type end); ///< remove the messages in the range [beg, end)
    virtual void *slice(const pos_type index) const; ///< get pointer to the i-st slice
    virtual void do_after_remove(const size_t count); ///< do something after remove
private:
    /** The extended slice block */
    class slice_type
    {
    public:
        slice_type(const shared_queue& queue, const pos_type index);
        const size_t ref_count() const;
        void ref_count(const size_t count);
        const size_t inc_ref_count();
        static size_t static_size();
    private:
        enum
        {
            REF_COUNT_OFFSET = 0,
            REF_COUNT_SIZE   = sizeof(uint32_t),
            OVERHEAD_SIZE    = REF_COUNT_OFFSET + REF_COUNT_SIZE
        };
    private:
        uint8_t *m_ptr;
    };
private:
    pos_type m_head; ///< the self head of the queue
    uint32_t m_count; ///< the count of messages
};

typedef shared_queue shared_queue_type;
typedef boost::shared_ptr<shared_queue_type> pshared_queue_type;

/**
 * Make the queue into the memory of the heap
 * @param size the size of a slice
 * @param count the count of slices
 * @return the queue
 */
template <typename Queue>
const pqueue_type make(const size_t size, const size_t count)
{
    return boost::make_shared<Queue>(size, count);
}

/**
 * Make the queue into the memory of the heap
 * @param ptr the pointer to the head of the queue
 * @param size the size of a slice
 * @param count the count of slices
 * @return the queue
 */
template <typename Queue>
const pqueue_type make(const void *ptr, const size_t size, const size_t count)
{
    return boost::make_shared<Queue>(ptr, size, count);
}

} //namespace queue

typedef queue::pqueue_type pqueue_type;

} //namespace dmux

#endif /* DMUX_QUEUE_H */

