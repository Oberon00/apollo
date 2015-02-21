// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#include <apollo/ward_ptr.hpp>
#include <memory>

#include "test_prefix.hpp"

struct warded : apollo::enable_ward_ptr_from_this<warded> {};


BOOST_AUTO_TEST_CASE(ward_ptr_basic)
{
    std::unique_ptr<warded> p(new warded);
    apollo::ward_ptr<warded> wp(p.get());
    BOOST_CHECK_EQUAL(wp.get(), p.get());
    BOOST_CHECK_EQUAL(wp.get(), p->ref().get());
    BOOST_CHECK(wp.valid());
    BOOST_CHECK(!!wp);
    p.reset();
    BOOST_CHECK_EQUAL(wp.get(), static_cast<warded*>(nullptr));
    BOOST_CHECK(!wp.valid());
    BOOST_CHECK(!wp);

    BOOST_CHECK_THROW(*wp, apollo::bad_ward_ptr);
}

// TODO: copy, move constructor, assignment operator; derived classes, ref()

#include "test_suffix.hpp"
