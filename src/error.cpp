#include <apollo/error.hpp>

APOLLO_API int apollo::detail::push_current_exception_string(
    lua_State* L) BOOST_NOEXCEPT
{
    int arg = 0;
    try {
        throw;
    } catch(to_cpp_conversion_error const& e) {
        // TODO: Check for nullptr.
        arg = *boost::get_error_info<errinfo::stack_index>(e);
        lua_pushfstring(L, "%s [%s -> %s]",
            boost::get_error_info<errinfo::msg>(e)->c_str(),
            luaL_typename(L, arg),
            boost::get_error_info<boost::errinfo_type_info_name>(e)->c_str());
    } catch(std::exception const& e) {
        lua_pushfstring(L, "exception: %s", e.what());
    } catch(...) {
        lua_pushliteral(L, "unknown exception");
    }
    return arg;

}

APOLLO_API BOOST_NORETURN void
apollo::detail::error_from_pushed_exception_string(
    lua_State* L, int arg) BOOST_NOEXCEPT
{
    if (arg)
        luaL_argerror(L, arg, lua_tostring(L, -1));
    lua_error(L);
    std::abort(); // Should never be reached.

}
