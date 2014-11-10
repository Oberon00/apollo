#include <apollo/builtin_types.hpp>
#include <apollo/error.hpp>
#include <apollo/lapi.hpp>
#include <apollo/detail/light_key.hpp>

#include <boost/exception/info.hpp>
#include <boost/throw_exception.hpp>


namespace apollo {

static apollo::detail::light_key const msghKey = {};

void set_error_msg_handler(lua_State* L)
{
    lua_rawsetp(L, LUA_REGISTRYINDEX, &msghKey);
}

bool push_error_msg_handler(lua_State* L)
{
    lua_rawgetp(L, LUA_REGISTRYINDEX, &msghKey);
    if lua_isnil(L, -1) {
        lua_pop(L, 1);
        return false;
    }
    return true;
}

void pcall(lua_State* L, int nargs, int nresults, int msgh)
{
    int const r = lua_pcall(L, nargs, nresults, msgh);
    if (r != LUA_OK) {
        auto lua_msg = from_stack(L, -1, std::string("(no error message)"));
        lua_pop(L, 1);
        BOOST_THROW_EXCEPTION(lua_api_error()
                              << errinfo::lua_state(L)
                              << errinfo::lua_msg(lua_msg)
                              << errinfo::lua_error_code(r)
                              << errinfo::msg("lua_pcall() failed"));
    }
}

void pcall(lua_State* L, int nargs, int nresults)
{
    int const msgh = push_error_msg_handler(L) ? lua_absindex(L, -nargs - 2) : 0;
    if (msgh)
        lua_insert(L, msgh); // move beneath arguments and function

    auto cleanup = [L, msgh]() -> void {
        if (msgh)
            lua_remove(L, msgh);
    };
        
    try { pcall(L, nargs, nresults, msgh); }
    catch (...) {
        cleanup();
        throw;
    }
    cleanup();
}

} // namespace apollo
