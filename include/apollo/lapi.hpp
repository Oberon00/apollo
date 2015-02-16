#ifndef APOLLO_LAPI_HPP_INCLUDED
#define APOLLO_LAPI_HPP_INCLUDED APOLLO_LAPI_HPP_INCLUDED

#include <apollo/lua_include.hpp>

namespace apollo {

APOLLO_API void set_error_msg_handler(lua_State* L);
APOLLO_API bool push_error_msg_handler(lua_State* L);

APOLLO_API void pcall(lua_State* L, int nargs, int nresults, int msgh);
APOLLO_API void pcall(lua_State* L, int nargs, int nresults);

// for k, v in pairs(with) do t[k] = v end, but uses rawset and next.
// Error reporting: lua_error
APOLLO_API void extend_table(lua_State* L, int t, int with);

// max_depth = 0: Do nothing.
// max_depth = 1: Same as extend_table(L, t, with).
// max_depth = n: For each t[k] and with[k] that are both tables, recursively
//   call extend_table_deep(L, t[k], with[k], n - 1).
//   Otherwise set t[k] = with[k].
// Error reporting: lua_error
APOLLO_API void extend_table_deep(lua_State* L,
    int t, int with, unsigned max_depth = 2);

} // namespace apollo

#endif // APOLLO_LAPI_HPP_INCLUDED
