#include <iostream>
#include <complex>

#include "converters.hpp"
#include "function.hpp"

using namespace cpplua;

void nullaryProcedure()
{
    std::cout << "nullaryProcedure() called\n";
}

bool nullaryFunction()
{
    std::cout << "nullaryFunction() called\n";
    return true;
}

void unaryProcedure(int s)
{
    std::cout << "got \"" << s << "\"\n";
}

int main()
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    
    push(L, "\0bar");
    auto s2 = toType<std::string>(L, -1);
    assert(s2.size() == 4);
    push(L, &nullaryProcedure);
    lua_setglobal(L, "f");
    luaL_dostring(L, "f()");
    push(L, &nullaryFunction);
    lua_setglobal(L, "g");
    luaL_dostring(L, "print(g())");
    push(L, &unaryProcedure);
    lua_setglobal(L, "myprint");
    if (luaL_dostring(L, "myprint('5')")) {
        std::cerr << "error calling myprint():\n";
        std::cerr << '\t' << lua_tostring(L, -1) << '\n';
        lua_pop(L, 1);
    }
    lua_close(L);
}
