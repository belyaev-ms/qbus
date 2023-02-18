#ifndef QBUS_CONNECTOR_H
#define QBUS_CONNECTOR_H

#include "qbus/queue.h"
#include "qbus/memory.h"
#include "qbus/locker.h"
#include "qbus/common.h"
#include <time.h>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#include <boost/interprocess/sync/interprocess_upgradable_mutex.hpp>

namespace qbus
{

namespace bus 
{
    class base_bus;
}

namespace connector
{
    
typedef queue::id_type id_type;
typedef queue::pos_type pos_type;
typedef message::tag_type tag_type;

/** types of connectors */
enum direction_type
{
    CON_IN      = 0,    ///< input
    CON_OUT     = 1,    ///< output
    CON_BIDIR   = 2     ///< bidirectional (input and output)
};

/**
 * The base connector
 */
class base_connector
{
    friend class qbus::bus::base_bus;
public:
    typedef boost::shared_ptr<base_connector> pconnector_type;
    base_connector(const std::string& name, const direction_type type);
    virtual ~base_connector();
    const std::string& name() const; ///< get the name of the connector
    direction_type type() const; ///< get the type of the connecter
    bool create(const id_type cid, const size_t size, 
        const struct timespec *pkeepalive_timeout = NULL); ///< create the connector
    bool open(); ///< open the connector
    bool push(const tag_type tag, const void *data, const size_t size); ///< push data to the connector
    bool push(const tag_type tag, const void *data, const size_t size, const struct timespec& timeout); ///< push data to the connector
    const pmessage_type get() const; ///< get the next message from the connector
    const pmessage_type get(const struct timespec& timeout) const; ///< get the next message from the connector
    bool pop(); ///< remove the next message from the connector
    bool pop(const struct timespec& timeout); ///< remove the next message from the connector
    bool enabled() const; ///< check if the connected is enabled
    size_t capacity() const; ///< get the capacity of the connector
protected:
    virtual bool do_create(const id_type cid, const size_t size,
        const struct timespec *pkeepalive_timeout, pconnector_type pconnector) = 0; ///< create the connector
    virtual bool do_open(pconnector_type pconnector) = 0; ///< open the connector
    virtual bool do_push(const tag_type tag, const void *data, const size_t size) = 0; ///< push data to the connector
    virtual bool do_timed_push(const tag_type tag, const void *data, const size_t size, const struct timespec& timeout); ///< push data to the connector
    virtual const pmessage_type do_get() const = 0; ///< get the next message from the connector
    virtual const pmessage_type do_timed_get(const struct timespec& timeout) const; ///< get the next message from the connector
    virtual bool do_pop() = 0; ///< remove the next message from the connector
    virtual bool do_timed_pop(const struct timespec& timeout); ///< remove the next message from the connector
    virtual size_t get_capacity() const = 0; ///< get the capacity of the connector
private:
    bool create(const id_type cid, const size_t size, 
        const struct timespec *pkeepalive_timeout, pconnector_type pconnector); ///< create the connector
    bool open(pconnector_type pconnector); ///< open the connector
private:
    const std::string m_name;
    const direction_type m_type;
    bool m_opened;
};

typedef base_connector connector_type;
typedef connector_type::pconnector_type pconnector_type;

/**
 * The shared connector
 */
class shared_connector : public base_connector
{
public:
    shared_connector(const std::string& name, const direction_type type);
protected:
    virtual bool do_create(const id_type cid, const size_t size, 
        const struct timespec *pkeepalive_timeout, pconnector_type pconnector); ///< create the connector
    virtual bool do_open(pconnector_type pconnector); ///< open the connector
    virtual void *get_memory() const; ///< get the pointer to the shared memory
    virtual size_t memory_size(const size_t size) const = 0; ///< get the size of the shared memory
    bool create_memory(const size_t size); ///< create the shared memory
    bool open_memory(); ///< open the shared memory
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
public:
    simple_connector(const std::string& name, const direction_type type);
protected:
    virtual bool do_create(const id_type cid, const size_t size,
        const struct timespec *pkeepalive_timeout, pconnector_type pconnector); ///< create the connector
    virtual bool do_open(pconnector_type pconnector); ///< open the connector
    virtual size_t memory_size(const size_t size) const; ///< get the size of the shared memory
    virtual bool do_push(const tag_type tag, const void *data, const size_t sz); ///< push data to the connector
    virtual const pmessage_type do_get() const; ///< get the next message from the connector
    virtual bool do_pop(); ///< remove the next message from the connector
    virtual size_t get_capacity() const; ///< get the capacity of the connector
    void create_queue(const id_type cid, const size_t size, 
        const struct timespec *pkeepalive_timeout, pconnector_type pconnector); ///< create the queue
    void open_queue(pconnector_type pconnector); ///< open the queue
    void free_queue(); ///< free the queue
private:
    pqueue_type m_pqueue;
};

typedef simple_connector<queue::simple_queue> single_bidirectional_connector_type;
typedef simple_connector<queue::shared_queue> multi_bidirectional_connector_type;
typedef simple_connector<queue::unreadable_shared_queue> multi_output_connector_type;

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
    virtual const pmessage_type do_timed_get(const struct timespec& timeout) const; ///< get the next message from the connector
    virtual bool do_pop(); ///< remove the next message from the connector
    virtual bool do_timed_pop(const struct timespec& timeout); ///< remove the next message from the connector
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
    virtual bool do_push(const tag_type tag, const void *data, const size_t size); ///< push data to the connector
    virtual bool do_timed_push(const tag_type tag, const void *data, const size_t size, const struct timespec& timeout); ///< push data to the connector
};

/**
 * The input/output connector
 */
template <typename Connector>
class bidirectional_connector : public Connector
{
public:
    explicit bidirectional_connector(const std::string& name);
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
 * The sharable locker interface based on boost::interprocess_upgradable_mutex
 */
class sharable_locker_interface : public base_locker_interface<false>
{
public:
    typedef boost::interprocess::interprocess_upgradable_mutex locker_type;
    typedef boost::interprocess::scoped_lock<locker_type> scoped_lock_type;
    typedef boost::interprocess::sharable_lock<locker_type> sharable_lock_type;
protected:
    typedef boost::interprocess::try_to_lock_type try_to_lock_type;
    struct scoped_try_lock : public scoped_lock_type
    {
        explicit scoped_try_lock(locker_type& locker) :
            scoped_lock_type(locker, try_to_lock_type())
        {}
    };
    struct sharable_try_lock : public sharable_lock_type
    {
        explicit sharable_try_lock(locker_type& locker) :
            sharable_lock_type(locker, try_to_lock_type())
        {}
    };
public:
    typedef scoped_try_lock lock_to_push_type;
    typedef scoped_try_lock lock_to_pop_type;
    typedef sharable_try_lock lock_to_get_type;
};

/**
 * The sharable locker interface to a connector that has a sharable pop operation
 */
class sharable_locker_with_sharable_pop_interface : public sharable_locker_interface
{
public:
    typedef scoped_try_lock lock_to_push_type;
    typedef sharable_try_lock lock_to_pop_type;
    typedef sharable_try_lock lock_to_get_type;
};

/**
 * The sharable spin locker interface
 */
class sharable_spinlocker_interface : public base_locker_interface<true>
{
public:
    typedef shared_locker locker_type;
    typedef scoped_lock<locker_type> scoped_lock_type;
    typedef sharable_lock<locker_type> sharable_lock_type;
    typedef scoped_try_lock<locker_type> lock_to_push_type;
    typedef scoped_try_lock<locker_type> lock_to_pop_type;
    typedef sharable_try_lock<locker_type> lock_to_get_type;
};

/**
 * The sharable spin locker interface to a connector that has a sharable pop operation
 */
class sharable_spinlocker_with_sharable_pop_interface : public base_locker_interface<true>
{
public:
    typedef shared_locker locker_type;
    typedef scoped_lock<locker_type> scoped_lock_type;
    typedef sharable_lock<locker_type> sharable_lock_type;
    typedef scoped_try_lock<locker_type> lock_to_push_type;
    typedef sharable_try_lock<locker_type> lock_to_pop_type;
    typedef sharable_try_lock<locker_type> lock_to_get_type;
};

/**
 * The sharable POSIX locker interface based on pthread_rwlock_t
 */
class sharable_posixlocker_interface : public base_locker_interface<true>
{
public:
    typedef shared_posix_locker locker_type;
    typedef scoped_lock<locker_type> scoped_lock_type;
    typedef sharable_lock<locker_type> sharable_lock_type;
    typedef scoped_try_lock<locker_type> lock_to_push_type;
    typedef scoped_try_lock<locker_type> lock_to_pop_type;
    typedef sharable_try_lock<locker_type> lock_to_get_type;
};

/**
 * The sharable POSIX locker interface to a connector that has a sharable pop operation
 */
class sharable_posixlocker_with_sharable_pop_interface : public base_locker_interface<true>
{
public:
    typedef shared_posix_locker locker_type;
    typedef scoped_lock<locker_type> scoped_lock_type;
    typedef sharable_lock<locker_type> sharable_lock_type;
    typedef scoped_try_lock<locker_type> lock_to_push_type;
    typedef sharable_try_lock<locker_type> lock_to_pop_type;
    typedef sharable_try_lock<locker_type> lock_to_get_type;
};

/**
 * The sharable barrier 
 */
class sharable_barrier
{
public:
    typedef shared_barrier barrier_type;
    sharable_barrier();
    barrier_type& barrier() const; ///< get the barrier
    void *create_barrier(void *ptr); ///< create the barrier
    void *open_barrier(void *ptr); ///< open the barrier
    void free_barrier(); ///< free the barrier
    static size_t barrier_size(); ///< get size of the barrier
protected:
    mutable barrier_type *m_pbarrier; ///< pointer to the barrier
};

/**
 * The stub barrier
 */
class stub_barrier
{
public:
    typedef void barrier_type;
    void barrier() const {}
    void *create_barrier(void *ptr) { return ptr; }
    void *open_barrier(void *ptr) { return ptr; }
    void free_barrier() {}
    static size_t barrier_size() { return 0; }
};

/**
 * The base safe connector for inter process communications
 */
template <typename Connector, typename Locker, typename Barrier>
class base_safe_connector : public Connector, protected Barrier
{
protected:
    typedef Connector base_type;
    typedef typename Locker::locker_type locker_type;
    typedef typename Locker::scoped_lock_type scoped_lock_type;
    typedef typename Locker::sharable_lock_type sharable_lock_type;
    typedef typename Locker::lock_to_push_type lock_to_push_type;
    typedef typename Locker::lock_to_get_type lock_to_get_type;
    typedef typename Locker::lock_to_pop_type lock_to_pop_type;
public:
    explicit base_safe_connector(const std::string& name);
    virtual ~base_safe_connector();
protected:
    virtual bool do_create(const id_type cid, const size_t size, 
        const struct timespec *pkeepalive_timeout, pconnector_type pconnector); ///< create the connector
    virtual bool do_open(pconnector_type pconnector); ///< open the connector
    virtual void *get_memory() const; ///< get the pointer to the shared memory
    virtual size_t memory_size(const size_t size) const; ///< get the size of the shared memory
    virtual bool do_push(const tag_type tag, const void *data, const size_t size); ///< push data to the connector
    virtual const pmessage_type do_get() const; ///< get the next message from the connector
    virtual bool do_pop(); ///< remove the next message from the connector
    locker_type& locker() const; ///< get the locker
private:
    mutable locker_type *m_plocker;
};

/**
 * The safe connector for inter process communications
 */
template <typename Connector, typename Locker = sharable_locker_interface,
    bool B = Locker::has_timed_lock>
class safe_connector : public base_safe_connector<Connector, Locker, stub_barrier>
{
    typedef base_safe_connector<Connector, Locker, stub_barrier> base_type;
    typedef typename base_type::locker_type locker_type;
    typedef typename base_type::scoped_lock_type scoped_lock_type;
    typedef typename base_type::sharable_lock_type sharable_lock_type;
    typedef typename base_type::lock_to_push_type lock_to_push_type;
    typedef typename base_type::lock_to_get_type lock_to_get_type;
    typedef typename base_type::lock_to_pop_type lock_to_pop_type;
public:
    explicit safe_connector(const std::string& name) : base_type(name) {}
};

/**
 * The safe connector for inter process communications
 */
template <typename Connector, typename Locker>
class safe_connector<Connector, Locker, true> : public base_safe_connector<Connector, Locker, sharable_barrier>
{
    typedef Connector connector_type;
    typedef base_safe_connector<Connector, Locker, sharable_barrier> base_type;
    typedef typename base_type::locker_type locker_type;
    typedef typename base_type::scoped_lock_type scoped_lock_type;
    typedef typename base_type::sharable_lock_type sharable_lock_type;
    typedef typename base_type::lock_to_push_type lock_to_push_type;
    typedef typename base_type::lock_to_get_type lock_to_get_type;
    typedef typename base_type::lock_to_pop_type lock_to_pop_type;
public:
    explicit safe_connector(const std::string& name);
    virtual bool do_push(const tag_type tag, const void *data, const size_t size); ///< push data to the connector
    virtual bool do_timed_push(const tag_type tag, const void *data, const size_t size, const struct timespec& timeout); ///< push data to the connector
    virtual const pmessage_type do_timed_get(const struct timespec& timeout) const; ///< get the next message from the connector
    virtual bool do_timed_pop(const struct timespec& timeout); ///< remove the next message from the connector
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
 * @param cid the identifier of the connector
 * @param size the size of a queue
 * @param pkeepalive_timeout the keep alive timeout of the connector
 * @param pconnector the parent connector
 * @return the result of the creating
 */
//virtual
template <typename Queue>
bool simple_connector<Queue>::do_create(const id_type cid, const size_t size,
    const struct timespec *pkeepalive_timeout, pconnector_type pconnector)
{
    if (base_type::do_create(cid, size, pkeepalive_timeout, pconnector))
    {
        create_queue(cid, size, pkeepalive_timeout, pconnector);
        return true;
    }
    return false;
}

/**
 * Open the connector
 * @param pconnector the parent connector
 * @return the result of the opening
 */
//virtual
template <typename Queue>
bool simple_connector<Queue>::do_open(pconnector_type pconnector)
{
    if (base_type::do_open(pconnector))
    {
        open_queue(pconnector);
        return true;
    }
    return false;
}

/**
 * Create the queue
 * @param cid the identifier of the queue
 * @param size the size of a queue
 * @param pkeepalive_timeout the keep alive timeout of the queue
 * @param pconnector the parent connector
 */
template <typename Queue>
void simple_connector<Queue>::create_queue(const id_type cid, const size_t size, 
    const struct timespec *pkeepalive_timeout, pconnector_type pconnector)
{
    m_pqueue = queue::create<queue_type>(cid, get_memory(), size, 
        pconnector ? 
            static_cast<simple_connector<Queue>*>(pconnector.get())->m_pqueue :
            pqueue_type());
    if (pkeepalive_timeout != NULL)
    {
        m_pqueue->keepalive_timeout(pkeepalive_timeout->tv_sec);
    }
}

/**
 * Open the queue
 * @param pconnector the parent connector
 */
template <typename Queue>
void simple_connector<Queue>::open_queue(pconnector_type pconnector)
{
    m_pqueue = queue::open<queue_type>(get_memory(), 
        pconnector ? 
            static_cast<simple_connector<Queue>*>(pconnector.get())->m_pqueue :
            pqueue_type());
}

/**
 * Free the queue
 */
template <typename Queue>
void simple_connector<Queue>::free_queue()
{
    m_pqueue.reset();
}

/**
 * Get the size of the shared memory
 * @param size the size of a queue
 * @return the size of the shared memory
 */
//virtual
template <typename Queue>
size_t simple_connector<Queue>::memory_size(const size_t size) const
{
    return queue_type::static_size(size);
}

/**
 * Push data to the connector
 * @param tag the tag of the data
 * @param data the data
 * @param size the size of the data
 * @return result of the pushing
 */
//virtual
template <typename Queue>
bool simple_connector<Queue>::do_push(const tag_type tag, const void *data, const size_t size)
{
    return m_pqueue->push(tag, data, size);
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
bool simple_connector<Queue>::do_pop()
{
    return m_pqueue->pop();
}

/**
 * Get the capacity of the connector
 * @return the capacity of the connector
 */
//virtual
template <typename Queue>
size_t simple_connector<Queue>::get_capacity() const
{
    return m_pqueue->capacity();
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
 * Get the next message from the connector
 * @param timeout the allowable timeout of the getting
 * @return the message
 */
//virtual
template <typename Connector>
const pmessage_type output_connector<Connector>::do_timed_get(const struct timespec& timeout) const
{
    QBUS_UNUSED(timeout);
    return pmessage_type();
}

/**
 * Remove the next message from the connector
 * @return the result of the removing
 */
//virtual
template <typename Connector>
bool output_connector<Connector>::do_pop()
{
    return false;
}

/**
 * Remove the next message from the connector
 * @param timeout the allowable timeout of the removing
 * @return the result of the removing
 */
//virtual
template <typename Connector>
bool output_connector<Connector>::do_timed_pop(const struct timespec& timeout)
{
    QBUS_UNUSED(timeout);
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
 * Push data to the connector
 * @param tag the tag of the data
 * @param data the data
 * @param size the size of the data
 * @return result of the pushing
 */
//virtual
template <typename Connector>
bool input_connector<Connector>::do_push(const tag_type tag, const void *data, const size_t size)
{
    QBUS_UNUSED(tag);
    QBUS_UNUSED(data);
    QBUS_UNUSED(size);
    return false;
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
template <typename Connector>
bool input_connector<Connector>::do_timed_push(const tag_type tag, const void *data, 
    const size_t size, const struct timespec& timeout)
{
    QBUS_UNUSED(tag);
    QBUS_UNUSED(data);
    QBUS_UNUSED(size);
    QBUS_UNUSED(timeout);
    return false;
}

//==============================================================================
//  bidirectional_connector
//==============================================================================
/**
 * Constructor
 * @param name the name of the connector
 */
template <typename Connector>
bidirectional_connector<Connector>::bidirectional_connector(const std::string& name) :
    Connector(name, CON_BIDIR)
{
}

//==============================================================================
//  base_safe_connector
//==============================================================================
/**
 * Constructor
 * @param name the name of the connector
 */
template <typename Connector, typename Locker, typename Barrier>
base_safe_connector<Connector, Locker, Barrier>::base_safe_connector(const std::string& name) :
    base_type(name),
    m_plocker(NULL)
{
}

/**
 * Destructor
 */
// virtual
template <typename Connector, typename Locker, typename Barrier>
base_safe_connector<Connector, Locker, Barrier>::~base_safe_connector()
{
    if (base_type::enabled())
    {
        uint8_t *ptr = reinterpret_cast<uint8_t*>(base_type::get_memory());
        spinlock *pspinlock = reinterpret_cast<spinlock*>(ptr);
        ptr += sizeof(spinlock);
        scoped_lock<spinlock> guard(*pspinlock);
        volatile uint32_t *pcounter = reinterpret_cast<volatile uint32_t*>(ptr);
        {
            scoped_lock_type lock(*m_plocker);
            base_type::free_queue();
        }
        if (--(*pcounter) == 0)
        {
            Barrier::free_barrier();
            m_plocker->~locker_type();
        }
    }
}

/**
 * Create the connector
 * @param cid the identifier of the connector
 * @param size the size of a queue
 * @param pkeepalive_timeout the keep alive timeout of the connector
 * @param pconnector the parent connector
 * @return the result of the creating
 */
//virtual
template <typename Connector, typename Locker, typename Barrier>
bool base_safe_connector<Connector, Locker, Barrier>::do_create(const id_type cid,
    const size_t size, const struct timespec *pkeepalive_timeout,
    pconnector_type pconnector)
{
    if (base_type::create_memory(size))
    {
        /* shared memory object are automatically initialized and the spinlock
         * is locked by default */
        uint8_t *ptr = reinterpret_cast<uint8_t*>(base_type::get_memory());
        spinlock *pspinlock = reinterpret_cast<spinlock*>(ptr);
        ptr += sizeof(spinlock);
        volatile uint32_t *pcounter = reinterpret_cast<volatile uint32_t*>(ptr);
        ptr += sizeof(uint32_t);
        m_plocker = new (ptr) locker_type();
        ptr += sizeof(locker_type);
        ptr = reinterpret_cast<uint8_t*>(Barrier::create_barrier(ptr));
        scoped_lock_type lock(*m_plocker);
        base_type::create_queue(cid, size, pkeepalive_timeout, pconnector);
        ++(*pcounter);
        pspinlock->unlock();
        return true;
    }
    return false;
}

/**
 * Open the connector
 * @param pconnector the parent connector
 * @return the result of the opening
 */
//virtual
template <typename Connector, typename Locker, typename Barrier>
bool base_safe_connector<Connector, Locker, Barrier>::do_open(pconnector_type pconnector)
{
    if (base_type::open_memory())
    {
        uint8_t *ptr = reinterpret_cast<uint8_t*>(base_type::get_memory());
        spinlock *pspinlock = reinterpret_cast<spinlock*>(ptr);
        ptr += sizeof(spinlock);
        scoped_lock<spinlock> guard(*pspinlock);
        volatile uint32_t *pcounter = reinterpret_cast<volatile uint32_t*>(ptr);
        if (*pcounter > 0)
        {
            ptr += sizeof(uint32_t);
            m_plocker = reinterpret_cast<locker_type*>(ptr);
            ptr += sizeof(locker_type);
            ptr = reinterpret_cast<uint8_t*>(Barrier::open_barrier(ptr));
            scoped_lock_type lock(*m_plocker);
            base_type::open_queue(pconnector);
            ++(*pcounter);
            return true;
        }
    }
    return false;
}

/**
 * Get the pointer to the shared memory
 * @return the pointer to the shared memory
 */
//virtual
template <typename Connector, typename Locker, typename Barrier>
void *base_safe_connector<Connector, Locker, Barrier>::get_memory() const
{
    return reinterpret_cast<uint8_t*>(base_type::get_memory()) +
        sizeof(locker_type) + Barrier::barrier_size() + sizeof(spinlock) + 
        sizeof(uint32_t);
}

/**
 * Get the size of the shared memory
 * @param size the size of a queue
 * @return the size of the shared memory
 */
//virtual
template <typename Connector, typename Locker, typename Barrier>
size_t base_safe_connector<Connector, Locker, Barrier>::memory_size(const size_t size) const
{
    return base_type::memory_size(size) + sizeof(locker_type) +
        Barrier::barrier_size() + sizeof(spinlock) + sizeof(uint32_t);
}

/**
 * Push data to the connector
 * @param tag the tag of the data
 * @param data the data
 * @param size the size of the data
 * @return result of the pushing
 */
//virtual
template <typename Connector, typename Locker, typename Barrier>
bool base_safe_connector<Connector, Locker, Barrier>::do_push(const tag_type tag, 
    const void *data, const size_t size)
{
    lock_to_push_type lock(*m_plocker);
    if (lock.owns() && base_type::do_push(tag, data, size))
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
template <typename Connector, typename Locker, typename Barrier>
const pmessage_type base_safe_connector<Connector, Locker, Barrier>::do_get() const
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
template <typename Connector, typename Locker, typename Barrier>
bool base_safe_connector<Connector, Locker, Barrier>::do_pop()
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
template <typename Connector, typename Locker, typename Barrier>
typename base_safe_connector<Connector, Locker, Barrier>::locker_type&
    base_safe_connector<Connector, Locker, Barrier>::locker() const
{
    return *m_plocker;
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
 * Push data to the connector
 * @param tag the tag of the data
 * @param data the data
 * @param size the size of the data
 * @return result of the pushing
 */
//virtual
template <typename Connector, typename Locker>
bool safe_connector<Connector, Locker, true>::do_push(const tag_type tag, 
    const void *data, const size_t size)
{
    lock_to_push_type lock(base_type::locker());
    if (lock.owns() && base_type::do_push(tag, data, size))
    {
        base_type::barrier().open();
        return true;
    }
    return false;
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
template <typename Connector, typename Locker>
bool safe_connector<Connector, Locker, true>::do_timed_push(const tag_type tag, 
    const void *data, const size_t size, const struct timespec& timeout)
{
    const struct timespec ts = get_monotonic_time() + timeout;
    while (get_monotonic_time() < ts)
    {
        lock_to_push_type lock(base_type::locker(), ts - get_monotonic_time());
        if (lock.owns() && connector_type::do_push(tag, data, size))
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
bool safe_connector<Connector, Locker, true>::
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
    connector::input_connector<
        connector::single_bidirectional_connector_type> > single_input_connector_type;
typedef connector::safe_connector<
    connector::output_connector<
        connector::single_bidirectional_connector_type> > single_output_connector_type;
typedef connector::safe_connector<
    connector::bidirectional_connector<
        connector::single_bidirectional_connector_type> > single_bidirectional_connector_type;
typedef connector::safe_connector<
    connector::input_connector<connector::multi_bidirectional_connector_type>,
    connector::sharable_spinlocker_with_sharable_pop_interface> multi_input_connector_type;
typedef connector::safe_connector<
    connector::output_connector<connector::multi_output_connector_type>,
    connector::sharable_spinlocker_with_sharable_pop_interface> multi_output_connector_type;
typedef connector::safe_connector<
    connector::bidirectional_connector<connector::multi_bidirectional_connector_type>,
    connector::sharable_spinlocker_with_sharable_pop_interface> multi_bidirectional_connector_type;

typedef connector::pconnector_type pconnector_type;

} //namespace qbus

#endif /* QBUS_CONNECTOR_H */

