// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#include <apollo/class.hpp>
#include <apollo/lapi.hpp>
#include <apollo/function.hpp>
#include <apollo/builtin_types.hpp>

#include "test_prefix.hpp"


namespace {

struct foo_cls {
    int i;
};

void inc_foos(foo_cls& foo1, foo_cls& foo2)
{
    ++foo1.i;
    ++foo2.i;
}

} // anonymous namespace

// This tests that even the second argument of a function can correctly be
// passed by reference (even though the current implementation stores the
// arguments temporarily in a std::tuple and uses std::tuple_cat).
BOOST_AUTO_TEST_CASE(call_by_ref)
{
    apollo::register_class<foo_cls>(L);
    foo_cls foo1 {0}, foo2 {2};
    apollo::push(L, &inc_foos, &foo1, &foo2);
    apollo::pcall(L, 2, 0);
    BOOST_CHECK_EQUAL(foo1.i, 1);
    BOOST_CHECK_EQUAL(foo2.i, 3);
}

#include "test_suffix.hpp"
