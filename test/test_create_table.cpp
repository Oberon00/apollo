#include <apollo/create_table.hpp>
#include <apollo/converters.hpp>
#include <apollo/function.hpp>

#include "test_prefix.hpp"

static double add(double a, double b)
{
    return a + b;
}

BOOST_AUTO_TEST_CASE(set_table)
{
    luaL_requiref(L, "base", &luaopen_base, true);
    lua_pop(L, 1);
    lua_pushglobaltable(L);
    apollo::rawset_table(L, -1)
        (1, "somevalue")
        ("mykey", "myvalue")
        ("add", &add);
    lua_pop(L, 1);
    if (luaL_dostring(L,
        "assert(_G[1] == 'somevalue', 'invalid _G[1]')\n"
        "assert(mykey == 'myvalue', 'invalid mykey')\n"
        "assert(add(2, 3) == 5, 'invalid add return value')\n")
    ) {
        BOOST_ERROR(lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}

BOOST_AUTO_TEST_CASE(new_table)
{
    apollo::new_table(L)
        (1, 2)
        ("foo", "bar");
    lua_rawgeti(L, -1, 1);
    BOOST_CHECK_EQUAL(lua_tointeger(L, -1), 2);
    lua_pop(L, 1);
    lua_getfield(L, -1, "foo");
    BOOST_CHECK_EQUAL(lua_tostring(L, -1), "bar");
    lua_pop(L, 2);
}

#include "test_suffix.hpp"
