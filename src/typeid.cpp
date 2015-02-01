#include <apollo/class.hpp>
#include <apollo/typeid.hpp>

#include <boost/assert.hpp>

namespace {
    struct table_tag {};
}

APOLLO_API boost::typeindex::type_info const& apollo::lbuiltin_typeid(int id)
{
    using boost::typeindex::type_id;
    switch (id) {
        case LUA_TBOOLEAN: return type_id<bool>().type_info();
        case LUA_TFUNCTION: return type_id<lua_CFunction>().type_info();
        case LUA_TLIGHTUSERDATA: return type_id<void**>().type_info();
        case LUA_TNIL:
        case LUA_TNONE: return type_id<void>().type_info();
        case LUA_TNUMBER: return type_id<lua_Number>().type_info();
        case LUA_TSTRING: return type_id<char const*>().type_info();
        case LUA_TTABLE: return type_id<table_tag>().type_info();
        case LUA_TTHREAD: return type_id<lua_State*>().type_info();
        case LUA_TUSERDATA: return type_id<void*>().type_info();

        default:
            BOOST_ASSERT_MSG(false, "id is not a valid lua type id");
            return type_id<void>().type_info();
    }
}

APOLLO_API boost::typeindex::type_info const& apollo::ltypeid(
    lua_State* L, int idx)
{
    if (detail::is_apollo_instance(L, idx))
        return *detail::as_holder(L, idx)->type().rtti_type;
    return lbuiltin_typeid(lua_type(L, idx));
}
