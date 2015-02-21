// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#include <apollo/stack_balance.hpp>

#include <boost/assert.hpp>

namespace apollo {

stack_balance::stack_balance(lua_State* L, int diff, int act)
    : m_L((BOOST_ASSERT(L), L))
    , m_desired_top(lua_gettop(L) + diff)
    , m_action(act)
{
    BOOST_ASSERT(m_desired_top >= 0);

    // When adjusting, debug makes no sense because
    // all errors are corrected automatically.
    BOOST_ASSERT((act & adjust) != adjust || !(act & debug));
}

stack_balance::~stack_balance()
{
    int const top = lua_gettop(m_L);
    if (top > m_desired_top) {
        if (m_action & pop)
            lua_settop(m_L, m_desired_top);
        else if (m_action & debug)
            BOOST_ASSERT("unbalanced stack: too many elements" && false);
    } else if (top < m_desired_top) {
        if (m_action & push_nil)
            lua_settop(m_L, m_desired_top);
       else if (m_action & debug)
            BOOST_ASSERT("unbalanced stack: too few elements" && false);
    }
}

} // namespace apollo
