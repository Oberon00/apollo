// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifndef APOLLO_CREATE_TABLE_HPP_INCLUDED
#define APOLLO_CREATE_TABLE_HPP_INCLUDED APOLLO_CREATE_TABLE_HPP_INCLUDED

#include <apollo/converters.hpp>

#include <apollo/lua_include.hpp>
#include <boost/assert.hpp>

#include <stack>

namespace apollo {

namespace detail {

template <typename Derived>
class basic_table_setter {
private:
    Derived&& move_this()
    {
        return std::move(*static_cast<Derived*>(this));
    }

public:
    basic_table_setter(lua_State* L, int table_idx)
        : m_L(L)
    {
        m_table_idx.push(table_idx);
    }

    ~basic_table_setter()
    {
        BOOST_ASSERT_MSG(
            m_table_idx.size() <= 1,
            "You forgot a end_subtable()");
    }

    basic_table_setter(basic_table_setter const&) = default; // Use with care!
    basic_table_setter& operator= (basic_table_setter const&) = delete;

    template <typename K, typename V>
    Derived&& operator() (K&& key, V&& value)
    {
        apollo::push(m_L, std::forward<K>(key), std::forward<V>(value));
        lua_rawset(m_L, m_table_idx.top());
        return move_this();
    }

    template <typename V>
    Derived&& operator() (int key, V&& value)
    {
        apollo::push(m_L, std::forward<V>(value));
        lua_rawseti(m_L, m_table_idx.top(), key);
        return move_this();
    }

    template <typename K>
    Derived&& subtable(K&& key)
    {
        lua_newtable(m_L);
        top_subtable(std::forward<K>(key));
        return move_this();
    }

    Derived&& end_subtable()
    {
        BOOST_ASSERT_MSG(m_table_idx.size() >= 2,
            "Too many end_subtable() calls."); // Last one is no subtable.
        pop_table();
        return move_this();
    }

    template <typename K>
    Derived&& thistable_as(K&& key)
    {
        apollo::push(m_L, key);
        lua_pushvalue(m_L, m_table_idx.top());
        lua_rawset(m_L, m_table_idx.top());
        return move_this();
    }

    Derived&& thistable_index()
    {
        return thistable_as("__index");
    }

    Derived&& metatable()
    {
        if (!lua_getmetatable(m_L, m_table_idx.top())) {
            lua_newtable(m_L);
            lua_pushvalue(m_L, -1);
            lua_setmetatable(m_L, m_table_idx.top());
        }
        m_table_idx.push(lua_gettop(m_L));
        return move_this();
    }

    Derived&& end_metatable()
    {
        return end_subtable();
    }

protected:
    void pop_table()
    {
        BOOST_ASSERT_MSG(
            !m_table_idx.empty(),
            "Too many end_subtable() calls.");
        BOOST_ASSERT(m_table_idx.top() == lua_gettop(m_L));
        BOOST_ASSERT(lua_istable(m_L, -1));
        lua_pop(m_L, 1);
        m_table_idx.pop();
    }

    template <typename K>
    void top_subtable(K&& key)
    {
        apollo::push(m_L, std::forward<K>(key));
        lua_pushvalue(m_L, -2);
        lua_rawset(m_L, m_table_idx.top());
        m_table_idx.push(lua_gettop(m_L));
    }

    lua_State* const m_L;
    std::stack<int> m_table_idx;
};

struct table_setter: basic_table_setter<table_setter> {
    table_setter(lua_State* L, int table_idx)
        : basic_table_setter<table_setter>(L, table_idx)
    {}
};

} // namespace detail


template <>
struct converter<detail::table_setter>
{
    template <typename T>
    static int push(lua_State*, T) {
        static_assert(!std::is_same<T, T>::value, // Make dependent.
            "Use subtable() instead of nested"
            " calls to newtable/rawset_table/etc.");
        return {};
    }

    template <typename T>
    static bool is_convertible(lua_State*, T) {
        static_assert(!std::is_same<T, T>::value, // Make dependent.
            "Use subtable() instead of nested"
            " calls to newtable/rawset_table/etc.");
        return {}; // Silence gcc's -Wreturn-type
    }

    template <typename T>
    static detail::table_setter to(lua_State*, T) {
        static_assert(!std::is_same<T, T>::value, // Make dependent.
            "Use subtable() instead of nested"
            " calls to newtable/rawset_table/etc.");
        return {nullptr, 0}; // Silence gcc's -Wreturn-type
    }
};


inline detail::table_setter rawset_table(lua_State* L, int table_idx)
{
    return {L, lua_absindex(L, table_idx)};
}

inline detail::table_setter new_table(lua_State* L)
{
    lua_newtable(L);
    return {L, lua_gettop(L)};
}

} // namespace apollo

#endif // APOLLO_CREATE_TABLE_HPP_INCLUDED
