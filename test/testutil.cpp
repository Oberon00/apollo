#include "testutil.hpp"

#include <apollo/stack_balance.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_monitor.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <exception> // std::terminate()
#include <iostream>  // std::cerr

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

lstate_fixture::lstate_fixture()
{
    L = luaL_newstate();
    BOOST_REQUIRE(L);
}

lstate_fixture::~lstate_fixture()
{
    BOOST_CHECK_EQUAL(lua_gettop(L), 0);
    lua_close(L);
}

void check_dostring(lua_State* L, char const* s)
{
    if (luaL_dostring(L, s)) {
        BOOST_ERROR(lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

void require_dostring(lua_State* L, char const* s)
{
    apollo::stack_balance balance(L);
    if (luaL_dostring(L, s))
        BOOST_FAIL(lua_tostring(L, -1));
}
