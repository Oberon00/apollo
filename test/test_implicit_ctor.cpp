// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#include <apollo/builtin_types.hpp>
#include <apollo/ctor_wrapper.hpp>
#include <apollo/function.hpp>
#include <apollo/implicit_ctor.hpp>
#include <apollo/lapi.hpp>
#include <apollo/to_raw_function.hpp>

#include <boost/noncopyable.hpp>

#include "test_prefix.hpp"

namespace {

struct bar_cls {};

struct foo_cls {
    static int const got_bar_cls;
    explicit foo_cls(int i_): i(i_) {}
    explicit foo_cls(bar_cls): i(got_bar_cls) {}
    ~foo_cls() { i = static_cast<int>(0xdeadbeef); }
    foo_cls(foo_cls const&) = default;
    foo_cls& operator= (foo_cls const&) = default;

    int i;
};

struct unmoveable: private boost::noncopyable {
    explicit unmoveable(int) {}
    unmoveable(unmoveable&&) = delete;
    unmoveable& operator= (unmoveable&&) = delete;
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
    apollo::register_class<unmoveable>(L);
    apollo::add_implicit_ctor(L, &apollo::new_wrapper<unmoveable, int>);
    apollo::add_implicit_ctor(L, &apollo::ctor_wrapper<foo_cls, int>);
    apollo::add_implicit_ctor(L, &apollo::ctor_wrapper<foo_cls, bar_cls>);

    APOLLO_PUSH_FUNCTION_STATIC(L, &needs_cref);
    lua_pushinteger(L, 42);

    auto& unmoved = apollo::unwrap_ref(
        apollo::to<unmoveable>(L, -1));
    (void)unmoved;

    BOOST_REQUIRE(apollo::is_convertible<foo_cls>(L, -1));
    auto foo = apollo::unwrap_ref(apollo::to<foo_cls>(L, -1));
    BOOST_CHECK_EQUAL(foo.i, 42);
    BOOST_CHECK(!apollo::is_convertible<foo_cls&>(L, -1));
    BOOST_REQUIRE(apollo::is_convertible<foo_cls const&>(L, -1));
    BOOST_CHECK_EQUAL(apollo::to<foo_cls const&>(L, -1).get().i, 42);
    BOOST_CHECK(apollo::to<foo_cls const&>(L, -1).owns_object());
    BOOST_CHECK_EQUAL(APOLLO_TO_ARG(L, -1, foo_cls const&).i, 42);

    apollo::pcall(L, 1, 0);

    apollo::push(L, bar_cls());
    BOOST_REQUIRE(apollo::is_convertible<foo_cls>(L, -1));
    foo = apollo::to<foo_cls>(L, -1).get();
    lua_pop(L, 1);
    BOOST_CHECK_EQUAL(foo.i, foo_cls::got_bar_cls);

    // Test that implicit ctors don't block normal conversion.
    apollo::push(L, foo);
    BOOST_REQUIRE(apollo::is_convertible<foo_cls>(L, -1));
    foo = apollo::to<foo_cls>(L, -1).get();
    lua_pop(L, 1);
    BOOST_CHECK_EQUAL(foo.i, foo_cls::got_bar_cls);
}

#include "test_suffix.hpp"
