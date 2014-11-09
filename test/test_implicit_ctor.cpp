#include <apollo/implicit_ctor.hpp>
#include <apollo/lapi.hpp>
#include <apollo/function.hpp>
#include <apollo/raw_function.hpp>

#include "test_prefix.hpp"

namespace {

struct bar_cls {};

struct foo_cls {
    static int const got_bar_cls;
    explicit foo_cls(int i_): i(i_) {}
    explicit foo_cls(bar_cls): i(got_bar_cls) {}

    int i;
};

int const foo_cls::got_bar_cls = INT_MAX;

void needs_cref(foo_cls const& foo)
{
    BOOST_CHECK_EQUAL(foo.i, 42);
}

} // anonymous namespace

BOOST_AUTO_TEST_CASE(implicit_ctor_conversion)
{
    apollo::register_class<foo_cls>(L);
    apollo::register_class<bar_cls>(L);
    apollo::add_implicit_ctor(L, &apollo::ctor_wrapper<foo_cls, int>);
    apollo::add_implicit_ctor(L, &apollo::ctor_wrapper<foo_cls, bar_cls>);

    APOLLO_PUSH_FUNCTION_STATIC(L, &needs_cref);
    lua_pushinteger(L, 42);

    BOOST_REQUIRE(apollo::is_convertible<foo_cls>(L, -1));
    auto foo = apollo::unwrap_bound_ref(apollo::from_stack<foo_cls>(L, -1));
    BOOST_CHECK_EQUAL(foo.i, 42);
    BOOST_CHECK(!apollo::is_convertible<foo_cls&>(L, -1));
    BOOST_REQUIRE(apollo::is_convertible<foo_cls const&>(L, -1));
    BOOST_CHECK_EQUAL(apollo::from_stack<foo_cls const&>(L, -1).get().i, 42);
    BOOST_CHECK(apollo::from_stack<foo_cls const&>(L, -1).owns_object());

    apollo::pcall(L, 1, 0);

    apollo::push(L, bar_cls());
    BOOST_REQUIRE(apollo::is_convertible<foo_cls>(L, -1));
    foo = apollo::from_stack<foo_cls>(L, -1);
    lua_pop(L, 1);
    BOOST_CHECK_EQUAL(foo.i, foo_cls::got_bar_cls);

    // Test that implicit ctors don't block normal conversion.
    apollo::push(L, foo);
    BOOST_REQUIRE(apollo::is_convertible<foo_cls>(L, -1));
    foo = apollo::from_stack<foo_cls>(L, -1);
    lua_pop(L, 1);
    BOOST_CHECK_EQUAL(foo.i, foo_cls::got_bar_cls);
}

#include "test_suffix.hpp"
