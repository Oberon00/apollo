#ifndef APOLLO_CREATE_TABLE_HPP_INCLUDED
#define APOLLO_CREATE_TABLE_HPP_INCLUDED APOLLO_CREATE_TABLE_HPP_INCLUDED

#include <lua.hpp>

namespace apollo {

namespace detail {

class table_setter {
public:
    table_setter(lua_State* L, int table_idx)
        : m_L(L), m_table_idx(table_idx) {}

    template <typename K, typename V>
    table_setter& operator() (K&& key, V&& value)
    {
        apollo::push(m_L, std::forward<K>(key), std::forward<V>(value));
        lua_rawset(m_L, m_table_idx);
        return *this;
    }

    template <typename V>
    table_setter& operator() (int key, V&& value)
    {
        apollo::push(m_L, std::forward<V>(value));
        lua_rawseti(m_L, m_table_idx, key);
        return *this;
    }

private:
    lua_State* m_L;
    int m_table_idx;
};

} // namespace detail

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