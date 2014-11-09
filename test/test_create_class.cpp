#include <apollo/create_class.hpp>

#include "test_prefix.hpp"

namespace {

static unsigned g_n_calls = 0;

class foo_cls {
public:
    void test()
    {
        ++g_n_calls;
    }
};

class bar_cls {
public:
    char const* btest()
    {
        ++g_n_calls;
        return "got it";
    }
};

} // anonymous namespace

BOOST_AUTO_TEST_CASE(simple)
{
    g_n_calls = 0;
    luaL_requiref(L, "base", &luaopen_base, true);
    lua_pop(L, 1);

    lua_pushglobaltable(L);
    apollo::export_classes(L, -1)
        .cls<foo_cls>("foo_cls") // Executed first
            .ctor<>()
            .thistable_index()
            ("test", APOLLO_TO_RAW_FUNCTION(&foo_cls::test))
        .end_cls();
    lua_pop(L, 1);

    require_dostring(L,
        "local foo = foo_cls.new()\n"
        "foo_cls.test(foo)\n"
        "foo:test()\n"
        "assert(foo.nonexistent == nil)\n");
    BOOST_CHECK_EQUAL(g_n_calls, 2u);
}

BOOST_AUTO_TEST_CASE(nested)
{
    g_n_calls = 0;
    luaL_requiref(L, "base", &luaopen_base, true);
    lua_pop(L, 1);

    lua_pushglobaltable(L);
    apollo::export_classes(L, -1)
        .cls<foo_cls>("foo_cls") // Executed first
            .thistable_index()
            ("test", APOLLO_TO_RAW_FUNCTION(&foo_cls::test))
            .cls<bar_cls>("bar_cls")
                .ctor<>()
                .thistable_index()
                ("btest", APOLLO_TO_RAW_FUNCTION(&bar_cls::btest))
            .end_cls()
            .ctor<>()
        .end_cls();
    lua_pop(L, 1);

    require_dostring(L,
        "local foo = foo_cls.new()\n"
        "foo_cls.test(foo)\n"
        "foo:test()\n"
        "assert(foo.nonexistent == nil)\n");
    BOOST_CHECK_EQUAL(g_n_calls, 2u);

    g_n_calls = 0;
    require_dostring(L,
        "local bar = foo_cls.bar_cls.new()\n"
        "assert(foo_cls.bar_cls.btest(bar) == 'got it', 'btest retval')\n"
        "assert(bar:btest() == 'got it', 'btest retval (OO style call)')\n");
    BOOST_CHECK_EQUAL(g_n_calls, 2u);
}

#include "test_suffix.hpp"
