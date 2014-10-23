#ifndef APOLLO_TYPEID_HPP_INCLUDED
#define APOLLO_TYPEID_HPP_INCLUDED TYPEID_HPP_INCLUDED

#include <lua.hpp>

#include <typeinfo>

namespace apollo {

std::type_info const& lbuiltin_typeid(int id);

std::type_info const& ltypeid(lua_State* L, int idx);

} // namespace apollo


#endif // APOLLO_TYPEID_HPP_INCLUDED
