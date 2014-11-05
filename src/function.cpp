#include <apollo/function.hpp>
#include <apollo/detail/light_key.hpp>
#include <apollo/stack_balance.hpp>

static apollo::detail::light_key function_tag = {};

std::type_info const& apollo::detail::function_type(
    lua_State* L, int idx)
{
    stack_balance b(L);

    if (!lua_getupvalue(L, idx, fn_upval_tag))
        return typeid(void);
    if (lua_touserdata(L, -1) != function_tag)
        return typeid(void);
    lua_pop(L, 1); // Necessary to keep idx correct:
    BOOST_VERIFY(lua_getupvalue(L, idx, fn_upval_type));
    return *static_cast<std::type_info const*>(lua_touserdata(L, -1));
}

void apollo::detail::push_function_tag(lua_State* L)
{
    lua_pushlightuserdata(L, function_tag);
}
