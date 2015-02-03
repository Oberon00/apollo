#ifndef APOLLO_CLOSING_STATE_HPP_INCLUDED
#define APOLLO_CLOSING_STATE_HPP_INCLUDED

#include <lua.hpp>

namespace apollo {

class closing_lstate {
public:
    explicit closing_lstate(lua_State* L): m_L(L) {}

    closing_lstate(): m_L(luaL_newstate()) {}

    closing_lstate(closing_lstate&& other)
        : m_L(other.m_L)
    {
        other.m_L = nullptr;
    }

    closing_lstate& operator= (closing_lstate&& other)
    {
        m_L = other.m_L;
        other.m_L = nullptr;
    }

    ~closing_lstate()
    {
        if (m_L)
            lua_close(m_L);
    }

    operator lua_State* ()
    {
        return m_L;
    }

    lua_State* get()
    {
        return m_L;
    }

private:
    lua_State* m_L;
};

} // namespace apollo

#endif // APOLLO_CLOSING_STATE_HPP_INCLUDED
