#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE connector_test
#include <boost/test/unit_test.hpp>

#include "qbus/connector.h"
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
    pconnector_type pconnector1 = connector::make<out_connector_type>("test");
    pconnector_type pconnector2 = connector::make<in_connector_type>("test");
    BOOST_REQUIRE(pconnector1);
    BOOST_REQUIRE(pconnector2);
    BOOST_REQUIRE(pconnector1->create(512, 32));
    BOOST_REQUIRE(pconnector2->open());
    BOOST_REQUIRE(!pconnector1->get());
    BOOST_REQUIRE(!pconnector1->pop());
    BOOST_REQUIRE(!pconnector2->get());
    buffer_t buffer = make_buffer(512);
    BOOST_REQUIRE(pconnector1->add(&buffer[0], buffer.size()));
    pmessage = pconnector2->get();
    BOOST_REQUIRE(pmessage);
    BOOST_REQUIRE_EQUAL(pmessage->data_size(), buffer.size());
    buffer_t data(pmessage->data_size());
    pmessage->unpack(&data[0]);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(buffer.begin(), buffer.end(),
        data.begin(), data.end());
    BOOST_REQUIRE(pconnector2->pop());
    pmessage = pconnector2->get();
    BOOST_REQUIRE(!pmessage);
}

