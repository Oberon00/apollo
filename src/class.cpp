#include <apollo/class.hpp>
#include <apollo/gc.hpp>

static apollo::detail::light_key object_tag = {};

APOLLO_API void apollo::detail::push_instance_metatable(
    lua_State* L,
    class_info const& cls) BOOST_NOEXCEPT
{
    lua_rawgetp(L, LUA_REGISTRYINDEX, &cls);
    if (!lua_istable(L, -1)) {
        BOOST_ASSERT(lua_isnil(L, -1));
        lua_pop(L, 1);

        lua_createtable(L, 1, 1);

        lua_pushlightuserdata(L, object_tag);
        lua_rawseti(L, -2, 1);

        lua_pushliteral(L, "__gc");
        // Destroy through pointer to interface class.
        lua_pushcfunction(L, &gc_object_with_mt<instance_holder>);
        lua_rawset(L, -3);

        // Copy metatable because we also want to return it.
        lua_pushvalue(L, -1);
        lua_rawsetp(L, LUA_REGISTRYINDEX, &cls);
    }
#ifndef NDEBUG
    lua_rawgeti(L, -1, 1);
    BOOST_ASSERT(lua_touserdata(L, -1) == object_tag);
    lua_pop(L, 1);
#endif
}

APOLLO_API bool apollo::detail::is_apollo_instance(lua_State* L, int idx)
{
    if (!lua_getmetatable(L, idx))
        return false;

    lua_rawgeti(L, -1, 1);
    bool r = lua_touserdata(L, -1) == object_tag;
    lua_pop(L, 2);
    return r;
}
