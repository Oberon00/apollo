#include <apollo/builtin_types.hpp>
#include <apollo/closing_lstate.hpp>
#include <apollo/function.hpp>

#include <functional>
#include <cassert>

namespace {

bool is_odd(int n)
{
    return n % 2 != 0;
}

} // anonymous namespace

int main()
{
    apollo::closing_lstate L;
    luaL_openlibs(L);

    apollo::push(L, 42); // Same as lua_pushinteger(L, 42)
    assert(apollo::to<int>(L, -1) == 42);
    lua_pop(L, 1);

    apollo::push(L, &is_odd); // Functions can be pushed just like other types.
    lua_setglobal(L, "is_odd");
    luaL_dostring(L, "print(is_odd(4), is_odd(5))"); // --> false	true

    // Pushed functions can be retrieved back:
    lua_getglobal(L, "is_odd");
    assert(apollo::to<bool(*)(int)>(L, -1) == &is_odd);


    // Lua functions can be retrieved as boost:: or std::function
    luaL_dostring(L, "function is_even(n) return n % 2 == 0 end");
    lua_getglobal(L, "is_even");
    auto is_even = apollo::to<std::function<bool(int)>>(L, -1);
    assert(is_even(4));
    assert(!is_even(5));
}
