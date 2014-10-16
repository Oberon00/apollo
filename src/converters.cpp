#include <apollo/converters.hpp>

#include <cmath>

unsigned apollo::detail::string_conversion_steps<char>::get(
    lua_State* L, int idx)
{
    switch(lua_type(L, idx)) {
    case LUA_TSTRING:
        return lua_rawlen(L, idx) == 1 ? 0 : no_conversion;
    case LUA_TNUMBER: {
        lua_Number n = lua_tonumber(L, idx);
        return n >= 0 && n < 10 && n == std::floor(n) ?
                1 : no_conversion;
    }
    default:
        return no_conversion;
    }
    BOOST_UNREACHABLE_RETURN(no_conversion);
}
