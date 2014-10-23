#include <apollo/typeid.hpp>

std::type_info const& apollo::lbuiltin_typeid(int id)
{
    return typeid(void);
}

std::type_info const& apollo::ltypeid(lua_State* L, int idx)
{
    return typeid(void);
}
