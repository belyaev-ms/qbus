#ifndef QBUS_EXCEPTIONS_H
#define QBUS_EXCEPTIONS_H
#include <exception>

namespace qbus
{

class base_exception : public std::exception {};

class lock_exception : public base_exception
{
public:
    virtual const char* what() const throw()
    {
        return "qbus::lock_exception";
    }
};

} //namespace qbus

#endif /* QBUS_EXCEPTIONS_H */

