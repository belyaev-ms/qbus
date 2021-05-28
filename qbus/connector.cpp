#include "qbus/connector.h"
#include <boost/smart_ptr/detail/spinlock.hpp>

namespace qbus
{

namespace connector
{

//==============================================================================
//  base_connector
//==============================================================================
/**
 * Constructor
 * @param name the name of the connector
 * @param type the type of the connector
 */
base_connector::base_connector(const std::string& name, const direction_type type) :
    m_name(name),
    m_type(type),
    m_opened(false)
{
}

/**
 * Destructor
 */
//virtual
base_connector::~base_connector()
{
}

/**
 * Get the name of the connector
 * @return the name of the connector
 */
const std::string& base_connector::name() const
{
    return m_name;
}

/**
 * Get the type of the connecter
 * @return the type of the connecter
 */
direction_type base_connector::type() const
{
    return m_type;
}

/**
 * Create the connector
 * @param cid the identifier of the connector
 * @param size the size of a queue
 * @return the result of the creating
 */
bool base_connector::create(const id_type cid, const size_t size)
{
    if (!m_opened)
    {
        m_opened = do_create(cid, size);
        return m_opened;
    }
    return false;
}

/**
 * Open the connector
 * @return the result of the opening
 */
bool base_connector::open()
{
    if (!m_opened)
    {
        m_opened = do_open();
        return m_opened;
    }
    return false;
}

/**
 * Push data to the connector
 * @param tag the tag of the data
 * @param data the data
 * @param size the size of the data
 * @return result of the pushing
 */
bool base_connector::push(const tag_type tag, const void *data, const size_t size)
{
    return (m_opened && (CON_OUT == m_type || CON_BIDIR == m_type)) ?
        do_push(tag, data, size) : false;
}

/**
 * Push data to the connector
 * @param tag the tag of the data
 * @param data the data
 * @param size the size of the data
 * @param timeout the allowable timeout of the pushing
 * @return result of the pushing
 */
bool base_connector::push(const tag_type tag, const void *data, const size_t size,
    const struct timespec& timeout)
{
    return (m_opened && (CON_OUT == m_type || CON_BIDIR == m_type)) ?
        do_timed_push(tag, data, size, timeout) : false;
}

/**
 * Get the next message from the connector
 * @return the message
 */
const pmessage_type base_connector::get() const
{
    return (m_opened && (CON_IN == m_type || CON_BIDIR == m_type)) ?
        do_get() : pmessage_type();
}

/**
 * Get the next message from the connector
 * @param timeout the allowable timeout of the getting
 * @return the message
 */
const pmessage_type base_connector::get(const struct timespec& timeout) const
{
    return (m_opened && (CON_IN == m_type || CON_BIDIR == m_type)) ?
        do_timed_get(timeout) : pmessage_type();
}

/**
 * Remove the next message from the connector
 * @return the result of the removing
 */
bool base_connector::pop()
{
    return (m_opened && (CON_IN == m_type || CON_BIDIR == m_type)) ?
        do_pop() : false;
}

/**
 * Remove the next message from the connector
 * @param timeout the allowable timeout of the removing
 * @return the result of the removing
 */
bool base_connector::pop(const struct timespec& timeout)
{
    return (m_opened && (CON_IN == m_type || CON_BIDIR == m_type)) ?
        do_timed_pop(timeout) : false;
}

/**
 * Push data to the connector
 * @param tag the tag of the data
 * @param data the data
 * @param size the size of the data
 * @param timeout the allowable timeout of the pushing
 * @return result of the pushing
 */
//virtual
bool base_connector::do_timed_push(const tag_type tag, const void *data, 
    const size_t size, const struct timespec& timeout)
{
    const struct timespec ts = get_monotonic_time() + timeout;
    unsigned int k = 0;
    while (get_monotonic_time() < ts)
    {
        if (do_push(tag, data, size))
        {
            return true;
        }
        boost::detail::yield(k++);
    }
    return false;
}

/**
 * Get the next message from the connector
 * @param timeout the allowable timeout of the getting
 * @return the message
 */
//virtual
const pmessage_type base_connector::do_timed_get(const struct timespec& timeout) const
{
    const struct timespec ts = get_monotonic_time() + timeout;
    pmessage_type pmessage;
    unsigned int k = 0;
    while (get_monotonic_time() < ts)
    {
        pmessage = do_get();
        if (pmessage)
        {
            return pmessage;
        }
        boost::detail::yield(k++);
    }
    return pmessage;
}

/**
 * Remove the next message from the connector
 * @param timeout the allowable timeout of the removing
 * @return the result of the removing
 */
//virtual
bool base_connector::do_timed_pop(const struct timespec& timeout)
{
    const struct timespec ts = get_monotonic_time() + timeout;
    unsigned int k = 0;
    while (get_monotonic_time() < ts)
    {
        if (do_pop())
        {
            return true;
        }
        boost::detail::yield(k++);
    }
    return false;
}

//==============================================================================
//  shared_connector
//==============================================================================
/**
 * Constructor
 * @param name the name of the connector
 * @param type the type of the connector
 */
shared_connector::shared_connector(const std::string& name, const direction_type type) :
    base_connector(name, type),
    m_memory(name)
{
}

/**
 * Create the connector
 * @param cid the identifier of the connector
 * @param size the size of a queue
 * @return the result of the creating
 */
//virtual
bool shared_connector::do_create(const id_type cid, const size_t size)
{
    return create_memory(size);
}

/**
 * Open the connector
 * @return the result of the opening
 */
//virtual
bool shared_connector::do_open()
{
    return open_memory();
}

/**
 * Create the shared memory
 * @param cid the identifier of the connector
 * @param size the size of a queue
 * @return the result of the creating
 */
bool shared_connector::create_memory(const size_t size)
{
    return m_memory.create(memory_size(size));
}

/**
 * Open the shared memory
 * @return the result of the opening
 */
bool shared_connector::open_memory()
{
    return m_memory.open();
}

/**
 * Get the pointer to the shared memory
 * @return the pointer to the shared memory
 */
//virtual
void *shared_connector::get_memory()
{
    return m_memory.get();
}

} //namespace connector
} //namespace qbus