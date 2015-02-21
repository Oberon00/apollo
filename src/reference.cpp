// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#include <apollo/reference.hpp>

#include <boost/assert.hpp>

namespace apollo {


registry_reference::registry_reference()
    : m_L(nullptr)
    , m_ref(LUA_NOREF)
{
}

registry_reference::registry_reference(lua_State* L_, int idx, ref_mode mode)
    : m_L(nullptr)
    , m_ref(LUA_NOREF)
{
    reset(L_, idx, mode);
}


registry_reference::~registry_reference()
{
    if (m_L)
        luaL_unref(m_L, LUA_REGISTRYINDEX, m_ref);
}

registry_reference::registry_reference(registry_reference const& rhs):
    m_L(rhs.m_L),
    m_ref(LUA_NOREF)
{
    if (!rhs.empty()) {
        rhs.push();
        reset(-1);
    }
}

registry_reference& registry_reference::operator=(registry_reference const& rhs)
{
    if (rhs.empty()) {
        reset();
        return *this;
    }
    rhs.push();
    reset(rhs.m_L, -1);
    return *this;
}

registry_reference::registry_reference(registry_reference&& rhs)
    : m_L(rhs.m_L)
    , m_ref(rhs.m_ref)
{
    rhs.m_L = nullptr;
    rhs.m_ref = LUA_NOREF;
}

// Note: Not self-assignment safe
registry_reference& registry_reference::operator=(registry_reference&& rhs)
{
    BOOST_ASSERT(this != &rhs);
    m_L = rhs.m_L;
    m_ref = rhs.m_ref;
    rhs.m_L = nullptr;
    rhs.m_ref = LUA_NOREF;
    return *this;
}



bool registry_reference::empty() const
{
    bool isEmpty = m_ref == LUA_NOREF;
    BOOST_ASSERT(isEmpty || m_L);
    return isEmpty;
}

void registry_reference::reset(int idx, ref_mode mode)
{
    luaL_unref(m_L, LUA_REGISTRYINDEX, m_ref);

    if (idx == 0) {
        m_ref = LUA_NOREF;
    } else {
        if (idx == -1 && mode == ref_mode::move) {
            m_ref = luaL_ref(m_L, LUA_REGISTRYINDEX);
        } else {
            lua_pushvalue(m_L, idx);
            m_ref = luaL_ref(m_L, LUA_REGISTRYINDEX);
            if (mode == ref_mode::move)
                lua_remove(m_L, idx);
        }
    }
}

void registry_reference::reset(lua_State* L_, int idx, ref_mode mode)
{
    if (m_L)
        luaL_unref(m_L, LUA_REGISTRYINDEX, m_ref);
    if (L_) {
        m_L = L_;
        reset(idx, mode);
    } else {
        BOOST_ASSERT(idx == 0);
        m_L = nullptr;
        m_ref = LUA_NOREF;
    }
}

void registry_reference::push() const
{
    BOOST_ASSERT(!empty());
    lua_rawgeti(m_L, LUA_REGISTRYINDEX, m_ref);
}


} // namespace apollo
