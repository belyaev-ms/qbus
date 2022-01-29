#include "qbus/locker.h"
#include "qbus/common.h"

#include <assert.h>
#include <limits.h>
#include <boost/thread/thread_time.hpp>
#include <boost/interprocess/detail/atomic.hpp>

namespace qbus
{

//==============================================================================
//  shared_locker
//==============================================================================

/**
 * Constructor
 */
shared_locker::shared_locker() :
#if __cplusplus >= 201103L
    m_lock{ BOOST_DETAIL_SPINLOCK_INIT },
#endif
    m_scoped(0),
    m_sharable(0)
{
#if __cplusplus < 201103L
    m_lock = BOOST_DETAIL_SPINLOCK_INIT;
#endif
}

/**
 * Try to book the exclusive lock
 * @return the type of the current lock
 */
shared_locker::lock_state shared_locker::try_book_lock()
{
    guard_type guard(m_lock);
    if (0 == m_scoped)
    {
        m_scoped = 1;
        return 0 == m_sharable ? LS_NONE : LS_SHARABLE;
    }
    return LS_SCOPED;
}

/**
 * Get the count of the sharable lock
 * @return the count of the sharable lock
 */
unsigned int shared_locker::count_sharable()
{
    guard_type guard(m_lock);
    return m_sharable;
}

/**
 * Try to set the exclusive lock
 * @return the result of the setting
 */
bool shared_locker::try_lock()
{
    guard_type guard(m_lock);
    if (0 == m_scoped && 0 == m_sharable)
    {
        m_scoped = 1;
        return true;
    }
    return false;
}

/**
 * Set the exclusive lock
 */
void shared_locker::lock()
{
    unsigned int k = 0;
    while (1)
    {
        switch (try_book_lock())
        {
            case LS_NONE:
                return;
            case LS_SCOPED:
                break;
            case LS_SHARABLE:
                while (count_sharable() > 0)
                {
                    boost::detail::yield(k++);
                }
                return;
        }
        boost::detail::yield(k++);
    }
}

/**
 * Try to set the exclusive lock until the time comes
 * @param timeout the allowable timeout of the setting
 * @return the result of the setting
 */
bool shared_locker::timed_lock(const struct timespec& timeout)
{
    const struct timespec ts = get_monotonic_time() + timeout;
    unsigned int k = 0;
    while (get_monotonic_time() < ts)
    {
        switch (try_book_lock())
        {
            case LS_NONE:
                return true;
            case LS_SCOPED:
                break;
            case LS_SHARABLE:
                while (get_monotonic_time() < ts)
                {
                    if (0 == count_sharable())
                    {
                        return true;
                    }
                    boost::detail::yield(k++);
                }
                unlock();
                return false;
        }
        boost::detail::yield(k++);
    }
    return false;
}

/**
 * Remove the exclusive lock
 */
void shared_locker::unlock()
{
    guard_type guard(m_lock);
    assert(1 == m_scoped);
    m_scoped = 0;
}

/**
 * Try to set the sharable lock
 * @return the result of the setting
 */
bool shared_locker::try_lock_sharable()
{
    guard_type guard(m_lock);
    if (0 == m_scoped && m_sharable < UINT_MAX)
    {
        ++m_sharable;
        return true;
    }
    return false;
}

/**
 * Set the sharable lock
 */
void shared_locker::lock_sharable()
{
    unsigned int k = 0;
    while (1)
    {
        if (try_lock_sharable())
        {
            return;
        }
        boost::detail::yield(k++);
    }
}

/**
 * Try to set the sharable lock until the time comes
 * @param timeout the allowable timeout of the setting
 * @return the result of the setting
 */
bool shared_locker::timed_lock_sharable(const struct timespec& timeout)
{
    const struct timespec ts = get_monotonic_time() + timeout;
    unsigned int k = 0;
    while (get_monotonic_time() < ts)
    {
        if (try_lock_sharable())
        {
            return true;
        }
        boost::detail::yield(k++);
    }
    return false;
}

/**
 * Remove the sharable lock
 */
void shared_locker::unlock_sharable()
{
    guard_type guard(m_lock);
    assert(m_sharable > 0);
    --m_sharable;
}

//==============================================================================
//  shared_barrier
//==============================================================================
/**
 * Constructor
 */
shared_barrier::shared_barrier() :
    m_barrier(0),
    m_counter1(0),
    m_counter2(0)
{
}

/**
 * Open the barrier
 */
void shared_barrier::open()
{
    scoped_lock<locker_type> lock(m_locker);
    while (m_counter1 > 0)
    {
        --m_counter1;
        ++m_counter2;
        m_barrier.post();
    }
}

/**
 * Knock on the barrier
 */
void shared_barrier::knock() const
{
    while (m_counter2 > 0)
    {
        sched_yield();
    }
    scoped_lock<locker_type> lock(m_locker);
    ++m_counter1;
}

/**
 * Wait until barrier opens
 */
void shared_barrier::wait() const
{
    knock();
    expect();
}

/**
 * Wait until barrier opens
 * @param the allowable timeout of the waiting
 * @return the result of the waiting
 */
bool shared_barrier::wait(const struct timespec& timeout) const
{
    knock();
    return expect(timeout);
}

/**
 * Expect the barrier to open
 */
void shared_barrier::expect() const
{
    m_barrier.wait();
    scoped_lock<locker_type> lock(m_locker);
    --m_counter2;
}

/**
 * Expect the barrier to open
 * @param the allowable timeout of the waiting
 * @return the result of the waiting
 */
bool shared_barrier::expect(const struct timespec& timeout) const
{
    boost::posix_time::ptime abs_time = boost::get_system_time() +
        boost::posix_time::seconds(timeout.tv_sec) +
        boost::posix_time::microsec(timeout.tv_nsec / 1000);
    if (!m_barrier.timed_wait(abs_time))
    {
        scoped_lock<locker_type> lock(m_locker);
        if (m_counter2 == 0)
        {
            --m_counter1;
            return false;
        }
        --m_counter2;
        return true;
    }
    scoped_lock<locker_type> lock(m_locker);
    --m_counter2;
    return true;
}

//==============================================================================
//  spinlock
//==============================================================================

/**
 * Try to set the lock
 * @return the result of the setting
 */
bool spinlock::try_lock()
{
    if (LS_LOCKED == boost::interprocess::ipcdetail::atomic_cas32(&m_lock,
            LS_LOCKED, LS_UNLOCKED))
    {
        __sync_synchronize();
        return true;
    }
    return false;
}

/**
 * Set the lock
 */
void spinlock::lock()
{
    unsigned int k = 0;
    while (!try_lock())
    {
        boost::detail::yield(k++);
    }
}

/**
 * Try to set the lock until the time comes
 * @param timeout the allowable timeout of the setting
 * @return the result of the setting
 */
bool spinlock::timed_lock(const struct timespec& timeout)
{
    const struct timespec ts = get_monotonic_time() + timeout;
    unsigned int k = 0;
    while (get_monotonic_time() < ts)
    {
        if (try_lock())
        {
            return true;
        }
        boost::detail::yield(k++);
    }
    return false;
}

/**
 * Remove the lock
 */
void spinlock::unlock()
{
    boost::interprocess::ipcdetail::atomic_write32(&m_lock, LS_UNLOCKED);
    __sync_synchronize();
}

} //namespace qbus

