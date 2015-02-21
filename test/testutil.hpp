// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#include <apollo/lua_include.hpp>

struct lstate_fixture {
    lua_State* L;

    lstate_fixture();
    ~lstate_fixture();
};

void check_dostring(lua_State* L, char const* s);
void require_dostring(lua_State* L, char const* s);
