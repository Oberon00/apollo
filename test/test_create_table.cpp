// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#include <apollo/builtin_types.hpp>
#include <apollo/create_table.hpp>
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
    check_dostring(L,
        "assert(_G[1] == 'somevalue', 'invalid _G[1]')\n"
        "assert(mykey == 'myvalue', 'invalid mykey')\n"
        "assert(add(2, 3) == 5, 'invalid add return value')\n");
}

static void checktable(lua_State* L)
{
    lua_setglobal(L, "t");
    check_dostring(L,
        "assert(type(t) == 'table', 't not a table but ' .. tostring(t))\n"
        "assert(t[1] == 2, tostring(t[1]) .. ' ~= 2')\n"
        "assert(t.foo == 'bar', tostring(t.foo) .. ' ~= \"bar\"')\n"
        "assert(t.sub.add(2, 3) == 5, 'invalid add return value')\n"
        "assert(t.sub[2] == 42, tostring(t.sub[2]) .. ' ~= 42')\n"
        "assert(t.sub2[3] == 7, tostring(t.sub2[3]) .. ' ~= 7')\n"
        "assert(t.sub2.nested.x == 'y',\n"
        "   tostring(t.sub2.nested.x) .. ' ~= \"y\"')\n"
        "assert(type(t.sub2.nested[1]) == 'table',\n"
        "   type(tostring(t.sub2.nested[1])) .. ' ~= \"table\"')\n"
        "assert(t.sub[3] == nil, tostring(t.sub[3]) .. ' ~= nil')\n"
        "print(t, t.sub, t.sub2)\n");
}

BOOST_AUTO_TEST_CASE(nested_table)
{
    luaL_requiref(L, "base", &luaopen_base, true);
    lua_pop(L, 1);
    apollo::new_table(L)
        (1, 2)
        ("foo", "bar")
        .subtable("sub")
            ("add", &add)
            (2, 42)
        .end_subtable()
        .subtable("sub2")
            (3, 7)
            .subtable("nested")
                ("x", "y")
                .subtable(1)
                .end_subtable()
            .end_subtable()
        .end_subtable();
    checktable(L);

    lua_newtable(L);
    apollo::rawset_table(L, -1)
        (1, 2)
        ("foo", "bar")
        .subtable("sub")
            ("add", &add)
            (2, 42)
        .end_subtable()
        .subtable("sub2")
            (3, 7)
            .subtable("nested")
                ("x", "y")
                .subtable(1)
                .end_subtable()
            .end_subtable()
        .end_subtable();
    checktable(L);
}

#include "test_suffix.hpp"
