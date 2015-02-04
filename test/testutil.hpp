#include <apollo/lua_include.hpp>

struct lstate_fixture {
    lua_State* L;

    lstate_fixture();
    ~lstate_fixture();
};

void check_dostring(lua_State* L, char const* s);
void require_dostring(lua_State* L, char const* s);
