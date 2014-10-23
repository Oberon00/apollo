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
