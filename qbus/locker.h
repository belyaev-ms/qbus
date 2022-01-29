#ifndef QBUS_LOCKER_H
#define QBUS_LOCKER_H

#include "qbus/exceptions.h"
#include <time.h>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

namespace qbus
{

/**
 * Simple class for RW locker which is placed in a shared memory
 */
class shared_locker
{
public:
    shared_locker();
    void lock();
    bool timed_lock(const struct timespec& timeout);
    bool try_lock();
    void unlock();
    void lock_sharable();
    bool timed_lock_sharable(const struct timespec& timeout);
    bool try_lock_sharable();
    void unlock_sharable();
protected:
    enum lock_state
    {
        LS_SCOPED,
        LS_SHARABLE,
        LS_NONE
    };
    lock_state try_book_lock();
    unsigned int count_sharable();
private:
    shared_locker(const shared_locker& );
    shared_locker& operator=(const shared_locker& );
private:
    typedef boost::detail::spinlock lock_type;
    typedef boost::detail::spinlock::scoped_lock guard_type;
private:
    lock_type m_lock;
    volatile unsigned int m_scoped;
    volatile unsigned int m_sharable;
};

/** Type to indicate to a locker constructor that must not lock it */
struct defer_lock_type{};
/** Type to indicate to a locker constructor that must try to lock it */
struct try_to_lock_type{};

/**
 * Class scoped_lock is meant to carry out the tasks for locking, unlocking,
 * try-locking and timed-locking for the Locker
 */
template <typename Locker>
class scoped_lock
{
public:
    typedef Locker locker_type;
    explicit scoped_lock(locker_type& locker);
    scoped_lock(locker_type& locker, defer_lock_type);
    scoped_lock(locker_type& locker, try_to_lock_type);
    scoped_lock(locker_type& locker, const struct timespec& timeout);
    ~scoped_lock();
    void lock();
    bool try_lock();
    bool timed_lock(const struct timespec& timeout);
    void unlock();
    bool owns() const;
private:
    scoped_lock();
    scoped_lock(const scoped_lock& );
    scoped_lock& operator=(const scoped_lock& );
private:
    locker_type& m_locker;
    bool m_locked;
};

template <typename Locker>
class scoped_try_lock : public scoped_lock<Locker>
{
public:
    explicit scoped_try_lock(Locker& locker) :
        scoped_lock<Locker>(locker, try_to_lock_type())
    {}
    scoped_try_lock(Locker& locker, const struct timespec& timeout) :
        scoped_lock<Locker>(locker, timeout)
    {}
};

/**
 * Class scoped_lock is meant to carry out the tasks for sharable-locking,
 * sharable-unlocking, try-sharable-locking and timed-sharable-locking for
 * the Locker
 */
template <typename Locker>
class sharable_lock
{
public:
    typedef Locker locker_type;
    explicit sharable_lock(locker_type& locker);
    sharable_lock(locker_type& locker, defer_lock_type);
    sharable_lock(locker_type& locker, try_to_lock_type);
    sharable_lock(locker_type& locker, const struct timespec& timeout);
    ~sharable_lock();
    void lock();
    bool try_lock();
    bool timed_lock(const struct timespec& timeout);
    void unlock();
    bool owns() const;
private:
    sharable_lock();
    sharable_lock(const sharable_lock& );
    sharable_lock& operator=(const sharable_lock& );
private:
    locker_type& m_locker;
    bool m_locked;
};

template <typename Locker>
class sharable_try_lock : public sharable_lock<Locker>
{
public:
    explicit sharable_try_lock(Locker& locker) :
        sharable_lock<Locker>(locker, try_to_lock_type())
    {}
    sharable_try_lock(Locker& locker, const struct timespec& timeout) :
        sharable_lock<Locker>(locker, timeout)
    {}
};

/**
 * The barrier hold all the waiting threads until some thread opens it
 */
class shared_barrier
{
public:
    shared_barrier();
    void open(); ///< open the barrier
    void knock() const; ///< knock on the barrier
    void wait() const; ///< wait until barrier opens
    bool wait(const struct timespec& timeout) const; ///< wait until barrier opens
    void expect() const; ///< expect the barrier to open
    bool expect(const struct timespec& timeout) const; ///< expect the barrier to open
private:
    typedef boost::interprocess::interprocess_mutex locker_type;
    mutable locker_type m_locker;
    mutable boost::interprocess::interprocess_semaphore m_barrier;
    mutable volatile uint32_t m_counter1;
    mutable volatile uint32_t m_counter2;
};

/**
 * The simple spinlock
 */
class spinlock
{
public:
    enum lock_state
    {
        LS_LOCKED,
        LS_UNLOCKED
    };
    void lock();
    bool timed_lock(const struct timespec& timeout);
    bool try_lock();
    void unlock();
protected:
private:
    spinlock(const spinlock& );
    spinlock& operator=(const spinlock& );
private:
    volatile uint32_t m_lock;
};

#define QBUS_SPINLOCK_INIT_LOCKED = { qbus::spinlock::LS_UNLOCKED }
#define QBUS_SPINLOCK_INIT_ULOCKED = { qbus::spinlock::LS_UNLOCKED }

//==============================================================================
//  scoped_lock
//==============================================================================
/**
 * Constructor
 * Set the exclusive lock immediately
 * @param locker the slave locker
 */
template <typename Locker>
scoped_lock<Locker>::scoped_lock(locker_type& locker) :
    m_locker(locker),
    m_locked(false)
{
    lock();
}

/**
 * Constructor
 * Defer the exclusive lock
 * @param locker the slave locker
 * @param
 */
template <typename Locker>
scoped_lock<Locker>::scoped_lock(locker_type& locker, defer_lock_type) :
    m_locker(locker),
    m_locked(false)
{}

/**
 * Constructor
 * Try to set the exclusive lock immediately
 * @param locker the slave locker
 * @param
 */
template <typename Locker>
scoped_lock<Locker>::scoped_lock(locker_type& locker, try_to_lock_type) :
    m_locker(locker),
    m_locked(m_locker.try_lock())
{}

/**
 * Constructor
 * Try to set the exclusive lock immediately
 * @param locker the slave locker
 * @param timeout the allowable timeout of the setting
 */
template <typename Locker>
scoped_lock<Locker>::scoped_lock(locker_type& locker, const struct timespec& timeout) :
    m_locker(locker),
    m_locked(m_locker.timed_lock(timeout))
{}

/**
 * Destructor
 * Remove the exclusive lock
 */
template <typename Locker>
scoped_lock<Locker>::~scoped_lock()
{
    try
    {
        if (m_locked)
        {
            m_locker.unlock();
        }
    }
    catch (...)
    {
    }
}

/**
 * Set the exclusive lock
 */
template <typename Locker>
void scoped_lock<Locker>::lock()
{
    if (m_locked)
    {
        throw lock_exception();
    }
    m_locker.lock();
    m_locked = true;
}

/**
 * Try to set the exclusive lock
 * @return
 */
template <typename Locker>
bool scoped_lock<Locker>::try_lock()
{
    if (m_locked)
    {
        throw lock_exception();
    }
    m_locked = m_locker.try_lock();
    return m_locked;
}

/**
 * Try to set the exclusive lock until the time comes
 * @param timeout the allowable timeout of the setting
 * @return the result of the setting
 */
template <typename Locker>
bool scoped_lock<Locker>::timed_lock(const struct timespec& timeout)
{
    if (m_locked)
    {
        throw lock_exception();
    }
    m_locked = m_locker.timed_lock(timeout);
    return m_locked;
}

/**
 * Remove the exclusive lock
 */
template <typename Locker>
void scoped_lock<Locker>::unlock()
{
    if (!m_locked)
    {
        throw lock_exception();
    }
    m_locker.unlock();
    m_locked = false;
}

/**
 * Check the lock is locked
 * @return the result of the checking
 */
template <typename Locker>
bool scoped_lock<Locker>::owns() const
{
    return m_locked;
}

//==============================================================================
//  sharable_lock
//==============================================================================
/**
 * Constructor
 * Set the sharable lock immediately
 * @param locker the slave locker
 */
template <typename Locker>
sharable_lock<Locker>::sharable_lock(locker_type& locker) :
    m_locker(locker),
    m_locked(false)
{
    lock();
}

/**
 * Constructor
 * Defer the sharable lock
 * @param locker the slave locker
 * @param
 */
template <typename Locker>
sharable_lock<Locker>::sharable_lock(locker_type& locker, defer_lock_type) :
    m_locker(locker),
    m_locked(false)
{}

/**
 * Constructor
 * Try to set the sharable lock immediately
 * @param locker the slave locker
 * @param
 */
template <typename Locker>
sharable_lock<Locker>::sharable_lock(locker_type& locker, try_to_lock_type) :
    m_locker(locker),
    m_locked(m_locker.try_lock_sharable())
{}

/**
 * Constructor
 * Try to set the exclusive lock immediately
 * @param locker the slave locker
 * @param timeout the allowable timeout of the setting
 */
template <typename Locker>
sharable_lock<Locker>::sharable_lock(locker_type& locker, const struct timespec& timeout) :
    m_locker(locker),
    m_locked(m_locker.timed_lock_sharable(timeout))
{}

/**
 * Destructor
 * Remove the sharable lock
 */
template <typename Locker>
sharable_lock<Locker>::~sharable_lock()
{
    try
    {
        if (m_locked)
        {
            m_locker.unlock_sharable();
        }
    }
    catch (...)
    {
    }
}

/**
 * Set the sharable lock
 */
template <typename Locker>
void sharable_lock<Locker>::lock()
{
    if (m_locked)
    {
        throw lock_exception();
    }
    m_locker.lock_sharable();
    m_locked = true;
}

/**
 * Try to set the sharable lock
 * @return
 */
template <typename Locker>
bool sharable_lock<Locker>::try_lock()
{
    if (m_locked)
    {
        throw lock_exception();
    }
    m_locked = m_locker.try_lock_sharable();
    return m_locked;
}

/**
 * Try to set the sharable lock until the time comes
 * @param timeout the allowable timeout of the setting
 * @return the result of the setting
 */
template <typename Locker>
bool sharable_lock<Locker>::timed_lock(const struct timespec& timeout)
{
    if (m_locked)
    {
        throw lock_exception();
    }
    m_locked = m_locker.timed_lock_sharable(timeout);
    return m_locked;
}

/**
 * Remove the sharable lock
 */
template <typename Locker>
void sharable_lock<Locker>::unlock()
{
    if (!m_locked)
    {
        throw lock_exception();
    }
    m_locker.unlock_sharable();
    m_locked = false;
}

/**
 * Check the lock is locked
 * @return the result of the checking
 */
template <typename Locker>
bool sharable_lock<Locker>::owns() const
{
    return m_locked;
}

} //namespace qbus

#endif /* QBUS_LOCKER_H */

