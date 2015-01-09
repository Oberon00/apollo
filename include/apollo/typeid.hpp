#ifndef APOLLO_TYPEID_HPP_INCLUDED
#define APOLLO_TYPEID_HPP_INCLUDED TYPEID_HPP_INCLUDED

#include <apollo/config.hpp>

#include <lua.hpp>

#include <typeinfo>

namespace apollo {

APOLLO_API std::type_info const& lbuiltin_typeid(int id);

APOLLO_API std::type_info const& ltypeid(lua_State* L, int idx);

} // namespace apollo


#endif // APOLLO_TYPEID_HPP_INCLUDED
