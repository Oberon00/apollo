#include <apollo/function.hpp>
#include <apollo/detail/light_key.hpp>

static apollo::detail::light_key function_tag = {};
static apollo::detail::light_key light_function_tag = {};

std::pair<bool, std::type_info const*> apollo::detail::function_type(
    lua_State* L, int idx)
{
    if (!lua_getupvalue(L, idx, fn_upval_tag))
        return {false, &typeid(void)};
    void* tag = lua_touserdata(L, -1);
    lua_pop(L, 1);
    bool const is_light = tag == light_function_tag;
    if (!is_light && tag != function_tag)
        return {false, &typeid(void)};
    BOOST_VERIFY(lua_getupvalue(L, idx, fn_upval_type));
    auto pr = static_cast<std::type_info const*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return {is_light, pr};
}

bool apollo::detail::is_light_function(lua_State* L, int idx)
{
    if (!lua_getupvalue(L, idx, fn_upval_tag))
        return false;
    void* tag = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return tag == light_function_tag;
}

void apollo::detail::push_function_tag(lua_State* L, bool is_light)
{
    lua_pushlightuserdata(L, is_light ? light_function_tag : function_tag);
}
