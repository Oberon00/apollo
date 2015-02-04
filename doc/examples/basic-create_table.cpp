#include <apollo/closing_state.hpp>
#include <apollo/builtin_types.hpp>
#include <apollo/function.hpp>
#include <apollo/create_table.hpp>
#include <lua.hpp>

namespace {

bool is_odd(int n)
{
    return n % 2 != 0;
}

double sqr(double n)
{
    return n * n;
}

double half(double n)
{
    return n / 2;
}

} // anonymous namespace

int main()
{
    apollo::closing_lstate L;
    luaL_openlibs(L);

    lua_pushglobaltable(L);
    apollo::rawset_table(L, -1)
        ("global_constant", 42)
        ("is_odd", &is_odd)
        .subtable("mymodule")
            ("module_version", "0.1.0-dev")
            ("sqr", &sqr)
            ("half", &half)
        .end_subtable();
    lua_pop(L, 1);

    luaL_dostring(L,
        "print(global_constant, is_odd)\n"
        "print(mymodule, mymodule.module_version)\n"
        "print(is_odd(3), is_odd(5))\n"
        "print(mymodule.half(61), mymodule.sqr(8))\n");
}
