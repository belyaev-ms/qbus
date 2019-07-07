#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H
#include <exception>

namespace dmux
{

class base_exception : public std::exception {};

class lock_exception : public base_exception
{
public:
    virtual const char* what() const throw()
    {
        return "dmux::lock_exception";
    }
};

} //namespace dmux

#endif /* EXCEPTIONS_H */

