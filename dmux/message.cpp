#include "dmux/message.h"
#include <string.h>

namespace dmux
{
namespace message
{

/**
 * Constructor
 */
base_message::base_message() :
    m_ptr(NULL),
    m_size(0)
{
}

/**
 * Constructor
 * @param ptr the pointer to the raw message
 * @param sz the size of the raw message
 */
base_message::base_message(void *ptr, const size_t sz) :
    m_ptr(reinterpret_cast<uint8_t*>(ptr)),
    m_size(sz)
{
}

/**
 * Destructor
 */
//virtual
base_message::~base_message()
{

}

/**
 * Get the identifier of the message
 * @return the identifier of the message
 */
const id_type base_message::id() const
{
    return *reinterpret_cast<const id_type*>(m_ptr + ID_OFFSET);
}

/**
 * Set the identifier of the message
 * @param id the identifier of the message
 */
void base_message::id(const id_type id)
{
    *reinterpret_cast<id_type*>(m_ptr + ID_OFFSET) = id;
}

/**
 * Get the flags of the message
 * @return the flags of the message
 */
const flags_type base_message::flags() const
{
    return *reinterpret_cast<const flags_type*>(m_ptr + FLAGS_OFFSET);
}

/**
 * Set the flags of the message
 * @param flags the flags of the message
 */
void base_message::flags(const flags_type flags)
{
    *reinterpret_cast<flags_type*>(m_ptr + FLAGS_OFFSET) = flags;
}

/**
 * Get the size of data of the message
 * @return the size of data of the message
 */
const size_t base_message::size() const
{
    return *reinterpret_cast<const uint32_t*>(m_ptr + COUNT_OFFSET);
}

/**
 * Set the size of data of the message
 * @param sz the size of data of the message
 */
void base_message::size(const size_t sz)
{
    *reinterpret_cast<uint32_t*>(m_ptr + COUNT_OFFSET) = sz;
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
 * Clear the message
 */
void base_message::clear()
{
    if (m_ptr != NULL && m_size > 0)
    {
        memset(m_ptr, 0, m_size);
    }
}

/**
 * Attach a message
 * @param pmessage the message
 */
void base_message::attach(const pmessage_type pmessage)
{
    if (pmessage && !(pmessage->flags() & FLG_HEAD))
    {
        m_pmessage = pmessage;
    }
}

/**
 * Pack the data to the message
 * @param source the pointer to the source of data
 * @param size the size of the data
 */
void base_message::pack(const void *source, const size_t size)
{
    flags(do_pack(source, size));
}

/**
 * Pack the data to the message
 * @param source the pointer to the source of data
 * @param sz the size of the data
 * @return the flags of the message
 */
const flags_type base_message::do_pack(const void *source, const size_t sz)
{
    const uint8_t *ptr = reinterpret_cast<const uint8_t*>(source);
    const size_t cpct = capacity();
    if (sz <= cpct)
    {
        size(sz);
        if (ptr != NULL)
        {
            memcpy(data(), ptr, sz);
        }
        return FLG_HEAD | FLG_TAIL;
    }
    else
    {
        size(cpct);
        if (ptr != NULL)
        {
            memcpy(data(), ptr, cpct);
        }
        m_pmessage = make_message();
        const flags_type flgs = m_pmessage->do_pack(ptr + cpct, sz - cpct);
        m_pmessage->flags(flgs & ~FLG_HEAD);
        return FLG_HEAD;
    }
}

/**
 * Unpack the data from the message
 * @param dest the pointer to the destination of data
 */
void base_message::unpack(void *dest) const
{
    uint8_t *ptr = reinterpret_cast<uint8_t*>(dest);
    const size_t sz = size();
    memcpy(ptr, data(), sz);
    if (m_pmessage)
    {
        m_pmessage->unpack(ptr + sz);
    }
}

/**
 * Get the size of attached data
 * @return the size of attached data
 */
const size_t base_message::data_size() const
{
    return m_pmessage ? size() + m_pmessage->data_size() : size();
}

/**
 * Get the capacity of the message
 * @return the capacity of the message
 */
const size_t base_message::capacity() const
{
    return m_size - HEADER_SIZE;
}

} //namespace message
} //namespace dmux