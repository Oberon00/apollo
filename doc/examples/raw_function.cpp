#include <apollo/builtin_types.hpp>
#include <apollo/closing_lstate.hpp>
#include <apollo/to_raw_function.hpp>

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

    auto is_odd_rf = apollo::to_raw_function<decltype(&is_odd), &is_odd>();
    apollo::push(L, is_odd_rf);
    // Or, equivalently, even: lua_pushcfunction(L, is_odd_rf.f);
    lua_setglobal(L, "is_odd");
    luaL_dostring(L, "print(is_odd(4), is_odd(5))"); // --> false	true
}
