#include <apollo/function.hpp>
#include <apollo/detail/light_key.hpp>

static apollo::detail::light_key function_tag = {};

std::type_info const& apollo::detail::function_type(lua_State* L, int idx)
{
    if (!lua_getupvalue(L, idx, 2))
        return typeid(void);
    void* tag = lua_touserdata(L, -1);
    lua_pop(L, 1);
    if (tag != function_tag)
        return typeid(void);
    BOOST_VERIFY(lua_getupvalue(L, idx, 3));
    auto pr = static_cast<std::type_info const*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return *pr;
}

void apollo::detail::push_function_tag(lua_State* L)
{
    lua_pushlightuserdata(L, function_tag);
}
