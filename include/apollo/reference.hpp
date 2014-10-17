#ifndef APOLLO_REFERENCE_HPP_INCLUDED
#define APOLLO_REFERENCE_HPP_INCLUDED APOLLO_REFERENCE_HPP_INCLUDED

#include <lua.hpp>

namespace apollo {

class registry_reference {
public:
    enum class ref_mode { move, copy };
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

} // namespace apollo

#endif // APOLLO_REFERENCE_HPP_INCLUDED
