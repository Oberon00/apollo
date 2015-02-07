#include <apollo/builtin_types.hpp>
#include <apollo/default_argument.hpp>
#include <apollo/lapi.hpp>
#include <apollo/overload.hpp>
#include <apollo/to_raw_function.hpp>

#include <iostream>

#include "test_prefix.hpp"

namespace {

static unsigned g_n_calls = 0;
static unsigned g_n_s_calls = 0;
static unsigned g_n_i_calls = 0;
static unsigned g_n_si_calls = 0;
static unsigned g_n_is_calls = 0;

static void reset_calls()
{
    g_n_calls = 0;
    g_n_s_calls = g_n_i_calls = 0;
    g_n_si_calls = g_n_is_calls = 0;
}

static void proc0()
{
    ++g_n_calls;
}

/*static*/ void proc1(int i) // MSVC does not allow static fns as template arg.
{
    BOOST_CHECK_EQUAL(i, 42);
    ++g_n_calls;
    ++g_n_i_calls;
}

static void proc1s(std::string const& s)
{
    BOOST_CHECK_EQUAL(s, "foo");
    ++g_n_s_calls;
}

static void proc_si(std::string const& s, int i)
{
    BOOST_CHECK_EQUAL(s, "foo");
    BOOST_CHECK_EQUAL(i, 42);
    ++g_n_si_calls;
}

static void proc_is(int i, std::string const& s)
{
    BOOST_CHECK_EQUAL(i, 42);
    BOOST_CHECK_EQUAL(s, "foo");
    ++g_n_is_calls;
}

static void check_call_fails(
    lua_State* L, std::string const& expected, int nargs = 0)
{
    auto const check_err =
        [L, &expected](apollo::lua_api_error const& e) -> bool {
            namespace ei = apollo::errinfo;
            BOOST_CHECK_EQUAL(
                *boost::get_error_info<ei::lua_state>(e), L);
            BOOST_CHECK_EQUAL(
                *boost::get_error_info<ei::lua_error_code>(e), LUA_ERRRUN);
            std::string const msg = *boost::get_error_info<ei::lua_msg>(e);
            if (msg.find(expected) == std::string::npos)
                BOOST_ERROR("Wrong lua message: " + msg);
            else
                std::cout << "OK, got expected: " << msg << "\n";
            return true;
        };
    BOOST_CHECK_EXCEPTION(
        apollo::pcall(L, nargs, 0), apollo::lua_api_error, check_err);
}

static void check_none_viable(lua_State* L, int nargs = 0)
{
    check_call_fails(L, "No overload is viable", nargs);
}

static void check_ambiguous(lua_State* L, int nargs = 0)
{
    check_call_fails(L, "Ambiguous call", nargs);
}

} // anonymous namespace

BOOST_AUTO_TEST_CASE(single_overload)
{
    reset_calls();
    apollo::push(L, apollo::make_overloadset(&proc0));
    apollo::pcall(L, 0, 0);
    BOOST_CHECK_EQUAL(g_n_calls, 1u);

    apollo::push(L, apollo::make_overloadset(&proc1));
    lua_pushvalue(L, -1);
    lua_pushvalue(L, -1);

    check_none_viable(L, 0);

    lua_pushnil(L);
    check_none_viable(L, 1);

    lua_pushinteger(L, 42);
    apollo::pcall(L, 1, 0);
    BOOST_CHECK_EQUAL(g_n_i_calls, 1u);

    apollo::push(L, apollo::make_overloadset_with(
        apollo::make_function_with(&proc1,
            apollo::push_converter_for<void>(),
            apollo::make_default_arg(42))));
    apollo::pcall(L, 0, 0);
    BOOST_CHECK_EQUAL(g_n_i_calls, 2u);

    apollo::push(L, apollo::make_overloadset_with(
        apollo::make_function_with(&proc1,
            apollo::push_converter_for<void>(),
            apollo::make_default_arg(0xbad))));
    lua_pushinteger(L, 42);
    apollo::pcall(L, 1, 0);
    BOOST_CHECK_EQUAL(g_n_i_calls, 3u);

}

BOOST_AUTO_TEST_CASE(light_overload)
{
    apollo::push(L, apollo::make_overloadset_with(
        APOLLO_MAKE_LIGHT_FUNCTION(&proc1,
            apollo::push_converter_for<void>(),
            apollo::pull_converter_for<int>())));
    lua_pushinteger(L, 42);
    apollo::pcall(L, 1, 0);
    BOOST_CHECK_EQUAL(g_n_i_calls, 4u);
}

BOOST_AUTO_TEST_CASE(overload_resolution_singlearg)
{
    reset_calls();
    apollo::push(L, apollo::make_overloadset(&proc1, &proc1s));
    lua_pushvalue(L, -1);
    lua_pushvalue(L, -1);
    lua_pushvalue(L, -1);


    lua_pushinteger(L, 42);
    apollo::pcall(L, 1, 0);
    BOOST_CHECK_EQUAL(g_n_i_calls, 1u);

    lua_pushliteral(L, "foo");
    apollo::pcall(L, 1, 0);
    BOOST_CHECK_EQUAL(g_n_s_calls, 1u);

    check_none_viable(L, 0);

    lua_pushnil(L);
    check_none_viable(L, 1);

    apollo::push(L, apollo::make_overloadset_with(
        apollo::make_function_with(&proc1,
            apollo::push_converter_for<void>(),
            apollo::make_default_arg(42)),
        apollo::make_function(&proc1s)));
    lua_pushvalue(L, -1);
    lua_pushvalue(L, -1);
    lua_pushvalue(L, -1);

    apollo::pcall(L, 0, 0);
    BOOST_CHECK_EQUAL(g_n_i_calls, 2u);

    lua_pushinteger(L, 42);
    apollo::pcall(L, 1, 0);
    BOOST_CHECK_EQUAL(g_n_i_calls, 3u);

    lua_pushliteral(L, "foo");
    apollo::pcall(L, 1, 0);
    BOOST_CHECK_EQUAL(g_n_s_calls, 2u); // Should be called.
    BOOST_CHECK_EQUAL(g_n_i_calls, 3u); // Should not be called.

    lua_pushnil(L);
    check_none_viable(L, 1);
}

BOOST_AUTO_TEST_CASE(overload_resolution_2_args)
{
    reset_calls();
    apollo::push(L, apollo::make_overloadset(&proc_is, &proc_si));
    lua_pushvalue(L, -1);
    lua_pushvalue(L, -1);
    lua_pushvalue(L, -1);
    lua_pushvalue(L, -1);


    lua_pushinteger(L, 42);
    lua_pushliteral(L, "foo");
    apollo::pcall(L, 2, 0);
    BOOST_CHECK_EQUAL(g_n_is_calls, 1u);

    lua_pushliteral(L, "foo");
    lua_pushinteger(L, 42);
    apollo::pcall(L, 2, 0);
    BOOST_CHECK_EQUAL(g_n_si_calls, 1u);

    check_none_viable(L, 0);

    lua_pushliteral(L, "foo");
    check_none_viable(L, 1);

    lua_pushinteger(L, 42);
    check_none_viable(L, 1);

    apollo::push(L, apollo::make_overloadset_with(
        apollo::make_function_with(&proc_is,
            apollo::push_converter_for<void>(),
            apollo::make_default_arg(42),
            apollo::make_default_arg(std::string("foo"))),
        apollo::make_function(&proc_si)));
    lua_pushvalue(L, -1);
    lua_pushvalue(L, -1);
    lua_pushvalue(L, -1);

    apollo::pcall(L, 0, 0);
    BOOST_CHECK_EQUAL(g_n_is_calls, 2u);

    lua_pushinteger(L, 42);
    apollo::pcall(L, 1, 0);
    BOOST_CHECK_EQUAL(g_n_is_calls, 3u);

    lua_pushliteral(L, "foo");
    lua_pushinteger(L, 42);
    apollo::pcall(L, 2, 0);
    BOOST_CHECK_EQUAL(g_n_si_calls, 2u); // Should be called.
    BOOST_CHECK_EQUAL(g_n_is_calls, 3u); // Should not be called.

    lua_pushnil(L);
    lua_pushnil(L);
    check_none_viable(L, 2);
}

BOOST_AUTO_TEST_CASE(mixed_length)
{
    reset_calls();
    apollo::push(L, apollo::make_overloadset(
        &proc_is, &proc_si, &proc1, &proc1s));

    lua_pushvalue(L, -1);
    lua_pushliteral(L, "foo");
    apollo::pcall(L, 1, 0);
    BOOST_CHECK_EQUAL(g_n_s_calls, 1u);

    lua_pushvalue(L, -1);
    lua_pushinteger(L, 42);
    apollo::pcall(L, 1, 0);
    BOOST_CHECK_EQUAL(g_n_i_calls, 1u);

    lua_pushvalue(L, -1);
    lua_pushliteral(L, "foo");
    lua_pushinteger(L, 42);
    apollo::pcall(L, 2, 0);
    BOOST_CHECK_EQUAL(g_n_si_calls, 1u);

    lua_pushinteger(L, 42);
    lua_pushliteral(L, "foo");
    apollo::pcall(L, 2, 0);
    BOOST_CHECK_EQUAL(g_n_is_calls, 1u);
}

BOOST_AUTO_TEST_CASE(ambigous_overload)
{
    reset_calls();
    apollo::push(L, apollo::make_overloadset(&proc1, &proc1, &proc1s));
    lua_pushinteger(L, 42);
    check_ambiguous(L, 1);
}

#include "test_suffix.hpp"
