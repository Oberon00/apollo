// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#include <apollo/builtin_types.hpp>
#include <apollo/error.hpp>
#include <apollo/gc.hpp>
#include <apollo/lapi.hpp>

namespace {

struct test_cls {
    static unsigned n_destructions;

    ~test_cls() { ++test_cls::n_destructions; }
    test_cls(int v_): v(v_) {}
    test_cls(test_cls const&) = default;

    int v;
};

unsigned test_cls::n_destructions = 0;

} // Anonymous namespace

#include "test_prefix.hpp"


BOOST_AUTO_TEST_CASE(lapi_pcall)
{
    BOOST_REQUIRE_EQUAL(luaL_loadstring(L, "(nil)()"), LUA_OK);
    lua_pushvalue(L, -1);
    std::string expected_msg = "attempt to call a nil value";
    auto const check_pcall_err =
        [this, &expected_msg](apollo::lua_api_error const& e) -> bool {
            namespace ei = apollo::errinfo;
            BOOST_CHECK_EQUAL(
                *boost::get_error_info<ei::lua_state>(e), L);
            BOOST_CHECK_EQUAL(
                *boost::get_error_info<ei::lua_error_code>(e), LUA_ERRRUN);
            std::string const msg = *boost::get_error_info<ei::lua_msg>(e);
            if (msg.find(expected_msg) == std::string::npos)
                BOOST_ERROR("Wrong lua message: " + msg);
            return true;
    };
    BOOST_CHECK_EXCEPTION(
        apollo::pcall(L, 0, 0), apollo::lua_api_error, check_pcall_err);

    expected_msg = "message handler";
    BOOST_REQUIRE_EQUAL(
        luaL_loadstring(L, ("return '" + expected_msg + "'").c_str()), LUA_OK);
    lua_pushvalue(L, -1);
    apollo::set_error_msg_handler(L);
    BOOST_REQUIRE(apollo::push_error_msg_handler(L));
    BOOST_CHECK(lua_rawequal(L, -1, -2));
    lua_pop(L, 1);
    lua_insert(L, -2); // Move msgh below pcalled function.
    lua_pushvalue(L, -1); // Copy function: We want to call it twice.
    BOOST_CHECK_EXCEPTION(
        apollo::pcall(L, 0, 0), apollo::lua_api_error, check_pcall_err);
    BOOST_CHECK_EXCEPTION(
        apollo::pcall(L, 0, 0, -2), apollo::lua_api_error, check_pcall_err);
    lua_pop(L, 1); // Pop message handler
}

BOOST_AUTO_TEST_CASE(gc)
{
    test_cls::n_destructions = 0;
    static_assert(!std::is_trivially_destructible<test_cls>::value, "");

    apollo::push_bare_udata(L, test_cls(42));
    BOOST_CHECK_EQUAL(test_cls::n_destructions, 1u);
    BOOST_CHECK_EQUAL(static_cast<test_cls*>(lua_touserdata(L, -1))->v, 42);
    BOOST_CHECK_EQUAL(apollo::gc_object<test_cls>(L), 0);
    BOOST_CHECK_EQUAL(test_cls::n_destructions, 2u);
    lua_pop(L, 1); // Pop udata

    apollo::push_gc_object(L, test_cls(0xcafe));
    BOOST_CHECK_EQUAL(test_cls::n_destructions, 3u);
    BOOST_CHECK_EQUAL(static_cast<test_cls*>(lua_touserdata(L, -1))->v, 0xcafe);
    BOOST_REQUIRE(lua_getmetatable(L, -1));
    lua_getfield(L, -1, "__gc");
    BOOST_CHECK_EQUAL(lua_tocfunction(L, -1), &apollo::gc_object<test_cls>);
    lua_pop(L, 2); // Pop metatable and __gc function.
    lua_pop(L, 1);
    lua_gc(L, LUA_GCCOLLECT, 0);
    lua_gc(L, LUA_GCCOLLECT, 0);
    BOOST_CHECK_EQUAL(test_cls::n_destructions, 4u);
}

static int testthrower(lua_State* L)
{
    apollo::exceptions_to_lua_errors(L, [](int v) -> void {
        BOOST_CHECK_EQUAL(v, 42);
        throw std::runtime_error("test throw");
    }, 42);
    return 0;
}

BOOST_AUTO_TEST_CASE(errors)
{
    lua_pushcfunction(L, &testthrower);
    BOOST_REQUIRE_EQUAL(lua_pcall(L, 0, 0, 0), LUA_ERRRUN);
    BOOST_CHECK_EQUAL(lua_type(L, -1), LUA_TSTRING);
    BOOST_CHECK_EQUAL(lua_tostring(L, -1), "exception: test throw");
    lua_pop(L, 1);
}

#include "test_suffix.hpp"
