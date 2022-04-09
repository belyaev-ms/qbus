#ifndef QBUS_SERVICE_MESSAGE_H
#define QBUS_SERVICE_MESSAGE_H

#include "qbus/message.h"
#include <limits>

namespace qbus
{
    
namespace message
{
    
/**
 * The service message of a queue
 */
template <typename Message>
class service_message : public Message
{
public:
    enum
    {
        TAG = std::numeric_limits<tag_type>::max()
    };
    enum code_type
    {
        CODE_CONNECT,
        CODE_DISCONNECT,
        CODE_COUNT
    };
    uint8_t code() const
    {
        return *reinterpret_cast<const uint8_t*>(Message::data());
    }
};
   
} //namespace message

} //namespace qbus


#endif /* QBUS_SERVICE_MESSAGE_H */

