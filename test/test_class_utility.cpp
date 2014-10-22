#include <apollo/class_utility.hpp>

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

} // anonymous namespace

BOOST_AUTO_TEST_CASE(simple)
{
    g_n_calls = 0;
    luaL_requiref(L, "base", &luaopen_base, true);
    lua_pop(L, 1);

    lua_pushglobaltable(L);
    apollo::rawset_table(L, -1) // Executed second: Fails because -1's meaning changed.
        ("foo_cls", apollo::export_class<foo_cls>(L) // Executed first
            .ctor<>()
            ("test", APOLLO_TO_RAW_FUNCTION(&foo_cls::test)));

    if (luaL_dostring(L,
        "local foo = foo_cls.new()\n"
        "foo_cls.test(foo)")
    ) {
        BOOST_ERROR(lua_tostring(L, -1));
        lua_pop(L, 1);
        BOOST_FAIL("The previous error was fatal.");
    }
    BOOST_CHECK_EQUAL(g_n_calls, 1u);
}

#include "test_suffix.hpp"