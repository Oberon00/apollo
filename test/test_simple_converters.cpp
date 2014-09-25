#include <apollo/converters.hpp>
#include <apollo/stack_balance.hpp>
#include <cstring>

#include "test_prefix.hpp"

template <int LuaT = LUA_TNUMBER, typename T>
static void check_roundtrip(lua_State* L, T&& v)
{
    typedef typename apollo::detail::remove_qualifiers<T>::type T2;
    static_assert(apollo::detail::lua_type_id<T2>::value == LuaT,
                  "T not detected as LuaT");
    apollo::stack_balance balance(L);
    apollo::push(L, std::forward<T>(v));
    BOOST_REQUIRE(apollo::is_convertible<T>(L, -1));
    BOOST_CHECK_EQUAL(v, apollo::from_stack<T>(L, -1));
}

BOOST_AUTO_TEST_CASE(number_converter)
{
    check_roundtrip(L, -42); // int
    check_roundtrip(L, 42u); // unsigned
    check_roundtrip(L, -42L); // long
    check_roundtrip(L, 42uL); // unsigned long
    check_roundtrip(L, -42LL); // long long
    check_roundtrip(L, 0xfffffffffffffLL); // unsigned long long
    check_roundtrip(L, static_cast<short>(-42));
    check_roundtrip(L, static_cast<unsigned short>(42));

    check_roundtrip(L, static_cast<signed char>(-42));
    check_roundtrip(L, static_cast<unsigned char>(42));

    check_roundtrip(L, 42.0); // double
    check_roundtrip(L, 42.f); // float

    apollo::push(L, 42);
    apollo::push(L, "foo");
    BOOST_CHECK_THROW(apollo::from_stack<int>(L, -1), apollo::conversion_error);
    BOOST_CHECK_EQUAL(apollo::from_stack<int>(L, -2), 42);
    lua_pop(L, 2);
}

static void check_bool_fallback(lua_State* L, bool expected = true)
{
    BOOST_REQUIRE(apollo::is_convertible<bool>(L, -1));
    BOOST_CHECK_EQUAL(
        apollo::converter<bool>::n_conversion_steps(L, -1),
        apollo::no_conversion - 1);
    BOOST_CHECK_EQUAL(apollo::from_stack<bool>(L, -1), expected);
    lua_pop(L, 1);
}

BOOST_AUTO_TEST_CASE(bool_converter)
{
    check_roundtrip<LUA_TBOOLEAN>(L, true);
    check_roundtrip<LUA_TBOOLEAN>(L, false);

    lua_pushnil(L);
    check_bool_fallback(L, false);

    lua_pushliteral(L, "");
    check_bool_fallback(L);

    lua_pushnumber(L, 1.0);
    check_bool_fallback(L);

    apollo::push(L, true);
    apollo::push(L, false);
    BOOST_CHECK_EQUAL(apollo::from_stack<bool>(L, -2), true);
    BOOST_CHECK_EQUAL(apollo::from_stack<bool>(L, -1), false);
    lua_pop(L, 2);
}

template <typename T>
static void check_str_roundtrip(lua_State* L, T&& v)
{
    check_roundtrip<LUA_TSTRING>(L, v);
}

// sz: Length of s inclusive terminating '\0'.
static void check_streq(lua_State* L, char const* s, std::size_t sz)
{
    std::size_t len;
    auto cstr = lua_tolstring(L, -1, &len);
    BOOST_CHECK_EQUAL(len, sz - 1);
    BOOST_CHECK_EQUAL(memcmp(cstr, s, sz), 0);
    lua_pop(L, 1);
}

BOOST_AUTO_TEST_CASE(string_converter)
{
    // Literals should preserve embedded zeros and text thereafter.
    auto const& str = "abc\0def";
    apollo::push(L, str);
    check_streq(L, str, sizeof(str));

    // Mutable char arrays should do the same.
    char buf[] = "abc\0def";
    apollo::push(L, buf);
    check_streq(L, buf, sizeof(buf));

    char const* cstr = buf;
    apollo::push(L, cstr);
    check_streq(L, "abc", sizeof("abc"));

    std::string stdstr(str, sizeof(str) - 1);
    check_str_roundtrip(L, stdstr);
    check_str_roundtrip(L, "abc");
    check_str_roundtrip(L, 'c');
    check_str_roundtrip(L, '\0');

    apollo::push(L, buf);
    apollo::push(L, true);
    BOOST_CHECK_THROW(
        apollo::from_stack<char const*>(L, -1), apollo::conversion_error);
    BOOST_CHECK_THROW(
        apollo::from_stack<std::string>(L, -1), apollo::conversion_error);
    BOOST_CHECK_EQUAL(apollo::from_stack<std::string>(L, -2), stdstr);
    BOOST_CHECK_EQUAL(
        apollo::from_stack<char const*>(L, -2), std::string("abc"));

#define CHECK_THROW_CHAR BOOST_CHECK_THROW( \
    apollo::from_stack<char>(L, -1), apollo::conversion_error)

    apollo::push(L, 'c');
    lua_replace(L, -3);
    CHECK_THROW_CHAR;
    BOOST_CHECK_EQUAL(apollo::from_stack<char>(L, -2), 'c');
    lua_pop(L, 2);

    apollo::push(L, 42);
    BOOST_CHECK_EQUAL(apollo::from_stack<std::string>(L, -1), "42");
    CHECK_THROW_CHAR;
    lua_pop(L, 1);
    apollo::push(L, 4);
    BOOST_CHECK_EQUAL(apollo::from_stack<char>(L, -1), '4');

    lua_pop(L, 1);
    apollo::push(L, 9);
    BOOST_CHECK_EQUAL(apollo::from_stack<char>(L, -1), '9');

    lua_pop(L, 1);
    apollo::push(L, 4.1);
    CHECK_THROW_CHAR;

    lua_pop(L, 1);
    apollo::push(L, -4);
    CHECK_THROW_CHAR;

    lua_pop(L, 1);
    apollo::push(L, 10);
    CHECK_THROW_CHAR;

    lua_pop(L, 1);
}

#include "test_suffix.hpp"
