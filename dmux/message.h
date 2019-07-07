#ifndef DMUX_MESSAGE_H
#define DMUX_MESSAGE_H

#include <stddef.h>
#include <stdint.h>
#include <boost/shared_ptr.hpp>

namespace dmux
{
namespace message
{

typedef uint32_t id_type;
typedef uint32_t flags_type;

enum
{
    FLG_HEAD = 1,
    FLG_TAIL = 2
};

enum
{
    ID_OFFSET    = 0,
    ID_SIZE      = sizeof(id_type),
    FLAGS_OFFSET = ID_OFFSET + ID_SIZE,
    FLAGS_SIZE   = sizeof(flags_type),
    COUNT_OFFSET = FLAGS_OFFSET + FLAGS_SIZE,
    COUNT_SIZE   = sizeof(uint32_t),
    DATA_OFFSET  = COUNT_OFFSET + COUNT_SIZE,
    HEADER_SIZE  = DATA_OFFSET
};

/**
 * The base message
 */
class base_message
{
public:
    typedef boost::shared_ptr<base_message> pmessage_type;
    
    base_message();
    virtual ~base_message();
    const id_type id() const; ///< get the identificator of the message
    void id(const id_type id); ///< set the identificator of the message
    const flags_type flags() const; ///< get the flags of the message
    void attach(const pmessage_type pmessage); ///< attach a message
    void pack(const void *source, const size_t size); ///< pack the data to the message
    void unpack(void *dest) const; ///< unpack the data to the message
    const size_t data_size() const; ///< get the size of attached data
    const size_t capacity() const; ///< get the capacity of the message
protected:
    base_message(void *ptr, const size_t sz);
    void flags(const flags_type flags); ///< set the flags of the message
    const size_t size() const; ///< get the size of data of the message
    void size(const size_t sz); ///< set the size of data of the message
    void clear(); ///< clear the message
    void *data(); ///< get the pointer to data of the message
    const void *data() const; ///< get the pointer to data of the message
    virtual pmessage_type make_message() = 0; ///< make the new message
private:
    const flags_type do_pack(const void *data, const size_t sz); ///< pack the data to the message
private:
    uint8_t *m_ptr; ///< the pointer to the raw message
    const size_t m_size; ///< the size of the raw message
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
    typedef Queue queue_type;
    message();
    message(queue_type *queue, void *ptr);
protected:
    virtual pmessage_type make_message(); ///< make the new message
private:
    queue_type *m_queue;
};

/**
 * Constructor
 */
template <typename Queue>
message<Queue>::message() :
    base_message(),
    m_queue(NULL)
{
}

/**
 * Constructor
 * @param queue the parent queue
 * @param ptr the pointer to the raw message
 */
template <typename Queue>
message<Queue>::message(queue_type *queue, void *ptr) :
    base_message(ptr, queue->slice_size()),
    m_queue(queue)
{
}

/**
 * Make the new message
 * @return the new message
 */
//virtual
template <typename Queue>
pmessage_type message<Queue>::make_message()
{
    return m_queue->make();
}

} //namespace message

typedef dmux::message::pmessage_type pmessage_type;

} //namespace dmux

#endif /* DMUX_MESSAGE_H */