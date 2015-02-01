#include <apollo/class.hpp>
#include <apollo/typeid.hpp>

#include "test_prefix.hpp"

#define CHECK_TID_EQ(L, R) BOOST_CHECK_EQUAL(L.name(), R.name())

namespace {
struct test_cls {};
} // anonymous namespace

BOOST_AUTO_TEST_CASE(builtin_typeid)
{
    // Use a macro to see line numbers
#define CHECK_BUILTIN_T(L, R) \
    CHECK_TID_EQ(apollo::lbuiltin_typeid(L), boost::typeindex::type_id<R>())

    CHECK_BUILTIN_T(LUA_TNUMBER, lua_Number);
    CHECK_BUILTIN_T(LUA_TBOOLEAN, bool);
    CHECK_BUILTIN_T(LUA_TFUNCTION, lua_CFunction);
    CHECK_BUILTIN_T(LUA_TSTRING, char const*);
    CHECK_BUILTIN_T(LUA_TTHREAD, lua_State*);
    CHECK_BUILTIN_T(LUA_TNIL, void);
    // All others are unspecified.
#undef CHECK_BUILTIN_T
}

static int testf(lua_State*)
{
    return 0;
}

BOOST_AUTO_TEST_CASE(ltypeid)
{
    using boost::typeindex::type_id;

    apollo::register_class<test_cls>(L);

    lua_pushinteger(L, 42);
    CHECK_TID_EQ(apollo::ltypeid(L, -1), type_id<lua_Number>());
    lua_pop(L, 1);

    lua_pushboolean(L, true);
    CHECK_TID_EQ(apollo::ltypeid(L, -1), type_id<bool>());
    lua_pop(L, 1);

    lua_pushcfunction(L, &testf);
    CHECK_TID_EQ(apollo::ltypeid(L, -1), type_id<lua_CFunction>());
    lua_pop(L, 1);

    apollo::push(L, test_cls());
    CHECK_TID_EQ(apollo::ltypeid(L, -1), type_id<test_cls>());
    lua_pop(L, 1);

    lua_newuserdata(L, sizeof(char));
    CHECK_TID_EQ(
        apollo::ltypeid(L, -1), apollo::lbuiltin_typeid(LUA_TUSERDATA));
    lua_pop(L, 1);

    CHECK_TID_EQ(apollo::ltypeid(L, 1), apollo::lbuiltin_typeid(LUA_TNONE));
}

#include "test_suffix.hpp"
