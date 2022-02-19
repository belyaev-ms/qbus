#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE bus_test
#include <boost/test/unit_test.hpp>

#include "qbus/bus.h"
#include <vector>

typedef std::vector<uint8_t> buffer_t;

static buffer_t make_buffer(const size_t size)
{
    buffer_t buffer(size);
    for (size_t i = 0; i < size; ++i)
    {
        buffer[i] = i;
    }
    return buffer;
}

using namespace qbus;

BOOST_AUTO_TEST_CASE(simple_test)
{
    pmessage_type pmessage;
    pbus_type pbus1 = bus::make<single_output_bus_type>("test");
    pbus_type pbus2 = bus::make<single_input_bus_type>("test");
    BOOST_REQUIRE(pbus1);
    BOOST_REQUIRE(pbus2);
    bus::specification_type spec;
    spec.id = 1;
    spec.keepalive_timeout = 0;
    spec.min_capacity = 32 * 512;
    spec.max_capacity = 64 * 512;
    spec.capacity_factor = 50;
    BOOST_REQUIRE(pbus1->create(spec));
    BOOST_REQUIRE(pbus2->open());
    BOOST_REQUIRE(!pbus1->get());
    BOOST_REQUIRE(!pbus1->pop());
    BOOST_REQUIRE(!pbus2->get());
    buffer_t buffer = make_buffer(512);
    BOOST_REQUIRE(pbus1->push(0, &buffer[0], buffer.size()));
    pmessage = pbus2->get();
    BOOST_REQUIRE(pmessage);
    BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
    buffer_t data(pmessage->data_size());
    pmessage->unpack(&data[0]);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(),
        data.begin(), data.end());
    BOOST_REQUIRE(pbus2->pop());
    pmessage = pbus2->get();
    BOOST_REQUIRE(!pmessage);
}

BOOST_AUTO_TEST_CASE(overflow_test)
{
    pmessage_type pmessage;
    pbus_type pbus1 = bus::make<single_output_bus_type>("test");
    pbus_type pbus2 = bus::make<single_input_bus_type>("test");
    BOOST_REQUIRE(pbus1);
    BOOST_REQUIRE(pbus2);
    bus::specification_type spec;
    spec.id = 1;
    spec.keepalive_timeout = 0;
    spec.min_capacity = 32 * 512;
    spec.max_capacity = 64 * 512;
    spec.capacity_factor = 50;
    BOOST_REQUIRE(pbus1->create(spec));
    BOOST_REQUIRE(pbus2->open());
    BOOST_REQUIRE(!pbus1->get());
    BOOST_REQUIRE(!pbus1->pop());
    BOOST_REQUIRE(!pbus2->get());
    buffer_t buffer = make_buffer(512);
    for (size_t i = 0; i < 48; ++i)
    {
        BOOST_TEST_MESSAGE("push " << i << " message");
        BOOST_REQUIRE(pbus1->push(0, &buffer[0], buffer.size()));
    }
    for (size_t i = 0; i < 48; ++i)
    {
        BOOST_TEST_MESSAGE("get " << i << " message");
        pmessage = pbus2->get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        buffer_t data(pmessage->data_size());
        pmessage->unpack(&data[0]);
        BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(),
            data.begin(), data.end());
        BOOST_REQUIRE(pbus2->pop());
    }
    pmessage = pbus2->get();
    BOOST_REQUIRE(!pmessage);
}

BOOST_AUTO_TEST_CASE(overflow_test1)
{
    pmessage_type pmessage;
    pbus_type pbus1 = bus::make<single_output_bus_type>("test");
    pbus_type pbus2 = bus::make<single_input_bus_type>("test");
    BOOST_REQUIRE(pbus1);
    BOOST_REQUIRE(pbus2);
    bus::specification_type spec;
    spec.id = 1;
    spec.keepalive_timeout = 0;
    spec.min_capacity = 8 * 512;
    spec.max_capacity = 64 * 512;
    spec.capacity_factor = 50;
    BOOST_REQUIRE(pbus1->create(spec));
    BOOST_REQUIRE(pbus2->open());
    BOOST_REQUIRE(!pbus1->get());
    BOOST_REQUIRE(!pbus1->pop());
    BOOST_REQUIRE(!pbus2->get());
    buffer_t buffer = make_buffer(512);
    for (size_t i = 0; i < 48; ++i)
    {
        BOOST_TEST_MESSAGE("push " << i << " message");
        BOOST_REQUIRE(pbus1->push(0, &buffer[0], buffer.size()));
    }
    for (size_t i = 0; i < 48; ++i)
    {
        BOOST_TEST_MESSAGE("get " << i << " message");
        pmessage = pbus2->get();
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        buffer_t data(pmessage->data_size());
        pmessage->unpack(&data[0]);
        BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(),
            data.begin(), data.end());
        BOOST_REQUIRE(pbus2->pop());
    }
    pmessage = pbus2->get();
    BOOST_REQUIRE(!pmessage);
}

BOOST_AUTO_TEST_CASE(overflow_test2)
{
    struct timespec timeout = { 0, 1000000 };
    pmessage_type pmessage;
    pbus_type pbus1 = bus::make<single_output_bus_type>("test");
    pbus_type pbus2 = bus::make<single_input_bus_type>("test");
    BOOST_REQUIRE(pbus1);
    BOOST_REQUIRE(pbus2);
    bus::specification_type spec;
    spec.id = 1;
    spec.keepalive_timeout = 0;
    spec.min_capacity = 32 * 512;
    spec.max_capacity = 64 * 512;
    spec.capacity_factor = 50;
    BOOST_REQUIRE(pbus1->create(spec));
    BOOST_REQUIRE(pbus2->open());
    BOOST_REQUIRE(!pbus1->get(timeout));
    BOOST_REQUIRE(!pbus1->pop(timeout));
    BOOST_REQUIRE(!pbus2->get(timeout));
    buffer_t buffer = make_buffer(512);
    for (size_t i = 0; i < 48; ++i)
    {
        BOOST_TEST_MESSAGE("push " << i << " message");
        BOOST_REQUIRE(pbus1->push(0, &buffer[0], buffer.size(), timeout));
    }
    for (size_t i = 0; i < 48; ++i)
    {
        BOOST_TEST_MESSAGE("get " << i << " message");
        pmessage = pbus2->get(timeout);
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        buffer_t data(pmessage->data_size());
        pmessage->unpack(&data[0]);
        BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(),
            data.begin(), data.end());
        BOOST_REQUIRE(pbus2->pop(timeout));
    }
    pmessage = pbus2->get(timeout);
    BOOST_REQUIRE(!pmessage);
}

BOOST_AUTO_TEST_CASE(overflow_test3)
{
    struct timespec timeout = { 0, 1000000 };
    pmessage_type pmessage;
    pbus_type pbus1 = bus::make<single_output_bus_type>("test");
    pbus_type pbus2 = bus::make<single_input_bus_type>("test");
    BOOST_REQUIRE(pbus1);
    BOOST_REQUIRE(pbus2);
    bus::specification_type spec;
    spec.id = 1;
    spec.keepalive_timeout = 0;
    spec.min_capacity = 8 * 512;
    spec.max_capacity = 64 * 512;
    spec.capacity_factor = 50;
    BOOST_REQUIRE(pbus1->create(spec));
    BOOST_REQUIRE(pbus2->open());
    BOOST_REQUIRE(!pbus1->get(timeout));
    BOOST_REQUIRE(!pbus1->pop(timeout));
    BOOST_REQUIRE(!pbus2->get(timeout));
    buffer_t buffer = make_buffer(512);
    for (size_t i = 0; i < 48; ++i)
    {
        BOOST_TEST_MESSAGE("push " << i << " message");
        BOOST_REQUIRE(pbus1->push(0, &buffer[0], buffer.size(), timeout));
    }
    for (size_t i = 0; i < 48; ++i)
    {
        BOOST_TEST_MESSAGE("get " << i << " message");
        pmessage = pbus2->get(timeout);
        BOOST_REQUIRE(pmessage);
        BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
        buffer_t data(pmessage->data_size());
        pmessage->unpack(&data[0]);
        BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(),
            data.begin(), data.end());
        BOOST_REQUIRE(pbus2->pop(timeout));
    }
    pmessage = pbus2->get(timeout);
    BOOST_REQUIRE(!pmessage);
}


