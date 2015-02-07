#include <ctime>
#include <iostream>

#include <apollo/to_raw_function.hpp>
#include <apollo/function.hpp>
#include <apollo/builtin_types.hpp>
#include <apollo/emplace_ctor.hpp>
#include <apollo/create_table.hpp>

struct A {};

// luabind function
static float f1(int, float, const char*, A*)
{
    return 3.14f;
}

// empty function
static int f2(lua_State*)
{
    return 0;
}

inline double clocks_to_seconds(std::clock_t c)
{
    return static_cast<double>(c) / CLOCKS_PER_SEC;
}


int main()
{
    const int num_calls = 100000;
    const int loops = 10;


    lua_State* L = luaL_newstate();

    apollo::register_class<A>(L);
    lua_pushglobaltable(L);
    apollo::rawset_table(L, -1)
        ("test1", APOLLO_TO_RAW_FUNCTION(&f1))
        ("test2", apollo::raw_function(&f2))
        ("test3", &f1)
        ("A", apollo::get_raw_emplace_ctor_wrapper<A>());
    lua_pop(L, 1);

    std::clock_t total1 = 0;
    std::clock_t total2 = 0;
    std::clock_t total3 = 0;

    for (int i = 0; i < loops; ++i)
    {
        // benchmark apollo with to_raw_function
        std::clock_t start1 = std::clock();
        luaL_dostring(L, "a = A()\n"
                         "  for i = 1, 100000 do\n"
                         "  test1(5, 4.6, 'foo', a)\n"
                         "end");

        std::clock_t end1 = std::clock();


        // benchmark empty binding
        std::clock_t start2 = std::clock();
        luaL_dostring(L, "a = A()\n"
                         "for i = 1, 100000 do\n"
                         "  test2(5, 4.6, 'foo', a)\n"
                         "end");

        std::clock_t end2 = std::clock();

        // benchmark apollo without to_raw_function for test3 (but w/ for A()!)
        std::clock_t start3 = std::clock();
        luaL_dostring(L, "a = A()\n"
                         "  for i = 1, 100000 do\n"
                         "  test3(5, 4.6, 'foo', a)\n"
                         "end");

        std::clock_t end3 = std::clock();
        total1 += end1 - start1;
        total2 += end2 - start2;
        total3 += end3 - start3;
    }


    double time1 = clocks_to_seconds(total1);
    double time2 = clocks_to_seconds(total2);
    double time3 = clocks_to_seconds(total3);

    std::cout
        << "apollo : " << time1 * 1000000 / num_calls / loops << " microseconds per call\n"
        << "empty  : " << time2 * 1000000 / num_calls / loops << " microseconds per call\n"
        << "apollo w/o raw: " << time3 * 1000000 / num_calls / loops << " microseconds per call\n";

    lua_close(L);
}
