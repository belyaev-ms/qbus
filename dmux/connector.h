#ifndef DMUX_CONNECTOR_H
#define DMUX_CONNECTOR_H

#include "dmux/queue.h"
#include "dmux/memory.h"
#include "dmux/locker.h"
#include "dmux/common.h"
#include <time.h>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/interprocess_upgradable_mutex.hpp>

namespace dmux
{

namespace connector
{

/** types of connectors */
enum direction_type
{
    CON_IN  = 0,    ///< input
    CON_OUT = 1,    ///< output
    CON_IO  = 2     ///< input and output
};

/**
 * The base connector
 */
class base_connector
{
public:
    base_connector(const std::string& name, const direction_type type);
    virtual ~base_connector();
    const std::string& name() const; ///< get the name of the connector
    const direction_type type() const; ///< get the type of the connecter
    const bool create(const size_t size, const size_t count); ///< create the connector
    const bool open(); ///< open the connector
    const bool add(const void *data, const size_t size); ///< add data to the connector
    const bool add(const void *data, const size_t size, const struct timespec& timeout); ///< add data to the connector
    const pmessage_type get() const; ///< get the next message from the connector
    const pmessage_type get(const struct timespec& timeout) const; ///< get the next message from the connector
    const bool pop(); ///< remove the next message from the connector
    const bool pop(const struct timespec& timeout); ///< remove the next message from the connector
    const bool enabled() const; ///< check if the connected is enabled
protected:
    virtual const bool do_create(const size_t size, const size_t count) = 0; ///< create the connector
    virtual const bool do_open() = 0; ///< open the connector
    virtual const bool do_add(const void *data, const size_t size) = 0; ///< add data to the connector
    virtual const bool do_timed_add(const void *data, const size_t size, const struct timespec& timeout); ///< add data to the connector
    virtual const pmessage_type do_get() const = 0; ///< get the next message from the connector
    virtual const pmessage_type do_timed_get(const struct timespec& timeout) const; ///< get the next message from the connector
    virtual const bool do_pop() = 0; ///< remove the next message from the connector
    virtual const bool do_timed_pop(const struct timespec& timeout); ///< remove the next message from the connector
private:
    const std::string m_name;
    const direction_type m_type;
    bool m_opened;
};

typedef base_connector connector_type;
typedef boost::shared_ptr<connector_type> pconnector_type;

/**
 * The shared connector
 */
class shared_connector : public base_connector
{
public:
    shared_connector(const std::string& name, const direction_type type);
protected:
    virtual const bool do_create(const size_t size, const size_t count); ///< create the connector
    virtual const bool do_open(); ///< open the connector
    virtual void *get_memory(); ///< get the pointer to the shared memory
    virtual const size_t memory_size(const size_t size, const size_t count) const = 0; ///< get the size of the shared memory
private:
    shared_memory_type m_memory;
};

/**
 * The simple connector
 */
template <typename Queue>
class simple_connector : public shared_connector
{
    typedef shared_connector base_type;
    typedef Queue queue_type;
    typedef boost::shared_ptr<queue_type> pqueue_type;
public:
    simple_connector(const std::string& name, const direction_type type);
protected:
    virtual const bool do_create(const size_t size, const size_t count); ///< create the connector
    virtual const bool do_open(); ///< open the connector
    virtual const size_t memory_size(const size_t sz, const size_t cnt) const; ///< get the size of the shared memory
    virtual const bool do_add(const void *data, const size_t sz); ///< add data to the connector
    virtual const pmessage_type do_get() const; ///< get the next message from the connector
    virtual const bool do_pop(); ///< remove the next message from the connector
private:
    pqueue_type m_pqueue;
};

typedef simple_connector<queue::simple_queue> single_connector_type;
typedef simple_connector<queue::shared_queue> multi_connector_type;

/**
 * The output connector
 */
template <typename Connector>
class output_connector : public Connector
{
public:
    explicit output_connector(const std::string& name);
protected:
    virtual const pmessage_type do_get() const; ///< get the next message from the connector
    virtual const bool do_pop(); ///< remove the next message from the connector
};

/**
 * The input connector
 */
template <typename Connector>
class input_connector : public Connector
{
public:
    explicit input_connector(const std::string& name);
protected:
    virtual const bool do_add(const void *data, const size_t size); ///< add data to the connector
};

/**
 * The input/output connector
 */
template <typename Connector>
class io_connector : public Connector
{
public:
    explicit io_connector(const std::string& name);
};

/**
 * The base locker interface
 */
template <bool B>
struct base_locker_interface
{
    static const bool has_timed_lock = B;
};

/**
 * The sharable locker based on boost::interprocess_upgradable_mutex
 */
class sharable_locker_interface : public base_locker_interface<false>
{
public:
    typedef boost::interprocess::interprocess_upgradable_mutex locker_type;
protected:
    typedef boost::interprocess::try_to_lock_type try_to_lock_type;
    struct scoped_try_lock : public boost::interprocess::scoped_lock<locker_type>
    {
        explicit scoped_try_lock(locker_type& locker) :
            boost::interprocess::scoped_lock<locker_type>(locker, try_to_lock_type())
        {}
    };
    struct sharable_try_lock : public boost::interprocess::sharable_lock<locker_type>
    {
        explicit sharable_try_lock(locker_type& locker) :
            boost::interprocess::sharable_lock<locker_type>(locker, try_to_lock_type())
        {}
    };
public:
    typedef scoped_try_lock lock_to_add_type;
    typedef scoped_try_lock lock_to_pop_type;
    typedef sharable_try_lock lock_to_get_type;
};

/**
 * The sharable locker to a connector that has a sharable pop operation
 */
class sharable_locker_with_sharable_pop_interface : public sharable_locker_interface
{
public:
    typedef scoped_try_lock lock_to_add_type;
    typedef sharable_try_lock lock_to_pop_type;
    typedef sharable_try_lock lock_to_get_type;
};

/**
 * The sharable spin locker
 */
class sharable_spinlocker_interface : public base_locker_interface<true>
{
public:
    typedef shared_locker locker_type;
public:
    typedef scoped_try_lock<locker_type> lock_to_add_type;
    typedef scoped_try_lock<locker_type> lock_to_pop_type;
    typedef sharable_try_lock<locker_type> lock_to_get_type;
};

/**
 * The sharable spin locker to a connector that has a sharable pop operation
 */
class sharable_spinlocker_with_sharable_pop_interface : public base_locker_interface<true>
{
public:
    typedef shared_locker locker_type;
public:
    typedef scoped_try_lock<locker_type> lock_to_add_type;
    typedef sharable_try_lock<locker_type> lock_to_pop_type;
    typedef sharable_try_lock<locker_type> lock_to_get_type;
};

/**
 * The base safe connector for inter process communications
 */
template <typename Connector, typename Locker>
class base_safe_connector : public Connector
{
protected:
    typedef Connector base_type;
    typedef shared_barrier barrier_type;
    typedef typename Locker::locker_type locker_type;
    typedef typename Locker::lock_to_add_type lock_to_add_type;
    typedef typename Locker::lock_to_get_type lock_to_get_type;
    typedef typename Locker::lock_to_pop_type lock_to_pop_type;
public:
    explicit base_safe_connector(const std::string& name);
    virtual ~base_safe_connector();
protected:
    virtual const bool do_create(const size_t size, const size_t count); ///< create the connector
    virtual const bool do_open(); ///< open the connector
    virtual void *get_memory(); ///< get the pointer to the shared memory
    virtual const size_t memory_size(const size_t size, const size_t count) const; ///< get the size of the shared memory
    virtual const bool do_add(const void *data, const size_t size); ///< add data to the connector
    virtual const pmessage_type do_get() const; ///< get the next message from the connector
    virtual const bool do_pop(); ///< remove the next message from the connector
    locker_type& locker() const; ///< get the locker
    barrier_type& barrier() const; ///< get the barrier
private:
    mutable locker_type *m_plocker;
    mutable barrier_type *m_pbarrier;
    bool m_owner;
};

/**
 * The safe connector for inter process communications
 */
template <typename Connector, typename Locker = sharable_locker_interface,
    bool B = Locker::has_timed_lock>
class safe_connector : public base_safe_connector<Connector, Locker>
{
    typedef base_safe_connector<Connector, Locker> base_type;
    typedef typename base_type::locker_type locker_type;
    typedef typename base_type::lock_to_add_type lock_to_add_type;
    typedef typename base_type::lock_to_get_type lock_to_get_type;
    typedef typename base_type::lock_to_pop_type lock_to_pop_type;
public:
    explicit safe_connector(const std::string& name) : base_type(name) {}
};

/**
 * The safe connector for inter process communications
 */
template <typename Connector, typename Locker>
class safe_connector<Connector, Locker, true> : public base_safe_connector<Connector, Locker>
{
    typedef Connector connector_type;
    typedef base_safe_connector<Connector, Locker> base_type;
    typedef typename base_type::locker_type locker_type;
    typedef typename base_type::lock_to_add_type lock_to_add_type;
    typedef typename base_type::lock_to_get_type lock_to_get_type;
    typedef typename base_type::lock_to_pop_type lock_to_pop_type;
public:
    explicit safe_connector(const std::string& name);
    virtual const bool do_timed_add(const void *data, const size_t size, const struct timespec& timeout); ///< add data to the connector
    virtual const pmessage_type do_timed_get(const struct timespec& timeout) const; ///< get the next message from the connector
    virtual const bool do_timed_pop(const struct timespec& timeout); ///< remove the next message from the connector
};

/**
 * Make the connector
 * @param name the name of the connector
 * @return the connector
 */
template <typename Connector>
const pconnector_type make(const std::string& name)
{
    return boost::make_shared<Connector>(name);
}

//==============================================================================
//  simple_connector
//==============================================================================
/**
 * Constructor
 * @param name the name of the connector
 * @param type the type of the connector
 */
template <typename Queue>
simple_connector<Queue>::simple_connector(const std::string& name, const direction_type type) :
    base_type(name, type)
{
}

/**
 * Create the connector
 * @param sz the size of a data blocks
 * @param cnt the count of data blocks
 * @return the result of the creating
 */
//virtual
template <typename Queue>
const bool simple_connector<Queue>::do_create(const size_t sz, const size_t cnt)
{
    if (base_type::do_create(sz, cnt))
    {
        m_pqueue = boost::make_shared<queue_type>(get_memory(), sz, cnt);
        return true;
    }
    return false;
}

/**
 * Open the connector
 * @return the result of the opening
 */
//virtual
template <typename Queue>
const bool simple_connector<Queue>::do_open()
{
    if (base_type::do_open())
    {
        m_pqueue = boost::make_shared<queue_type>(get_memory());
        return true;
    }
    return false;
}

/**
 * Get the size of the shared memory
 * @param sz the size of a data blocks
 * @param cnt the count of data blocks
 * @return the size of the shared memory
 */
//virtual
template <typename Queue>
const size_t simple_connector<Queue>::memory_size(const size_t sz, const size_t cnt) const
{
    return queue_type::static_size(sz, cnt);
}

/**
 * Add data to the connector
 * @param data the data
 * @param sz the size of the data
 * @return result of the adding
 */
//virtual
template <typename Queue>
const bool simple_connector<Queue>::do_add(const void *data, const size_t sz)
{
    return m_pqueue->add(data, sz).get();
}

/**
 * Get the next message from the connector
 * @return the message
 */
//virtual
template <typename Queue>
const pmessage_type simple_connector<Queue>::do_get() const
{
    return m_pqueue->get();
}

/**
 * Remove the next message from the connector
 * @return the result of the removing
 */
//virtual
template <typename Queue>
const bool simple_connector<Queue>::do_pop()
{
    m_pqueue->pop();
    return true;
}

//==============================================================================
//  output_connector
//==============================================================================
/**
 * Constructor
 * @param name the name of the connector
 */
template <typename Connector>
output_connector<Connector>::output_connector(const std::string& name) :
    Connector(name, CON_OUT)
{
}

/**
 * Get the next message from the connector
 * @return the message
 */
//virtual
template <typename Connector>
const pmessage_type output_connector<Connector>::do_get() const
{
    return pmessage_type();
}

/**
 * Remove the next message from the connector
 * @return the result of the removing
 */
//virtual
template <typename Connector>
const bool output_connector<Connector>::do_pop()
{
    return false;
}

//==============================================================================
//  input_connector
//==============================================================================
/**
 * Constructor
 * @param name the name of the connector
 */
template <typename Connector>
input_connector<Connector>::input_connector(const std::string& name) :
    Connector(name, CON_IN)
{
}

/**
 * Add data to the connector
 * @param data the data
 * @param size the size of the data
 * @return result of the adding
 */
//virtual
template <typename Connector>
const bool input_connector<Connector>::do_add(const void *data, const size_t size)
{
    return false;
}

//==============================================================================
//  io_connector
//==============================================================================
/**
 * Constructor
 * @param name the name of the connector
 */
template <typename Connector>
io_connector<Connector>::io_connector(const std::string& name) :
    Connector(name, CON_IO)
{
}

//==============================================================================
//  base_safe_connector
//==============================================================================
/**
 * Constructor
 * @param name the name of the connector
 */
template <typename Connector, typename Locker>
base_safe_connector<Connector, Locker>::base_safe_connector(const std::string& name) :
    base_type(name),
    m_plocker(NULL),
    m_pbarrier(NULL),
    m_owner(false)
{
}

/**
 * Destructor
 */
// virtual
template <typename Connector, typename Locker>
base_safe_connector<Connector, Locker>::~base_safe_connector()
{
    if (m_owner)
    {
        m_plocker->~locker_type();
    }
}

/**
 * Create the connector
 * @param size the size of a data blocks
 * @param count the count of data blocks
 * @return the result of the creating
 */
//virtual
template <typename Connector, typename Locker>
const bool base_safe_connector<Connector, Locker>::do_create(const size_t size,
    const size_t count)
{
    if (base_type::do_create(size, count))
    {
        uint8_t *ptr = reinterpret_cast<uint8_t*>(base_type::get_memory());
        m_plocker = new (ptr) locker_type();
        ptr += sizeof(locker_type);
        m_pbarrier = new (ptr) barrier_type();
        m_owner = true;
        return true;
    }
    return false;
}

/**
 * Open the connector
 * @return the result of the opening
 */
//virtual
template <typename Connector, typename Locker>
const bool base_safe_connector<Connector, Locker>::do_open()
{
    if (base_type::do_open())
    {
        uint8_t *ptr = reinterpret_cast<uint8_t*>(base_type::get_memory());
        m_plocker = reinterpret_cast<locker_type*>(ptr);
        ptr += sizeof(locker_type);
        m_pbarrier = reinterpret_cast<barrier_type*>(ptr);
        return true;
    }
    return false;
}

/**
 * Get the pointer to the shared memory
 * @return the pointer to the shared memory
 */
//virtual
template <typename Connector, typename Locker>
void *base_safe_connector<Connector, Locker>::get_memory()
{
    return reinterpret_cast<uint8_t*>(base_type::get_memory()) +
        sizeof(locker_type) + sizeof(barrier_type);
}

/**
 * Get the size of the shared memory
 * @param size the size of a data blocks
 * @param count the count of data blocks
 * @return the size of the shared memory
 */
//virtual
template <typename Connector, typename Locker>
const size_t base_safe_connector<Connector, Locker>::memory_size(const size_t size,
    const size_t count) const
{
    return base_type::memory_size(size, count) +  sizeof(locker_type) +
        sizeof(barrier_type);
}

/**
 * Add data to the connector
 * @param data the data
 * @param size the size of the data
 * @return result of the adding
 */
//virtual
template <typename Connector, typename Locker>
const bool base_safe_connector<Connector, Locker>::do_add(const void *data,
    const size_t size)
{
    lock_to_add_type lock(*m_plocker);
    if (lock.owns() && base_type::do_add(data, size))
    {
        return true;
    }
    return false;
}

/**
 * Get the next message from the connector
 * @return the message
 */
//virtual
template <typename Connector, typename Locker>
const pmessage_type base_safe_connector<Connector, Locker>::do_get() const
{
    lock_to_get_type lock(*m_plocker);
    if (lock.owns())
    {
        return base_type::do_get();
    }
    return pmessage_type();
}

/**
 * Remove the next message from the connector
 * @return the result of the removing
 */
//virtual
template <typename Connector, typename Locker>
const bool base_safe_connector<Connector, Locker>::do_pop()
{
    lock_to_pop_type lock(*m_plocker);
    if (lock.owns() && base_type::do_pop())
    {
        return true;
    }
    return false;
}

/**
 * Get the locker
 * @return the locker
 */
template <typename Connector, typename Locker>
typename base_safe_connector<Connector, Locker>::locker_type&
    base_safe_connector<Connector, Locker>::locker() const
{
    return *m_plocker;
}

/**
 * Get the barrier
 * @return the barrier
 */
template <typename Connector, typename Locker>
typename base_safe_connector<Connector, Locker>::barrier_type&
    base_safe_connector<Connector, Locker>::barrier() const
{
    return *m_pbarrier;
}

//==============================================================================
//  safe_connector
//==============================================================================
/**
 * Constructor
 * @param name the name of the connector
 */
template <typename Connector, typename Locker>
safe_connector<Connector, Locker, true>::safe_connector(const std::string& name) :
    base_type(name)
{
}

/**
 * Add data to the connector
 * @param data the data
 * @param size the size of the data
 * @param timeout the allowable timeout of the adding
 * @return result of the adding
 */
//virtual
template <typename Connector, typename Locker>
const bool safe_connector<Connector, Locker, true>::do_timed_add(const void *data,
    const size_t size, const struct timespec& timeout)
{
    const struct timespec ts = get_monotonic_time() + timeout;
    while (get_monotonic_time() < ts)
    {
        lock_to_add_type lock(base_type::locker(), ts - get_monotonic_time());
        if (lock.owns() && connector_type::do_add(data, size))
        {
            base_type::barrier().open();
            return true;
        }
    }
    return false;
}

/**
 * Get the next message from the connector
 * @param timeout the allowable timeout of the getting
 * @return the message
 */
//virtual
template <typename Connector, typename Locker>
const pmessage_type safe_connector<Connector, Locker, true>::
    do_timed_get(const struct timespec& timeout) const
{
    const struct timespec ts = get_monotonic_time() + timeout;
    pmessage_type pmessage;
    while (get_monotonic_time() < ts)
    {
        lock_to_get_type lock(base_type::locker(), ts - get_monotonic_time());
        if (lock.owns())
        {
            pmessage = connector_type::do_get();
            if (pmessage)
            {
                return pmessage;
            }
            base_type::barrier().knock();
            lock.unlock();
            if (!base_type::barrier().expect(ts - get_monotonic_time()))
            {
                break;
            }
        }
    }
    return pmessage_type();
}

/**
 * Remove the next message from the connector
 * @param timeout the allowable timeout of the removing
 * @return the result of the removing
 */
//virtual
template <typename Connector, typename Locker>
const bool safe_connector<Connector, Locker, true>::
    do_timed_pop(const struct timespec& timeout)
{
    const struct timespec ts = get_monotonic_time() + timeout;
    while (get_monotonic_time() < ts)
    {
        lock_to_pop_type lock(base_type::locker(), ts - get_monotonic_time());
        if (lock.owns() && connector_type::do_pop())
        {
            return true;
        }
    }
    return false;
}

} //namespace connector

typedef connector::safe_connector<
    connector::input_connector<connector::single_connector_type> >  in_connector_type;
typedef connector::safe_connector<
    connector::output_connector<connector::single_connector_type> > out_connector_type;
typedef connector::safe_connector<
    connector::io_connector<connector::single_connector_type> >     io_connector_type;
typedef connector::safe_connector<
    connector::input_connector<connector::multi_connector_type>,
    connector::sharable_spinlocker_with_sharable_pop_interface>     in_multiout_connector_type;
typedef connector::safe_connector<
    connector::output_connector<connector::multi_connector_type>,
    connector::sharable_spinlocker_with_sharable_pop_interface>     out_multiout_connector_type;

typedef connector::pconnector_type pconnector_type;

} //namespace dmux

#endif /* DMUX_CONNECTOR_H */

