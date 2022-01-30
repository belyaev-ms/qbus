#ifndef BUS_H
#define BUS_H

#include "qbus/connector.h"
#include <string>
#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace qbus
{
    
namespace bus
{
    
typedef connector::id_type id_type;
typedef connector::pos_type pos_type;
typedef connector::tag_type tag_type;
typedef connector::direction_type direction_type;
typedef pos_type size_type;

struct specification_type
{
    id_type id; ///< the identifier of a bus
    size_type keep_alive_timeout; ///< the maximum idle time before forcibly removing a message
    size_type min_capacity; ///< the minimum value of a bus capacity
    size_type max_capacity; ///< the maximum value of a bus capacity
    size_type capacity_factor; ///< the new value of bus capacity will be = capacity * (capacity_factor + 100) / 100
};

struct controlblock_type
{
    uint32_t epoch; ///< the epoch of the control block
    id_type input_id; ///< the connector identifier for getting data
    id_type output_id; ///< the connector identifier for pushing data
};

/**
 * The base bus
 */
class base_bus
{
public:
    explicit base_bus(const std::string& name);
    virtual ~base_bus();
    const std::string& name() const; ///< get the name of the bus
    bool create(const specification_type& spec); ///< create the bus
    bool open(); ///< open the bus
    bool push(const tag_type tag, const void *data, const size_t size); ///< push data to the bus
    bool push(const tag_type tag, const void *data, const size_t size, const struct timespec& timeout); ///< push data to the bus
    const pmessage_type get() const; ///< get the next message from the bus
    const pmessage_type get(const struct timespec& timeout) const; ///< get the next message from the bus
    bool pop(); ///< remove the next message from the bus
    bool pop(const struct timespec& timeout); ///< remove the next message from the bus
    bool enabled() const; ///< check if the bus is enabled
    const specification_type& spec() const; ///< get the specification of the bus
protected:
    virtual bool do_create(const specification_type& spec); ///< create the bus
    virtual bool do_open(); ///< open the bus
    virtual bool do_push(const tag_type tag, const void *data, const size_t size); ///< push data to the bus
    virtual bool do_timed_push(const tag_type tag, const void *data, const size_t size, const struct timespec& timeout); ///< push data to the bus
    virtual const pmessage_type do_get() const; ///< get the next message from the bus
    virtual const pmessage_type do_timed_get(const struct timespec& timeout) const; ///< get the next message from the bus
    virtual bool do_pop(); ///< remove the next message from the bus
    virtual bool do_timed_pop(const struct timespec& timeout); ///< remove the next message from the bus
    virtual const specification_type& get_spec() const = 0; ///< get the specification of the bus
    virtual controlblock_type& get_controlblock() const = 0; ///< get the control block of the bus
    bool add_connector() const; ///< add new connector to the bus
    bool remove_connector() const; ///< remove the back connector from the bus
private:
    virtual pconnector_type make_connector(const std::string& name) const = 0; ///< make new connector
    pconnector_type make_connector(const id_type id) const; ///< make new connector
    pconnector_type output_connector() const; ///< get the output connector
    pconnector_type input_connector() const; ///< get the input connector
private:
    const std::string m_name;
    mutable std::list<pconnector_type> m_pconnectors;
    bool m_opened;
};

/**
 * The shared bus
 */
class shared_bus : public base_bus
{
    typedef base_bus base_type;
public:
    explicit shared_bus(const std::string& name);
protected:
    struct bus_body
    {
        specification_type spec;
        controlblock_type controlblock;
    };
    virtual bool do_create(const specification_type& spec); ///< create the bus
    virtual bool do_open(); ///< open the bus
    virtual void *get_memory() const; ///< get the pointer to the shared memory
    virtual size_t memory_size() const; ///< get the size of the shared memory
    virtual bool do_push(const tag_type tag, const void *data, const size_t size); ///< push data to the bus
    virtual bool do_timed_push(const tag_type tag, const void *data, const size_t size, const struct timespec& timeout); ///< push data to the bus
    virtual const pmessage_type do_get() const; ///< get the next message from the bus
    virtual const pmessage_type do_timed_get(const struct timespec& timeout) const; ///< get the next message from the bus
    virtual bool do_pop(); ///< remove the next message from the bus
    virtual bool do_timed_pop(const struct timespec& timeout); ///< remove the next message from the bus
    bool create_memory(); ///< create the shared memory
    bool open_memory(); ///< open the shared memory
    void free_memory(); ///< free the shared memory
    virtual const specification_type& get_spec() const; ///< get the specification of the bus
    virtual controlblock_type& get_controlblock() const; ///< get the control block of the bus
private:
    enum update_status
    {
        US_NONE     = 0x00,
        US_INPUT    = 0x01,
        US_OUTPUT   = 0x02,
        US_BOTH     = 0x03
    };
    bool is_updated() const; ///< check if the bus is updated
    update_status update_connectors() const; ///< update the connectors
    bool update_input_connector() const; ///< update the input connector
    bool update_output_connector() const; ///< update the output connector
private:
    pshared_memory_type m_pmemory;
    mutable controlblock_type m_controlblock; ///< the local copy of the control block
    mutable update_status m_status;
};

/**
 * The simple bus
 */
template <typename Connector>
class simple_bus : public shared_bus
{
public:
    typedef Connector connector_type;
    explicit simple_bus(const std::string& name);
protected:
    virtual pconnector_type make_connector(const std::string& name) const; ///< make new connector
};

/**
 * The output bus
 */
template <typename Bus>
class output_bus : public Bus
{
public:
    explicit output_bus(const std::string& name);
protected:
    virtual const pmessage_type do_get() const; ///< get the next message from the bus
    virtual const pmessage_type do_timed_get(const struct timespec& timeout) const; ///< get the next message from the bus
    virtual bool do_pop(); ///< remove the next message from the bus
    virtual bool do_timed_pop(const struct timespec& timeout); ///< remove the next message from the bus
};

/**
 * The input bus
 */
template <typename Bus>
class input_bus : public Bus
{
public:
    explicit input_bus(const std::string& name);
protected:
    virtual bool do_push(const tag_type tag, const void *data, const size_t size); ///< push data to the bus
    virtual bool do_timed_push(const tag_type tag, const void *data, const size_t size, const struct timespec& timeout); ///< push data to the bus
};

/**
 * The input/output bus
 */
template <typename Bus>
class bidirectional_bus : public Bus
{
public:
    explicit bidirectional_bus(const std::string& name);
};

/**
 * The base safe bus for inter process communications
 */
template <typename Bus, typename Locker>
class base_safe_bus : public Bus
{
protected:
    typedef Bus base_type;
public:
    explicit base_safe_bus(const std::string& name);
    virtual ~base_safe_bus();
protected:
    virtual bool do_create(const specification_type& spec); ///< create the bus
    virtual bool do_open(); ///< open the connector
    virtual void *get_memory() const; ///< get the pointer to the shared memory
    virtual size_t memory_size() const; ///< get the size of the shared memory
};

typedef boost::shared_ptr<base_bus> pbus_type;

/**
 * Make the bus
 * @param name the name of the bus
 * @return the bus
 */
template <typename Bus>
const pbus_type make(const std::string& name)
{
    return boost::make_shared<Bus>(name);
}

//==============================================================================
//  simple_bus
//==============================================================================
/**
 * Constructor
 * @param name the name of the bus
 */
template <typename Connector>
simple_bus<Connector>::simple_bus(const std::string& name) :
    shared_bus(name)
{
}

/**
 * Make new connector
 * @param name the name of new connector
 * @return new connector
 */
//virtual 
template <typename Connector>
pconnector_type simple_bus<Connector>::make_connector(const std::string& name) const
{
    return connector::make<connector_type>(name);
}

//==============================================================================
//  output_bus
//==============================================================================
/**
 * Constructor
 * @param name the name of the bus
 */
template <typename Bus>
output_bus<Bus>::output_bus(const std::string& name) :
    Bus(name)
{
}

/**
 * Get the next message from the bus
 * @return the message
 */
//virtual
template <typename Bus>
const pmessage_type output_bus<Bus>::do_get() const
{
    return pmessage_type();
}

/**
 * Get the next message from the bus
 * @param timeout the allowable timeout of the getting
 * @return the message
 */
//virtual
template <typename Bus>
const pmessage_type output_bus<Bus>::do_timed_get(const struct timespec& timeout) const
{
    return pmessage_type();
}

/**
 * Remove the next message from the bus
 * @return the result of the removing
 */
//virtual
template <typename Bus>
bool output_bus<Bus>::do_pop()
{
    return false;
}

/**
 * Remove the next message from the bus
 * @param timeout the allowable timeout of the removing
 * @return the result of the removing
 */
//virtual
template <typename Bus>
bool output_bus<Bus>::do_timed_pop(const struct timespec& timeout)
{
    return false;
}

//==============================================================================
//  input_bus
//==============================================================================
/**
 * Constructor
 * @param name the name of the bus
 */
template <typename Bus>
input_bus<Bus>::input_bus(const std::string& name) :
    Bus(name)
{
}

/**
 * Push data to the bus
 * @param tag the tag of the data
 * @param data the data
 * @param size the size of the data
 * @return result of the pushing
 */
//virtual
template <typename Bus>
bool input_bus<Bus>::do_push(const tag_type tag, const void *data, const size_t size)
{
    return false;
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
template <typename Bus>
bool input_bus<Bus>::do_timed_push(const tag_type tag, const void *data, 
    const size_t size, const struct timespec& timeout)
{
    return false;
}

//==============================================================================
//  bidirectional_bus
//==============================================================================
/**
 * Constructor
 * @param name the name of the bus
 */
template <typename Bus>
bidirectional_bus<Bus>::bidirectional_bus(const std::string& name) :
    Bus(name)
{
}

//==============================================================================
//  base_safe_bus
//==============================================================================
/**
 * Constructor
 * @param name the name of the bus
 */
template <typename Bus, typename Locker>
base_safe_bus<Bus, Locker>::base_safe_bus(const std::string& name) :
    base_type(name)
{
}

/**
 * Destructor
 */
// virtual
template <typename Bus, typename Locker>
base_safe_bus<Bus, Locker>::~base_safe_bus()
{
    if (base_type::enabled())
    {
    }
}

/**
 * Create the bus
 * @param spec the specification of the bus
 * @return the result of the creating
 */
//virtual
template <typename Bus, typename Locker>
bool base_safe_bus<Bus, Locker>::do_create(const specification_type& spec)
{
    return false;
}

/**
 * Open the bus
 * @return the result of the opening
 */
//virtual
template <typename Bus, typename Locker>
bool base_safe_bus<Bus, Locker>::do_open()
{
    return false;
}

/**
 * Get the pointer to the shared memory
 * @return the pointer to the shared memory
 */
//virtual
template <typename Bus, typename Locker>
void *base_safe_bus<Bus, Locker>::get_memory() const
{
    return reinterpret_cast<uint8_t*>(base_type::get_memory());
}

/**
 * Get the size of the shared memory
 * @return the size of the shared memory
 */
//virtual
template <typename Bus, typename Locker>
size_t base_safe_bus<Bus, Locker>::memory_size() const
{
    return base_type::memory_size();
}

} //namespace bus

typedef bus::simple_bus<single_input_connector_type> single_input_bus_type;
typedef bus::simple_bus<single_output_connector_type> single_output_bus_type;
typedef bus::simple_bus<single_bidirectional_connector_type> single_bidirectional_bus_type;
typedef bus::simple_bus<multi_input_connector_type> multi_input_bus_type;
typedef bus::simple_bus<multi_output_connector_type> multi_output_bus_type;
typedef bus::simple_bus<multi_bidirectional_connector_type> multi_bidirectional_bus_type;

typedef bus::pbus_type pbus_type;

} //namespace qbus

#endif /* BUS_H */

