#include <apollo/closing_state.hpp>
#include <apollo/builtin_types.hpp>
#include <apollo/function.hpp>
#include <apollo/class.hpp>
#include <apollo/create_table.hpp>
#include <apollo/property.hpp>
#include <lua.hpp>
#include <iostream>

namespace {

class my_class {
public:
    my_class(int foo_, char bar): foo(foo_), m_bar(bar)
    { }

    int foo; // Just for demonstration.

    void print_bar()
    {
        std::cout << "my_class::m_bar: " << m_bar << '\n';
    }

private:
    char m_bar;
};

} // anonymous namespace

int main()
{
    apollo::closing_lstate L;
    luaL_openlibs(L);

    apollo::register_class<my_class>(L);


    lua_pushglobaltable(L);
    apollo::rawset_table(L, -1)
        ("MyClass", &apollo::ctor_wrapper<my_class, int, char>);
    lua_pop(L, 1);

    apollo::push_class_metatable<my_class>(L);
    apollo::rawset_table(L, -1)
        .thistable_as("__index") // getmetatable(T).__index = getmetatable(T)
        ("foo",       &APOLLO_MEMBER_GETTER(my_class::foo))
        ("set_foo",   &APOLLO_MEMBER_SETTER(my_class::foo))
        ("print_bar", &my_class::print_bar);
    lua_pop(L, 1);

    luaL_dostring(L,
        "local mc = MyClass(42, 'a')\n"
        "mc:print_bar()\n"
        "print(mc:foo())\n"
        "mc:set_foo(7)\n"
        "print(mc:foo())\n");
}
