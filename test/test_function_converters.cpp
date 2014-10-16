#include <boost/function.hpp>
#include <boost/test/test_tools.hpp>
#include <apollo/function.hpp>
#include <string>

static unsigned g_n_calls = 0;

static void proc0()
{
    ++g_n_calls;
}

static void proc1(int a)
{
    BOOST_CHECK_EQUAL(a, 42);
    ++g_n_calls;
}

static void proc4(int a, std::string const& s, double d, bool b)
{
    BOOST_CHECK_EQUAL(a, 42);
    BOOST_CHECK_EQUAL(s, "foo");
    BOOST_CHECK_EQUAL(d, 3.14);
    BOOST_CHECK_EQUAL(b, false);
    ++g_n_calls;
}

static unsigned func0()
{
    return ++g_n_calls;
}

static char const* func1(int a)
{
    ++g_n_calls;
    BOOST_CHECK_EQUAL(a, 42);
    return "foo";
}

static bool func4(int a, std::string const& s, double d, bool b)
{
    ++g_n_calls;
    BOOST_CHECK_EQUAL(a, 42);
    BOOST_CHECK_EQUAL(s, "foo");
    BOOST_CHECK_EQUAL(d, 3.14);
    BOOST_CHECK_EQUAL(b, false);
    return b;
}

namespace {
struct test_struct {
    void memproc0() { proc0(); }
    bool memfunc4(int a, const std::string& s, double d, bool b)
    {
        return func4(a, s, d, b);
    }
};
} // anonymous namespace

namespace apollo {
    template <>
    struct converter<test_struct&> {
        using to_type = test_struct&;
        //static void push(lua_State*) { }
        static unsigned n_conversion_steps(lua_State* L, int idx)
        {
            return lua_isuserdata(L, idx) ? 0 : no_conversion;
        }

        static test_struct& from_stack(lua_State* L, int idx)
        {
            return *static_cast<test_struct*>(lua_touserdata(L, idx));
        }
    };
} // namespace apollo

#include "test_prefix.hpp"

BOOST_AUTO_TEST_CASE(plain_proc)
{
    g_n_calls = 0;
    apollo::push(L, &proc0);
    lua_pushnil(L); // To check that not only idx == -1 works
    lua_pushvalue(L, -2); // Copy, so that one is left after pcall.
    apollo::pcall(L, 0, 0);
    BOOST_REQUIRE_EQUAL(g_n_calls, 1u);
    BOOST_REQUIRE(apollo::is_convertible<decltype(&proc0)>(L, -2));
    auto proc0ptr = apollo::from_stack<decltype(&proc0)>(L, -2);
    lua_pop(L, 2);
    BOOST_CHECK_EQUAL(proc0ptr, &proc0);

    apollo::push(L, &proc1);
    lua_pushvalue(L, -1); // Copy, so that one is left after pcall.
    lua_pushinteger(L, 42);
    apollo::pcall(L, 1, 0);
    BOOST_REQUIRE_EQUAL(g_n_calls, 2u);
    BOOST_REQUIRE(apollo::is_convertible<decltype(&proc1)>(L, -1));
    auto proc1ptr = apollo::from_stack<decltype(&proc1)>(L, -1);
    lua_pop(L, 1);
    BOOST_CHECK_EQUAL(proc1ptr, &proc1);

    apollo::push(L, &proc4);
    lua_pushvalue(L, -1); // Copy, so that one is left after pcall.
    lua_pushinteger(L, 42);
    lua_pushliteral(L, "foo");
    lua_pushnumber(L, 3.14);
    lua_pushboolean(L, false);
    apollo::pcall(L, 4, 0);
    BOOST_REQUIRE_EQUAL(g_n_calls, 3u);
    BOOST_REQUIRE(apollo::is_convertible<decltype(&proc4)>(L, -1));
    auto proc4ptr = apollo::from_stack<decltype(&proc4)>(L, -1);
    lua_pop(L, 1);
    BOOST_CHECK_EQUAL(proc4ptr, &proc4);
}


BOOST_AUTO_TEST_CASE(plain_func)
{
    g_n_calls = 0;
    apollo::push(L, &func0);
    lua_pushnil(L);
    lua_pushvalue(L, -2); // Copy, so that one is left after pcall.
    apollo::pcall(L, 0, 1);
    BOOST_REQUIRE_EQUAL(g_n_calls, 1u);
    BOOST_CHECK_EQUAL(static_cast<unsigned>(lua_tointeger(L, -1)), g_n_calls);
    lua_pop(L, 1);
    BOOST_REQUIRE(apollo::is_convertible<decltype(&func0)>(L, -2));
    auto func0ptr = apollo::from_stack<decltype(&func0)>(L, -2);
    lua_pop(L, 2);
    BOOST_CHECK_EQUAL(func0ptr, &func0);

    apollo::push(L, &func1);
    lua_pushvalue(L, -1); // Copy, so that one is left after pcall.
    lua_pushinteger(L, 42);
    apollo::pcall(L, 1, 1);
    BOOST_REQUIRE_EQUAL(g_n_calls, 2u);
    BOOST_CHECK_EQUAL(apollo::from_stack<std::string>(L, -1), "foo");
    lua_pop(L, 1);
    BOOST_REQUIRE(apollo::is_convertible<decltype(&func1)>(L, -1));
    auto func1ptr = apollo::from_stack<decltype(&func1)>(L, -1);
    lua_pop(L, 1);
    BOOST_CHECK_EQUAL(func1ptr, &func1);

    apollo::push(L, &func4);
    lua_pushvalue(L, -1); // Copy, so that one is left after pcall.
    lua_pushinteger(L, 42);
    lua_pushliteral(L, "foo");
    lua_pushnumber(L, 3.14);
    lua_pushboolean(L, false);
    apollo::pcall(L, 4, 1);
    BOOST_REQUIRE_EQUAL(g_n_calls, 3u);
    BOOST_CHECK_EQUAL(lua_type(L, -1), LUA_TBOOLEAN);
    BOOST_CHECK_EQUAL(lua_toboolean(L, -1) ? true : false, false);
    lua_pop(L, 1);
    BOOST_REQUIRE(apollo::is_convertible<decltype(&func4)>(L, -1));
    auto func4ptr = apollo::from_stack<decltype(&func4)>(L, -1);
    lua_pop(L, 1);
    BOOST_CHECK_EQUAL(func4ptr, &func4);
}

static void check_proc(lua_State* L)
{
    g_n_calls = 0;

    auto proc0obj = apollo::from_stack<std::function<void()>>(L, -1);
    lua_pop(L, 1);
    proc0obj();
    BOOST_CHECK_EQUAL(g_n_calls, 1u);

    apollo::push(L, proc0obj);
    lua_pushnil(L);

    // Check that the std::function was not wrapped in a pcall lambda:
    auto proc0obj_copy = apollo::from_stack<decltype(proc0obj)>(L, -2);
    BOOST_CHECK_EQUAL(
        proc0obj_copy.target_type().name(),
        proc0obj.target_type().name());

    auto proc0objb = apollo::from_stack<boost::function<void()>>(L, -2);
    proc0objb();
    BOOST_CHECK_EQUAL(g_n_calls, 2u);
    lua_pop(L, 1); // Pop nil.
    apollo::pcall(L, 0, 0);
    BOOST_CHECK_EQUAL(g_n_calls, 3u);

    apollo::push(L, proc0objb);

    // Check that the boost::function was not wrapped in a pcall lambda:
    auto proc0objb_copy = apollo::from_stack<decltype(proc0objb)>(L, -1);
    BOOST_CHECK_EQUAL(
        proc0objb_copy.target_type().name(),
        proc0objb.target_type().name());

    proc0obj = apollo::from_stack<std::function<void()>>(L, -1);
    proc0obj();
    BOOST_CHECK_EQUAL(g_n_calls, 4u);
    apollo::pcall(L, 0, 0);
    BOOST_CHECK_EQUAL(g_n_calls, 5u);
}

BOOST_AUTO_TEST_CASE(proc_obj)
{
    apollo::push(L, &proc0);
     BOOST_CHECK_EQUAL(
        *apollo::from_stack<std::function<void()>>(L, -1)
            .target<void(*)()>(),
        &proc0);
    BOOST_CHECK_EQUAL(
        *apollo::from_stack<boost::function<void()>>(L, -1)
            .target<void(*)()>(),
        &proc0);
    check_proc(L);

    struct proc_obj {
        void operator() ()
        {
            proc0();
        }
    };
    std::function<void()> pobj(proc_obj{});
    apollo::push(L, pobj);
    check_proc(L);
}

static void check_func(lua_State* L)
{
    g_n_calls = 0;

    auto func1obj = apollo::from_stack<std::function<char const*(int)>>(L, -1);
    lua_pop(L, 1);
    BOOST_CHECK_EQUAL(func1obj(42), std::string("foo"));
    BOOST_CHECK_EQUAL(g_n_calls, 1u);

    apollo::push(L, func1obj);
    lua_pushnil(L);

    // Check that the std::function was not wrapped in a pcall lambda:
    auto func1obj_copy = apollo::from_stack<decltype(func1obj)>(L, -2);
    BOOST_CHECK_EQUAL(
        func1obj_copy.target_type().name(),
        func1obj.target_type().name());

    auto func1objb = apollo::from_stack<boost::function<char const*(int)>>(L, -2);
    BOOST_CHECK_EQUAL(func1objb(42), std::string("foo"));
    BOOST_CHECK_EQUAL(g_n_calls, 2u);
    lua_pop(L, 1); // Pop nil.
    lua_pushinteger(L, 42);
    apollo::pcall(L, 1, 1);
    BOOST_CHECK_EQUAL(lua_tostring(L, -1), std::string("foo"));
    lua_pop(L, 1);
    BOOST_CHECK_EQUAL(g_n_calls, 3u);

    apollo::push(L, func1objb);

    // Check that the boost::function was not wrapped in a pcall lambda:
    auto func1objb_copy = apollo::from_stack<decltype(func1objb)>(L, -1);
    BOOST_CHECK_EQUAL(
        func1objb_copy.target_type().name(),
        func1objb.target_type().name());

    func1obj = apollo::from_stack<std::function<char const*(int)>>(L, -1);
    BOOST_CHECK_EQUAL(func1obj(42), std::string("foo"));
    BOOST_CHECK_EQUAL(g_n_calls, 4u);
    lua_pushinteger(L, 42);
    apollo::pcall(L, 1, 1);
    BOOST_CHECK_EQUAL(lua_tostring(L, -1), std::string("foo"));
    lua_pop(L, 1);
    BOOST_CHECK_EQUAL(g_n_calls, 5u);
}

BOOST_AUTO_TEST_CASE(func_obj)
{
    apollo::push(L, &func1);
    BOOST_CHECK_EQUAL(
        *apollo::from_stack<std::function<char const*(int)>>(L, -1)
            .target<char const*(*)(int)>(),
        &func1);
    BOOST_CHECK_EQUAL(
        *apollo::from_stack<boost::function<char const*(int)>>(L, -1)
            .target<char const*(*)(int)>(),
        &func1);
    check_func(L);

    struct func_obj {
        char const* operator() (int a)
        {
            return func1(a);
        }
    };
    std::function<char const*(int)> fobj(func_obj{});
    apollo::push(L, fobj);
    check_func(L);
}


BOOST_AUTO_TEST_CASE(mem_func)
{
    g_n_calls = 0;
    apollo::push(L, &test_struct::memproc0);
    BOOST_CHECK_EQUAL(
        apollo::from_stack<decltype(&test_struct::memproc0)>(L, -1),
        &test_struct::memproc0);
    apollo::push_gc_object(L, test_struct());
    apollo::pcall(L, 1, 0);
    BOOST_CHECK_EQUAL(g_n_calls, 1u);

    // TODO: Duplicate from plain_func test case.
    apollo::push(L, &test_struct::memfunc4);
    BOOST_CHECK_EQUAL(
        apollo::from_stack<decltype(&test_struct::memfunc4)>(L, -1),
        &test_struct::memfunc4);
    apollo::push_gc_object(L, test_struct());
    lua_pushinteger(L, 42);
    lua_pushliteral(L, "foo");
    lua_pushnumber(L, 3.14);
    lua_pushboolean(L, false);
    apollo::pcall(L, 5, 1);
    BOOST_REQUIRE_EQUAL(g_n_calls, 2u);
    BOOST_CHECK_EQUAL(lua_type(L, -1), LUA_TBOOLEAN);
    BOOST_CHECK_EQUAL(lua_toboolean(L, -1) ? true : false, false);
    lua_pop(L, 1);
}


#include "test_suffix.hpp"
