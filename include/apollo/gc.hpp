#ifndef APOLLO_GC_HPP_INCLUDED
#define APOLLO_GC_HPP_INCLUDED APOLLO_GC_HPP_INCLUDED

#include <apollo/detail/meta_util.hpp>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <apollo/lua_include.hpp>

#include <type_traits>

namespace apollo {

template <typename T>
int gc_object(lua_State* L) BOOST_NOEXCEPT
{
    BOOST_ASSERT(lua_type(L, 1) == LUA_TUSERDATA);
    static_cast<T*>(lua_touserdata(L, 1))->~T();
    return 0;
}

template <typename T>
inline typename detail::remove_qualifiers<T>::type*
push_bare_udata(lua_State* L, T&& o)
{
    using obj_t = typename detail::remove_qualifiers<T>::type;
    void* uf = lua_newuserdata(L, sizeof(obj_t));
    try {
        return new(uf) obj_t(std::forward<T>(o));
    } catch (...) {
        lua_pop(L, 1);
        throw;
    }
}

template <typename T, typename... Args>
inline typename detail::remove_qualifiers<T>::type*
emplace_bare_udata(lua_State* L, Args&&... ctor_args)
{
    using obj_t = typename detail::remove_qualifiers<T>::type;
    void* uf = lua_newuserdata(L, sizeof(obj_t));
    try {
        return new(uf) obj_t(std::forward<Args>(ctor_args)...);
    } catch (...) {
        lua_pop(L, 1);
        throw;
    }
}


// Note: __gc will not unset metatable.
// Use for objects that cannot be retrieved from untrusted Lua code only.
template <typename T>
typename detail::remove_qualifiers<T>::type*
push_gc_object(lua_State* L, T&& o)
{
    using obj_t = typename detail::remove_qualifiers<T>::type;
    void* uf = push_bare_udata(L, std::forward<T>(o));
#ifdef BOOST_MSVC
#   pragma warning(push)
#   pragma warning(disable:4127) // Conditional expression is constant.
#endif
    if (!std::is_trivially_destructible<obj_t>::value) {
#ifdef BOOST_MSVC
#   pragma warning(pop)
#endif
        lua_createtable(L, 0, 1); // 0 sequence entries, 1 dictionary entry
        lua_pushcfunction(L, &gc_object<obj_t>);
        lua_setfield(L, -2, "__gc");
        lua_setmetatable(L, -2);
    }
    return static_cast<obj_t*>(uf);
}

} // namespace apollo

#endif // APOLLO_GC_HPP_INCLUDED
