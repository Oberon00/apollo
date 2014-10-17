#ifndef APOLLO_REFERENCE_HPP_INCLUDED
#define APOLLO_REFERENCE_HPP_INCLUDED APOLLO_REFERENCE_HPP_INCLUDED

#include <lua.hpp>
#include <boost/assert.hpp>

namespace apollo {

enum class ref_mode { move, copy };

class registry_reference {
public:
    registry_reference();
    explicit registry_reference(
        lua_State* L, int idx = -1, ref_mode mode = ref_mode::move);

    ~registry_reference();
    registry_reference(registry_reference const& rhs);
    registry_reference& operator=(registry_reference const& rhs);

    registry_reference(registry_reference&& rhs);
    registry_reference& operator=(registry_reference&& rhs);


    bool empty() const;
    void reset(int idx, ref_mode mode = ref_mode::move);
    void reset(
        lua_State* L = nullptr, int idx = 0,
        ref_mode mode = ref_mode::move);
    void push() const;

    lua_State* L() const { return m_L; }
    int get() const { return m_ref; }

private:
    lua_State* m_L;
    int m_ref;
};

class stack_reference {
public:
    stack_reference(lua_State* L = nullptr, int idx = 0) { reset(L, idx); }

    void reset(lua_State* L = nullptr, int idx = 0)
    {
        m_idx = idx < 0 ? lua_absindex(L, idx) : idx;
    }

    bool empty() const { return m_idx == 0; }
    bool valid(lua_State* L) const {
        BOOST_ASSERT(m_idx >= 0 || m_idx <= LUA_REGISTRYINDEX);
        return m_idx < 0 || m_idx > 0 && m_idx <= lua_gettop(L);
    }
    int get() const { return m_idx; }

private:
    int m_idx;
};

} // namespace apollo

#endif // APOLLO_REFERENCE_HPP_INCLUDED
