#include <apollo/reference.hpp>

#include "test_prefix.hpp"

static void checkemptyref(apollo::registry_reference const& r)
{
    BOOST_CHECK(r.empty());
    BOOST_CHECK(!r.L());
    BOOST_CHECK_EQUAL(r.get(), LUA_NOREF);
}

BOOST_AUTO_TEST_CASE(emtpy_registry_reference)
{
    apollo::registry_reference r;
    checkemptyref(r);
    r.reset();
    checkemptyref(r);
    //r.reset(0); // This is not required to be defined.
    r = r;
    checkemptyref(r);
    //r = std::move(r); // This is not required to be defined.
    apollo::registry_reference assigned;
    assigned = r;
    checkemptyref(assigned);
    apollo::registry_reference copied(r);
    checkemptyref(copied);
    apollo::registry_reference move_assigned;
    move_assigned = std::move(r);
    checkemptyref(move_assigned);
    apollo::registry_reference move_constructed(std::move(r));
    checkemptyref(move_constructed);
}

static void checkintref(
    lua_State* L, apollo::registry_reference const& r, int v)
{
    BOOST_CHECK(!r.empty());
    BOOST_CHECK_NE(r.get(), LUA_NOREF);
    BOOST_CHECK_NE(r.get(), LUA_REFNIL);
    BOOST_CHECK_EQUAL(r.L(), L);
    r.push();
    BOOST_CHECK_EQUAL(lua_tointeger(L, -1), v);
    lua_pop(L, 1);
}

BOOST_AUTO_TEST_CASE(nonempty_registry_reference)
{
    lua_pushinteger(L, 42);
    apollo::registry_reference r(L);
    BOOST_CHECK_EQUAL(lua_gettop(L), 0);
    BOOST_CHECK_EQUAL(r.L(), L);
    checkintref(L, r, 42);

    r = r;
    checkintref(L, r, 42);

    apollo::registry_reference assigned;
    assigned = r;
    checkintref(L, assigned, 42);
    apollo::registry_reference copied(r);
    checkintref(L, copied, 42);
    apollo::registry_reference move_assigned;
    move_assigned = std::move(copied);
    checkemptyref(copied);
    checkintref(L, move_assigned, 42);
    apollo::registry_reference move_constructed(std::move(r));
    checkemptyref(r);
    checkintref(L, move_constructed, 42);
}

BOOST_AUTO_TEST_CASE(reset_registry_reference)
{
    lua_pushinteger(L, 42);
    apollo::registry_reference r(L);
    auto r2 = r;
    checkintref(L, r2, 42);
    r2.reset();
    checkemptyref(r2);
    r.push();
    r2.reset(L, -1);
    BOOST_CHECK_EQUAL(lua_gettop(L), 0);
    checkintref(L, r2, 42);
    lua_pushinteger(L, 7);
    r2.reset(-1);
    BOOST_CHECK_EQUAL(lua_gettop(L), 0);
    checkintref(L, r2, 7);

    lua_pushinteger(L, 42);
    lua_pushinteger(L, 7);
    apollo::registry_reference r3(L, -2);
    lua_pop(L, 1);
    auto r4 = r3;
    checkintref(L, r4, 42);
    r4.reset();
    checkemptyref(r4);
    r3.push();
    lua_pushinteger(L, 7);
    r4.reset(L, -2);
    lua_pop(L, 1);
    BOOST_CHECK_EQUAL(lua_gettop(L), 0);
    checkintref(L, r4, 42);
    lua_pushinteger(L, 7);
    lua_pushinteger(L, 42);
    r4.reset(-2);
    lua_pop(L, 1);
    BOOST_CHECK_EQUAL(lua_gettop(L), 0);
    checkintref(L, r4, 7);

    r2.reset(0); // "Half-empty"
    BOOST_CHECK_EQUAL(r2.L(), L);
    BOOST_CHECK(r2.empty());
    BOOST_CHECK_EQUAL(r2.get(), LUA_NOREF);
}

BOOST_AUTO_TEST_CASE(registry_reference_copy)
{
    lua_pushinteger(L, 42);
    apollo::registry_reference r(
        L, -1, apollo::ref_mode::copy);
    BOOST_REQUIRE_EQUAL(lua_gettop(L), 1);
    checkintref(L, r, 42);
    lua_pushinteger(L, 7);
    r.reset(-1, apollo::ref_mode::copy);
    BOOST_REQUIRE_EQUAL(lua_gettop(L), 2);
    checkintref(L, r, 7);
    r.reset(-2, apollo::ref_mode::copy);
    BOOST_REQUIRE_EQUAL(lua_gettop(L), 2);
    checkintref(L, r, 42);
    r.reset();
    checkemptyref(r);
    r.reset(L, -2, apollo::ref_mode::copy);
    checkintref(L, r, 42);
    lua_pop(L, 2);
}

#include "test_suffix.hpp"
