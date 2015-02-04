#ifndef APOLLO_TYPEID_HPP_INCLUDED
#define APOLLO_TYPEID_HPP_INCLUDED TYPEID_HPP_INCLUDED

#include <apollo/config.hpp>

#include <apollo/lua_include.hpp>

#include <boost/type_index.hpp>

namespace apollo {

APOLLO_API boost::typeindex::type_info const& lbuiltin_typeid(int id);

APOLLO_API boost::typeindex::type_info const& ltypeid(lua_State* L, int idx);

} // namespace apollo


#endif // APOLLO_TYPEID_HPP_INCLUDED
