#ifndef APOLLO_CREATE_TABLE_HPP_INCLUDED
#define APOLLO_CREATE_TABLE_HPP_INCLUDED APOLLO_CREATE_TABLE_HPP_INCLUDED

#include <apollo/converters.hpp>

#include <lua.hpp>
#include <boost/assert.hpp>

#include <set>

namespace apollo {

namespace detail {

class table_setter_base {
public:
    table_setter_base(lua_State* L, int table_idx, bool newtable)
        : m_L(L), m_table_idx(table_idx), m_newtable(newtable) {}

    ~table_setter_base()
    {
        for (auto it = m_pending_removals.rbegin();
             it != m_pending_removals.rend();
             ++it
        ) {
            lua_remove(m_L, *it);
        }
    }

    table_setter_base(table_setter_base const&) = default; // Use with care!
    table_setter_base& operator=(table_setter_base const&) = delete; // Silence MSVC.


protected:
    void consume_table(table_setter_base&& value)
    {
        BOOST_ASSERT(value.m_L == m_L);
        if (value.m_table_idx != lua_gettop(m_L)) {
            lua_pushvalue(m_L, value.m_table_idx);
            if (value.m_table_idx > 0 && value.m_newtable)
                m_pending_removals.insert(value.m_table_idx);
        }
        m_pending_removals.insert(
            value.m_pending_removals.begin(),
            value.m_pending_removals.end());
        value.m_pending_removals.clear();
        value.m_table_idx = 0;
    }

    lua_State* const m_L;
    int m_table_idx;
    std::set<int> m_pending_removals;
    bool m_newtable;
};

template <typename Derived>
class basic_table_setter: public table_setter_base {
private:
    Derived&& move_this()
    {
        return std::move(*static_cast<Derived*>(this));
    }

    template <typename T>
    using nosetter = typename std::enable_if<
        !std::is_base_of<table_setter_base, T>::value,
        Derived&&>::type;

public:
    basic_table_setter(lua_State* L, int table_idx, bool newtable)
        : table_setter_base(L, table_idx, newtable)
    {}

    template <typename K, typename V>
    nosetter<V> operator() (K&& key, V&& value)
    {
        apollo::push(m_L, std::forward<K>(key), std::forward<V>(value));
        lua_rawset(m_L, m_table_idx);
        return move_this();
    }

    template <typename V>
    nosetter<V> operator() (int key, V&& value)
    {
        apollo::push(m_L, std::forward<V>(value));
        lua_rawseti(m_L, m_table_idx, key);
        return move_this();
    }

    template <typename K>
    Derived&& operator() (K&& key, table_setter_base&& value)
    {
        apollo::push(m_L, std::forward<K>(key));
        consume_table(std::move(value));
        lua_rawset(m_L, m_table_idx);
        return move_this();
    }

    Derived&& operator() (int key, table_setter_base&& value)
    {
        consume_table(std::move(value));
        lua_rawseti(m_L, m_table_idx, key);
        return move_this();
    }
};

struct table_setter: basic_table_setter<table_setter> {
    table_setter(lua_State* L, int table_idx, bool newtable)
        : basic_table_setter<table_setter>(L, table_idx, newtable)
    {}
};

} // namespace detail


inline detail::table_setter rawset_table(lua_State* L, int table_idx)
{
    return {L, lua_absindex(L, table_idx), false};
}

inline detail::table_setter new_table(lua_State* L)
{
    lua_newtable(L);
    return {L, lua_gettop(L), true};
}

} // namespace apollo

#endif // APOLLO_CREATE_TABLE_HPP_INCLUDED
