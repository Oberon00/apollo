#ifndef APOLLO_CREATE_TABLE_HPP_INCLUDED
#define APOLLO_CREATE_TABLE_HPP_INCLUDED APOLLO_CREATE_TABLE_HPP_INCLUDED

#include <apollo/converters.hpp>

#include <lua.hpp>
#include <boost/assert.hpp>

#include <set>

namespace apollo {

namespace detail {

class table_setter {
public:
    table_setter(lua_State* L, int table_idx, bool newtable)
        : m_L(L), m_table_idx(table_idx), m_newtable(newtable) {}

    ~table_setter()
    {
        for (auto it = m_pending_removals.rbegin();
             it != m_pending_removals.rend();
             ++it
        ) {
            lua_remove(m_L, *it);
        }
    }

    table_setter(table_setter const&) = default; // Use with care!

    template <typename K, typename V>
    table_setter&& operator() (K&& key, V&& value)
    {
        apollo::push(m_L, std::forward<K>(key), std::forward<V>(value));
        lua_rawset(m_L, m_table_idx);
        return std::move(*this);
    }

    template <typename V>
    table_setter&& operator() (int key, V&& value)
    {
        apollo::push(m_L, std::forward<V>(value));
        lua_rawseti(m_L, m_table_idx, key);
        return std::move(*this);
    }

    template <typename K>
    table_setter&& operator() (K&& key, table_setter&& value)
    {
        apollo::push(m_L, std::forward<K>(key));
        consume_table(std::move(value));
        lua_rawset(m_L, m_table_idx);
        return std::move(*this);
    }

    table_setter&& operator() (int key, table_setter&& value)
    {
        consume_table(std::move(value));
        lua_rawseti(m_L, m_table_idx, key);
        return std::move(*this);
    }


private:
    void consume_table(table_setter&& value)
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
