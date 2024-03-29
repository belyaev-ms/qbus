#ifndef QBUS_MESSAGE_H
#define QBUS_MESSAGE_H

#include <stddef.h>
#include <stdint.h>
#include <boost/shared_ptr.hpp>

namespace qbus
{
    
namespace message
{
    
enum
{
    FLG_HEAD = 1,
    FLG_TAIL = 2
};

typedef uint32_t tag_type;
typedef uint32_t flags_type;
typedef pid_t sid_type;
typedef sid_type (*get_sid_impl_type)(void);

sid_type get_sid(); ///< get the source identifier
size_t get_timestamp(); ///< get the current timestamp
void init_get_sid(get_sid_impl_type pfunc); ///< initialize `get_sid`

/**
 * The message
 */
class base_message
{
public:
    typedef boost::shared_ptr<base_message> pmessage_type;
    
    sid_type sid() const; //< get the surce identifier of the message
    size_t timestamp() const; ///< get the timestamp of the message
    flags_type flags() const; ///< get the flags of the message
    tag_type tag() const; ///< get the tag of the message
    void tag(const tag_type value); ///< set the tag of the message
    size_t counter() const; ///< get the reference counter of the message
    void counter(const size_t value); ///< set the reference counter of the message
    size_t inc_counter(); ///< increment the reference counter of the message
    size_t dec_counter(); ///< decrement the reference counter of the message
    size_t pack(const void *source, const size_t size); ///< pack the data to the message
    size_t unpack(void *dest) const; ///< unpack the data from the message
    size_t data_size() const; ///< get the size of attached data
    size_t size() const; ///< get the size of the message
    size_t total_size() const; ///< get total size of all chained messages
    void attach(pmessage_type pmessage); ///< attach the message
#ifdef QBUS_TEST_ENABLED
    void print() const; ///< print the information of message attributes
#endif
    static size_t static_size(const size_t cpct)
    {
        return HEADER_SIZE + cpct;
    }
    static size_t static_capacity(const size_t size)
    {
        return size > HEADER_SIZE ? size - HEADER_SIZE : 0;
    }
protected:
    explicit base_message(void *ptr);
    base_message(void *ptr, const size_t cpct);
    void *data(); ///< get the pointer to data of the message
    const void *data() const; ///< get the pointer to data of the message
private:
    enum
    {
        FLAGS_OFFSET    = 0,
        FLAGS_SIZE      = sizeof(flags_type),
        CAPACITY_OFFSET = FLAGS_OFFSET + FLAGS_SIZE,
        CAPACITY_SIZE   = sizeof(uint32_t),
        SID_OFFSET      = CAPACITY_OFFSET + CAPACITY_SIZE,
        SID_SIZE        = sizeof(sid_type),
        TAG_OFFSET      = SID_OFFSET + SID_SIZE,
        TAG_SIZE        = sizeof(tag_type),
        COUNTER_OFFSET  = TAG_OFFSET + TAG_SIZE,
        COUNTER_SIZE    = sizeof(uint32_t),
        TS_OFFSET       = COUNTER_OFFSET + COUNTER_SIZE,
        TS_SIZE         = sizeof(uint32_t),
        DATA_OFFSET     = TS_OFFSET + TS_SIZE
    };
protected:
    enum
    {
        HEADER_SIZE     = DATA_OFFSET
    };
private:
    void sid(const sid_type value); //< set the surce identifier of the message
    void timestamp(const size_t value); ///< set the timestamp of the message
    void flags(const flags_type value); ///< set the flags of the message
    size_t capacity() const; ///< get the capacity of the message
    size_t total_capacity() const; ///< get total capacity of all chained messages
    void capacity(const size_t value); ///< set the capacity of the message
private:
    uint8_t *m_ptr; ///< the pointer to the raw message
    pmessage_type m_pmessage; ///< the next message
};

typedef base_message message_type;
typedef base_message::pmessage_type pmessage_type;

/**
 * The message of a queue
 */
template <typename Queue>
class message : public base_message
{
public:
    friend Queue;
    typedef Queue queue_type;
    explicit message(void *ptr);
    message(void *ptr, const size_t cpct);
private:
    typedef typename queue_type::region_type region_type;
    typedef typename queue_type::message_desc_type message_desc_type;
    static message_desc_type static_make_message(queue_type& queue, const void *data, const size_t size);
    static message_desc_type static_get_message(const queue_type& queue);
};

//==============================================================================
//  message
//==============================================================================
/**
 * Make a message in the queue
 * @param queue the queue
 * @param data the pointer to data
 * @param size the size of data
 * @return the message description
 */
template <typename Queue>
//static 
typename message<Queue>::message_desc_type 
    message<Queue>::static_make_message(queue_type& queue, const void *data, const size_t size)
{
    pmessage_type pmessage;
    pmessage_type plast_message;
    region_type region;
    region_type *pprev_region = NULL;
    size_t rest = size;
    while (rest > 0)
    {
        size_t part = 0;
        do
        {
            region = queue.get_free_region(pprev_region);
            pprev_region = &region;
            if (0 == region.second)
            {
                return std::make_pair(pmessage_type(), 0);
            }
        } while (region.second <= HEADER_SIZE);
        part = std::min(rest, static_capacity(region.second));
        pmessage_type pnext_message = queue.make_message(queue.data(region.first), part);
        if (!pmessage)
        {
            pmessage = pnext_message;
        }
        else
        {
            plast_message->attach(pnext_message);
        }
        plast_message = pnext_message;
        rest -= part;
    }
    const size_t packed_size = pmessage->pack(data, size);
    assert(packed_size == size);
    return std::make_pair(pmessage, region.first + plast_message->size());
}

/**
 * Get a message in the queue
 * @param queue the queue
 * @return the message description
 */
template <typename Queue>
//static 
typename message<Queue>::message_desc_type 
    message<Queue>::static_get_message(const queue_type& queue)
{
    pmessage_type pmessage;
    pmessage_type plast_message;
    region_type region;
    region_type *pprev_region = NULL;
    do
    {
        do
        {
            region = queue.get_busy_region(pprev_region);
            pprev_region = &region;
            if (0 == region.second)
            {
                return std::make_pair(pmessage_type(), 0);
            }
        } while (region.second <= HEADER_SIZE);
        pmessage_type pnext_message = queue.make_message(queue.data(region.first));
        if (!pmessage)
        {
            pmessage = pnext_message;
            assert(pmessage->flags() & FLG_HEAD);
        }
        else
        {
            plast_message->attach(pnext_message);
        }
        plast_message = pnext_message;
    } while (!(plast_message->flags() & FLG_TAIL));
    return std::make_pair(pmessage, region.first + plast_message->size());
}

/**
 * Constructor
 * @param ptr the pointer to the raw message
 */
template <typename Queue>
message<Queue>::message(void *ptr) :
    base_message(ptr)
{
}

/**
 * Constructor
 * @param ptr the pointer to the raw message
 * @param cpct the capacity of the message
 */
template <typename Queue>
message<Queue>::message(void *ptr, const size_t cpct) :
    base_message(ptr, cpct)
{
}

} //namespace message

typedef message::pmessage_type pmessage_type;

} //namespace qbus


#endif /* QBUS_MESSAGE_H */

