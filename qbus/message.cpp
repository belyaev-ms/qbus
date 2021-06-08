#include "qbus/message.h"
#include <string.h>
#include <boost/interprocess/detail/atomic.hpp>

namespace qbus
{
namespace message
{

/**
 * Get the source identifier
 * @return the source identifier
 */
sid_type get_sid()
{
    static sid_type sid = getpid();
    return sid;
}

/**
 * Get the current timestamp
 * @return the current timestamp
 */
size_t get_timestamp()
{
    struct timespec ts = { 0 };
    while (clock_gettime(CLOCK_MONOTONIC, &ts) != 0);
    return ts.tv_sec;
}
    
//==============================================================================
//  base_message
//==============================================================================
/**
 * Constructor
 * @param ptr the pointer to the raw message
 */
base_message::base_message(void *ptr) :
    m_ptr(reinterpret_cast<uint8_t*>(ptr))
{
}

/**
 * Constructor
 * @param ptr the pointer to the raw message
 * @param cpct the capacity of the message
 */
base_message::base_message(void *ptr, const size_t cpct) :
    m_ptr(reinterpret_cast<uint8_t*>(ptr))
{
    memset(ptr, 0, HEADER_SIZE);
    capacity(cpct);
    sid(get_sid());
    timestamp(get_timestamp());
}

/**
 * Get the source identifier of the message
 * @return the source identifier of the message
 */
sid_type base_message::sid() const
{
    return *reinterpret_cast<const sid_type*>(m_ptr + SID_OFFSET);
}

/**
 * Set the source identifier of the message
 * @param value the source identifier of the message
 */
void base_message::sid(const sid_type value)
{
    *reinterpret_cast<sid_type*>(m_ptr + SID_OFFSET) = value;
}

/**
 * Get the source identifier of the message
 * @return the timestamp of the message
 */
size_t base_message::timestamp() const
{
    return *reinterpret_cast<const uint32_t*>(m_ptr + TS_OFFSET);
}

/**
 * Set the timestamp of the message
 * @param value the timestamp of the message
 */
void base_message::timestamp(const size_t value)
{
    *reinterpret_cast<uint32_t*>(m_ptr + TS_OFFSET) = value;
}

/**
 * Get the tag of the message
 * @return the tag of the message
 */
tag_type base_message::tag() const
{
    return *reinterpret_cast<const tag_type*>(m_ptr + TAG_OFFSET);
}

/**
 * Set the tag of the message
 * @param value the tag of the message
 */
void base_message::tag(const tag_type value)
{
    *reinterpret_cast<tag_type*>(m_ptr + TAG_OFFSET) = value;
}

/**
 * Get the flags of the message
 * @return the flags of the message
 */
flags_type base_message::flags() const
{
    return *reinterpret_cast<const flags_type*>(m_ptr + FLAGS_OFFSET);
}

/**
 * Set the flags of the message
 * @param value the flags of the message
 */
void base_message::flags(const flags_type value)
{
    *reinterpret_cast<flags_type*>(m_ptr + FLAGS_OFFSET) = value;
}

/**
 * Get the capacity of the message
 * @return the capacity of the message
 */
size_t base_message::capacity() const
{
    return *reinterpret_cast<const uint32_t*>(m_ptr + CAPACITY_OFFSET);
}

/**
 * Set the capacity of the message
 * @param value the capacity of the message
 */
void base_message::capacity(const size_t value)
{
    *reinterpret_cast<uint32_t*>(m_ptr + CAPACITY_OFFSET) = value;
}

/**
 * Get the reference counter of the message
 * @return the reference counter of the message
 */
size_t base_message::counter() const
{
    return boost::interprocess::ipcdetail::atomic_read32(reinterpret_cast<uint32_t*>(m_ptr + COUNTER_OFFSET));
}

/**
 * Increment reference counter of the message
 * @return the reference counter of the message
 */
size_t base_message::inc_counter()
{
    return boost::interprocess::ipcdetail::atomic_inc32(reinterpret_cast<uint32_t*>(m_ptr + COUNTER_OFFSET)) + 1;
}

/**
 * Decrement reference counter of the message
 * @return the reference counter of the message
 */
size_t base_message::dec_counter()
{
    return boost::interprocess::ipcdetail::atomic_dec32(reinterpret_cast<uint32_t*>(m_ptr + COUNTER_OFFSET)) - 1;
}

/**
 * Get the pointer to data of the message
 * @return the pointer to data of the message
 */
void *base_message::data()
{
    return m_ptr + DATA_OFFSET;
}

/**
 * Get the pointer to data of the message
 * @return the pointer to data of the message
 */
const void *base_message::data() const
{
    return m_ptr + DATA_OFFSET;
}

/**
 * Pack the data to the message
 * @param source the pointer to the source of data
 * @param size the size of the data
 * @return the size of the packed data
 */
size_t base_message::pack(const void *source, const size_t size)
{
    const uint8_t *ptr = reinterpret_cast<const uint8_t*>(source);
    const size_t cpct = capacity();
    if (size <= cpct)
    {
        memcpy(data(), ptr, size);
        flags(FLG_HEAD | FLG_TAIL);
        return size;
    }
    else
    {
        size_t rest_size = 0;
        memcpy(data(), ptr, cpct);
        flags(FLG_HEAD);
        if (m_pmessage)
        {
            rest_size = m_pmessage->pack(ptr + cpct, size - cpct);
            m_pmessage->flags(m_pmessage->flags() & ~FLG_HEAD);
        }
        return cpct + rest_size;
    }
}

/**
 * Unpack the data from the message
 * @param dest the pointer to the destination of data
 * @return size the size of the unpacked data
 */
size_t base_message::unpack(void *dest) const
{
    uint8_t *ptr = reinterpret_cast<uint8_t*>(dest);
    const size_t cpct = capacity();
    memcpy(ptr, data(), cpct);
    return cpct + (m_pmessage ? m_pmessage->unpack(ptr + cpct) : 0);
}

/**
 * Get the size of attached data
 * @return the size of attached data
 */
size_t base_message::data_size() const
{
    return total_capacity();
}

/**
 * Get total capacity of all chained messages
 * @return the total capacity of all chained messages
 */
size_t base_message::total_capacity() const
{
    return capacity() + (m_pmessage ? m_pmessage->total_capacity() : 0);
}

/**
 * Get the size of the message
 * @return the size of attached data
 */
size_t base_message::size() const
{
    return static_size(capacity());
}

/**
 * Get total size of all chained messages
 * @return the total size of all chained messages
 */
size_t base_message::total_size() const
{
    return static_size(capacity()) + (m_pmessage ? m_pmessage->total_size() : 0);
}

/**
 * Attach the message
 * @param pmessage the attached message
 */
void base_message::attach(pmessage_type pmessage)
{
    m_pmessage = pmessage;
}

} //namespace message

} //namespace qbus
