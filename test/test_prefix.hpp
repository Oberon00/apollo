#ifdef APOLLO_TEST_PREFIX_HPP_INCLUDED
#    error Test prefix may only be included once.
#endif
#define APOLLO_TEST_PREFIX_HPP_INCLUDED APOLLO_TEST_PREFIX_HPP_INCLUDED

#include <boost/preprocessor/cat.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_monitor.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <lua.hpp>

#include <exception> // std::terminate()
#include <iostream>  // std::cerr

struct lstate_fixture {
    lua_State* L;

    lstate_fixture()
    {
        L = luaL_newstate();
        if (!L) {
            std::cerr << "luaL_newstate() failed, probably out of memory.\n";
            std::cerr << "Terminating...\n";
            std::terminate();
        }
    }
    ~lstate_fixture() {
        BOOST_CHECK_EQUAL(lua_gettop(L), 0);
        lua_close(L);
    }
};

// See http://stackoverflow.com/a/5402543/2128694 for boost::exception support

inline void translateBoostException(const boost::exception &e)
{
    BOOST_FAIL(boost::diagnostic_information(e));
}

struct boost_exception_fixture
{
    boost_exception_fixture()
    {
        boost::unit_test::unit_test_monitor.register_exception_translator<
            boost::exception>(&translateBoostException);
    }
};


BOOST_GLOBAL_FIXTURE(boost_exception_fixture)

BOOST_FIXTURE_TEST_SUITE(
    BOOST_PP_CAT(BOOST_TEST_MODULE, _suite),
    lstate_fixture)
