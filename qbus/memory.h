#ifndef QBUS_MEMORY_H
#define QBUS_MEMORY_H

#include <stddef.h>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

namespace qbus
{
namespace memory
{

class shared_memory
{
public:
    explicit shared_memory(const std::string& name);
    virtual ~shared_memory();
    const bool create(const size_t sz); ///< create the memory
    const bool open(); ///< open the memory
    const size_t size() const; ///< get the size of the memory
    void *get(); ///< get the pointer to the memory
protected:
    void remove(); ///< remove the memory
private:
    typedef boost::interprocess::shared_memory_object memory_type;
    typedef boost::shared_ptr<memory_type> pmemory_type;
    typedef boost::interprocess::mapped_region region_type;
    typedef boost::shared_ptr<region_type> pregion_type;
private:
    const std::string& m_name;
    pmemory_type m_pmemory;
    pregion_type m_pregion;
};

} //namespace memory

typedef memory::shared_memory shared_memory_type;
typedef boost::shared_ptr<shared_memory_type> pshared_memory_type;

} //namespace qbus

#endif /* QBUS_MEMORY_H */

