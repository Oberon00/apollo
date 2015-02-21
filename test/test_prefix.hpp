// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifdef APOLLO_TEST_PREFIX_HPP_INCLUDED
#    error Test prefix may only be included once.
#endif
#define APOLLO_TEST_PREFIX_HPP_INCLUDED APOLLO_TEST_PREFIX_HPP_INCLUDED

#include "testutil.hpp"

#include <boost/preprocessor/cat.hpp>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(
    BOOST_PP_CAT(BOOST_TEST_MODULE, _suite),
    lstate_fixture)
