#include <apollo/lapi.hpp>
#include <apollo/error.hpp>

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
    lua_insert(L, -2); // Move msgh below pcalled function.
    lua_pushvalue(L, -1); // Copy function: We want to call it twice.
    BOOST_CHECK_EXCEPTION(
        apollo::pcall(L, 0, 0), apollo::lua_api_error, check_pcall_err);
    BOOST_CHECK_EXCEPTION(
        apollo::pcall(L, 0, 0, -2), apollo::lua_api_error, check_pcall_err);
    lua_pop(L, 1); // Pop message handler
}

BOOST_AUTO_TEST_CASE(lapi_rawgetset)
{
    // Make sure that nonraw gets and sets cause errors:
    luaL_requiref(L, "base", &luaopen_base, true);
    lua_pop(L, 1);
    BOOST_REQUIRE_EQUAL(LUA_OK, luaL_dostring(L,
        "setmetatable(_G, {\n"
        "__index = function() error('err index') end,\n"
        "__newindex = function() error('err newindex') end})"));
    BOOST_CHECK(luaL_dostring(L, "x = 0"));
    lua_pop(L, 1); // Pop error message
    BOOST_CHECK(luaL_dostring(L, "print(x)"));
    lua_pop(L, 1); // Pop error message

    // Test rawset:

}

#include "test_suffix.hpp"
