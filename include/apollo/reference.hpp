#ifndef APOLLO_REFERENCE_HPP_INCLUDED
#define APOLLO_REFERENCE_HPP_INCLUDED APOLLO_REFERENCE_HPP_INCLUDED

#include <apollo/config.hpp>
#include <apollo/converters_fwd.hpp>

#include <boost/assert.hpp>
#include <apollo/lua_include.hpp>

namespace apollo {

enum class ref_mode { move, copy };

class APOLLO_API registry_reference {
public:
    registry_reference();
    explicit registry_reference(
        lua_State* L_, int idx = -1, ref_mode mode = ref_mode::move);

    ~registry_reference();
    registry_reference(registry_reference const& rhs);
    registry_reference& operator=(registry_reference const& rhs);

    registry_reference(registry_reference&& rhs);
    registry_reference& operator=(registry_reference&& rhs);


    bool empty() const;
    void reset(int idx, ref_mode mode = ref_mode::move);
    void reset(
        lua_State* L_ = nullptr, int idx = 0,
        ref_mode mode = ref_mode::move);
    void push() const;

    lua_State* L() const { return m_L; }
    int get() const { return m_ref; }

private:
    lua_State* m_L;
    int m_ref;
};

template<>
struct converter<registry_reference>: converter_base<registry_reference> {

    static void push(lua_State* L, registry_reference const& r)
    {
        if (r.empty()) {
            lua_pushnil(L);
        } else {
            BOOST_ASSERT(r.L() == L);
            r.push();
        }
    }

    static unsigned n_conversion_steps(lua_State*, int)
    {
        return no_conversion - 1;
    }

    static registry_reference from_stack(lua_State* L, int idx)
    {
        return registry_reference(L, idx, ref_mode::copy);
    }
};

class stack_reference {
public:
    explicit stack_reference(lua_State* L = nullptr, int idx = 0)
    {
        reset(L, idx);
    }

    void reset(lua_State* L = nullptr, int idx = 0)
    {
        m_idx = idx < 0 ? lua_absindex(L, idx) : idx;
    }

    bool empty() const { return m_idx == 0; }
    bool valid(lua_State* L) const {
        BOOST_ASSERT(m_idx >= 0 || m_idx <= LUA_REGISTRYINDEX);
        return m_idx < 0 || (m_idx > 0 && m_idx <= lua_gettop(L));
    }
    int get() const { return m_idx; }

private:
    int m_idx;
};

template<>
struct converter<stack_reference>: converter_base<stack_reference> {

    static void push(lua_State* L, stack_reference const& r)
    {
        if (r.empty()) {
            lua_pushnil(L);
        } else {
            BOOST_ASSERT(r.valid(L));
            lua_pushvalue(L, r.get());
        }
    }

    static unsigned n_conversion_steps(lua_State*, int)
    {
        return no_conversion - 1;
    }

    static stack_reference from_stack(lua_State* L, int idx)
    {
        return stack_reference(L, idx);
    }
};

} // namespace apollo

#endif // APOLLO_REFERENCE_HPP_INCLUDED
