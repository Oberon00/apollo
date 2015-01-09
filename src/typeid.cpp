#include <apollo/class.hpp>
#include <apollo/typeid.hpp>

#include <boost/assert.hpp>

namespace {
    struct table_tag {};
}

APOLLO_API std::type_info const& apollo::lbuiltin_typeid(int id)
{
    switch (id) {
        case LUA_TBOOLEAN: return typeid(bool);
        case LUA_TFUNCTION: return typeid(lua_CFunction);
        case LUA_TLIGHTUSERDATA: return typeid(void**);
        case LUA_TNIL:
        case LUA_TNONE: return typeid(void);
        case LUA_TNUMBER: return typeid(lua_Number);
        case LUA_TSTRING: return typeid(char const*);
        case LUA_TTABLE: return typeid(table_tag);
        case LUA_TTHREAD: return typeid(lua_State*);
        case LUA_TUSERDATA: return typeid(void*);

        default:
            BOOST_ASSERT_MSG(false, "id is not a valid lua type id");
            return typeid(void);
    }
}

APOLLO_API std::type_info const& apollo::ltypeid(lua_State* L, int idx)
{
    if (detail::is_apollo_instance(L, idx))
        return *detail::as_holder(L, idx)->type().rtti_type;
    return lbuiltin_typeid(lua_type(L, idx));
}
