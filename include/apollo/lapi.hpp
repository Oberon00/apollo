#ifndef APOLLO_LAPI_HPP_INCLUDED
#define APOLLO_LAPI_HPP_INCLUDED APOLLO_LAPI_HPP_INCLUDED

#include <apollo/lua_include.hpp>

namespace apollo {

APOLLO_API void set_error_msg_handler(lua_State* L);
APOLLO_API bool push_error_msg_handler(lua_State* L);

APOLLO_API void pcall(lua_State* L, int nargs, int nresults, int msgh);
APOLLO_API void pcall(lua_State* L, int nargs, int nresults);


} // namespace apollo

#endif // APOLLO_LAPI_HPP_INCLUDED