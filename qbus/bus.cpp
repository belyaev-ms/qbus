#include "qbus/bus.h"
#include <boost/lexical_cast.hpp>

namespace qbus
{

namespace bus
{

//==============================================================================
//  base_bus
//==============================================================================
/**
 * Constructor
 * @param name the name of the bus
 */
base_bus::base_bus(const std::string& name) :
    m_name(name)
{
}

/**
 * Destructor
 */
//virtual
base_bus::~base_bus()
{
}

/**
 * Get the name of the connector
 * @return the name of the connector
 */
const std::string& base_bus::name() const
{
    return m_name;
}

/**
 * Create the bus
 * @param spec the specification of the bus
 * @return the result of the creating
 */
bool base_bus::create(const specification_type& spec)
{
    if (!m_opened)
    {
        m_opened = do_create(spec);
        return m_opened;
    }
    return false;
}

/**
 * Open the bus
 * @return the result of the opening
 */
bool base_bus::open()
{
    if (!m_opened)
    {
        m_opened = do_open();
        return m_opened;
    }
    return false;
}

/**
 * Check if the bus is enabled
 * @return result of the checking
 */
bool base_bus::enabled() const
{
    return m_opened;
}

/**
 * Make new connector
 * @param id the identifier of the connector
 * @return new connector
 */
pconnector_type base_bus::make_connector(const id_type id) const
{
    pconnector_type pconnector = make_connector(m_name + boost::lexical_cast<std::string>(id));
    if (pconnector->open())
    {
        return pconnector;
    }
    const specification_type sp = spec();
    if (sp.capacity_factor > 0)
    {
        size_type old_capacity = !m_pconnectors.empty() ? front_connector()->capacity() : 0;
        size_type new_capacity = std::max(sp.min_capacity, old_capacity * (sp.capacity_factor + 100) / 100);
        new_capacity = std::max(new_capacity, sp.max_capacity);
        if (new_capacity > old_capacity && pconnector->create(sp.id, new_capacity))
        {
            return pconnector;
        }
    }
    return NULL;
}

/**
 * Add new connector to the bus
 * @return result of the adding
 */
bool base_bus::add_connector() const
{
    controlblock_type& cb = get_controlblock();
    if (cb.front_connector_id + 1 != cb.back_connector_id)
    {
        pconnector_type pconnector = make_connector(cb.front_connector_id + 1);
        if (pconnector)
        {
            m_pconnectors.push_front(pconnector);
            ++cb.front_connector_id;
            return true;
        }
    }
    return false;
}

/**
 * Remove the back connector from the bus
 * @return result of the removing
 */
bool base_bus::remove_connector() const
{
    if (m_pconnectors.size() > 1)
    {
        controlblock_type& cb = get_controlblock();
        m_pconnectors.pop_back();
        ++cb.back_connector_id;
        return true;
    }
    return false;
}

/**
 * Get the front connector
 * @return the front connector
 */
pconnector_type base_bus::front_connector() const
{
    return m_pconnectors.front();
}

/**
 * Get the back connector
 * @return the back connector
 */
pconnector_type base_bus::back_connector() const
{
    return m_pconnectors.back();
}

/**
 * Get the specification of the bus
 * @return the specification of the bus
 */
const specification_type& base_bus::spec() const
{
    return get_spec();
}

/**
 * Push data to the bus
 * @param tag the tag of the data
 * @param data the data
 * @param size the size of the data
 * @return result of the pushing
 */
bool base_bus::push(const tag_type tag, const void *data, const size_t size)
{
    return m_opened && do_push(tag, data, size);
}

/**
 * Push data to the bus
 * @param tag the tag of the data
 * @param data the data
 * @param size the size of the data
 * @param timeout the allowable timeout of the pushing
 * @return result of the pushing
 */
bool base_bus::push(const tag_type tag, const void *data, const size_t size,
    const struct timespec& timeout)
{
    return m_opened && do_timed_push(tag, data, size, timeout);
}

/**
 * Get the next message from the bus
 * @return the message
 */
const pmessage_type base_bus::get() const
{
    if (m_opened)
    {
        return do_get();
    }
    return NULL;
}

/**
 * Get the next message from the bus
 * @param timeout the allowable timeout of the getting
 * @return the message
 */
const pmessage_type base_bus::get(const struct timespec& timeout) const
{
    if (m_opened)
    {
        return do_timed_get(timeout);
    }
    return NULL;
}

/**
 * Remove the next message from the bus
 * @return the result of the removing
 */
bool base_bus::pop()
{
    return m_opened && do_pop();
}

/**
 * Remove the next message from the bus
 * @param timeout the allowable timeout of the removing
 * @return the result of the removing
 */
bool base_bus::pop(const struct timespec& timeout)
{
    return m_opened && do_timed_pop(timeout);
}

/**
 * Push data to the bus
 * @param tag the tag of the data
 * @param data the data
 * @param size the size of the data
 * @return result of the pushing
 */
//virtual
bool base_bus::do_push(const tag_type tag, const void *data, const size_t size)
{
    while (!front_connector()->push(tag, data, size))
    {
        if (!add_connector())
        {
            return false;
        }
    }
    return true;
}

/**
 * Push data to the bus
 * @param tag the tag of the data
 * @param data the data
 * @param size the size of the data
 * @param timeout the allowable timeout of the pushing
 * @return result of the pushing
 */
//virtual
bool base_bus::do_timed_push(const tag_type tag, const void *data, const size_t size,
    const struct timespec& timeout)
{
    while (!front_connector()->push(tag, data, size, timeout))
    {
        if (!add_connector())
        {
            return false;
        }
    }
    return true;
}

/**
 * Get the next message from the bus
 * @return the message
 */
//virtual
const pmessage_type base_bus::do_get() const
{
    pmessage_type pmessage = back_connector()->get();
    while (!pmessage)
    {
        if (!remove_connector())
        {
            return NULL;
        }
        pmessage = back_connector()->get();
    }
    return pmessage;
}

/**
 * Get the next message from the bus
 * @param timeout the allowable timeout of the getting
 * @return the message
 */
//virtual
const pmessage_type base_bus::do_timed_get(const struct timespec& timeout) const
{
    pmessage_type pmessage = back_connector()->get(timeout);
    while (!pmessage)
    {
        if (!remove_connector())
        {
            return NULL;
        }
        pmessage = back_connector()->get(timeout);
    }
    return pmessage;
}

/**
 * Remove the next message from the bus
 * @return the result of the removing
 */
//virtual
bool base_bus::do_pop()
{
    while (!back_connector()->pop())
    {
        if (!remove_connector())
        {
            return false;
        }
    }
    return true;
}

/**
 * Remove the next message from the bus
 * @param timeout the allowable timeout of the removing
 * @return the result of the removing
 */
//virtual
bool base_bus::do_timed_pop(const struct timespec& timeout)
{
    while (!back_connector()->pop(timeout))
    {
        if (!remove_connector())
        {
            return false;
        }
    }
    return true;
}

/**
 * Create the connector
 * @param spec the specification of the bus
 * @return the result of the creating
 */
//virtual
bool base_bus::do_create(const specification_type& spec)
{
    pconnector_type pconnector = make_connector(0);
    if (pconnector)
    {
        m_pconnectors.push_front(pconnector);
        return true;
    }
    return false;
}

/**
 * Open the connector
 * @return the result of the opening
 */
//virtual
bool base_bus::do_open()
{
    const controlblock_type& cb = get_controlblock();
    id_type id = cb.back_connector_id;
    do
    {
        pconnector_type pconnector = make_connector(id);
        if (!pconnector)
        {
            m_pconnectors.clear();
            return false;
        }
        m_pconnectors.push_front(pconnector);
    } while (id++ != cb.front_connector_id);
    return true;
}

//==============================================================================
//  shared_bus
//==============================================================================
/**
 * Constructor
 * @param name the name of the connector
 */
shared_bus::shared_bus(const std::string& name) :
    base_bus(name)
{
}

/**
 * Create the shared memory
 * @return the result of the creating
 */
bool shared_bus::create_memory()
{
    m_pmemory = boost::make_shared<shared_memory_type>(name());
    return m_pmemory->create(memory_size());
}

/**
 * Open the shared memory
 * @return the result of the opening
 */
bool shared_bus::open_memory()
{
    m_pmemory = boost::make_shared<shared_memory_type>(name());
    return m_pmemory->open();
}

/**
 * Free the shared memory
 */
void shared_bus::free_memory()
{
    m_pmemory.reset();
}

/**
 * Get the size of the shared memory
 * @return the size of the shared memory
 */
//virtual
size_t shared_bus::memory_size() const
{
    return sizeof(bus_body);
}

/**
 * Get the pointer to the shared memory
 * @return the pointer to the shared memory
 */
//virtual
void *shared_bus::get_memory() const
{
    return m_pmemory->get();
}

/**
 * Create the connector
 * @param spec the specification of the bus
 * @return the result of the creating
 */
//virtual
bool shared_bus::do_create(const specification_type& spec)
{
    if (create_memory())
    {
        bus_body *pbody = reinterpret_cast<bus_body*>(get_memory());
        pbody->spec = spec;
        pbody->controlblock.front_connector_id = 0;
        pbody->controlblock.back_connector_id = 0;
        if (base_type::do_create(spec))
        {
            return true;
        }
        free_memory();
    }
    return false;
}

/**
 * Open the connector
 * @return the result of the opening
 */
//virtual
bool shared_bus::do_open()
{
    if (open_memory())
    {
        if (base_type::do_open())
        {
            return true;
        }
        free_memory();
    }
    return false;
}

/**
 * Get the specification of the bus
 * @return the specification of the bus
 */
//virtual
const specification_type& shared_bus::get_spec() const
{
    return reinterpret_cast<bus_body*>(get_memory())->spec;
}

/**
 * Get the control block of the bus
 * @return the control block of the bus
 */
//virtual
controlblock_type& shared_bus::get_controlblock() const
{
    return reinterpret_cast<bus_body*>(get_memory())->controlblock;
}

} //namespace bus

} //namespace qbus
