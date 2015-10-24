// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifndef APOLLO_BUILTIN_TYPES_HPP_INCLUDED
#define APOLLO_BUILTIN_TYPES_HPP_INCLUDED APOLLO_BUILTIN_TYPES_HPP_INCLUDED

#include <apollo/config.hpp>
#include <apollo/converters.hpp>

#include <boost/assert.hpp>

#include <cstdint>
#include <limits>
#include <string>

namespace apollo {

// Number converter //
template<typename T>
struct converter<T, typename std::enable_if<
        !std::is_enum<T>::value
        && detail::lua_type_id<T>::value == LUA_TNUMBER>::type
    >: converter_base<converter<T>> {
private:
    static bool const is_integral = std::is_integral<T>::value;
    static bool const is_safe_integral = is_integral &&
        sizeof(T) <= sizeof(lua_Integer) && // TODO heap allocated?!
        std::is_signed<T>::value == std::is_signed<lua_Integer>::value;

public:
    static int push(lua_State* L, T n)
    {
        APOLLO_DETAIL_CONSTCOND_BEGIN
        if (is_integral && (is_safe_integral || fits_in_lua_integer(n))) {
        APOLLO_DETAIL_CONSTCOND_END
            lua_pushinteger(L, static_cast<lua_Integer>(n));
        } else {
            lua_pushnumber(L, static_cast<lua_Number>(n));
        }
        return 1;
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        if (lua_type(L, idx) == LUA_TNUMBER) { // Actual number.
#if LUA_VERSION_NUM >= 503
            if ((lua_isinteger(L, idx) ? true : false) == is_integral) // TODO make outer
                return 0;
            return 1;
#else // LUA_VERSION_NUM >= 503
            return 0;
#endif // LUA_VERSION_NUM >= 503 / else
        }
        if (lua_isnumber(L, idx)) // String convertible to number.
            return 2;
        return no_conversion;
    }

    static T to(lua_State* L, int idx)
    {
#if LUA_VERSION_NUM >= 503
        APOLLO_DETAIL_CONSTCOND_BEGIN
        if (is_integral && lua_isinteger(L, idx))
            return static_cast<T>(lua_tointeger(L, idx));
        APOLLO_DETAIL_CONSTCOND_END
#endif
        return static_cast<T>(lua_tonumber(L, idx));
    }

#if LUA_VERSION_NUM >= 502 // lua_tonumerx() is only available since 5.2
    static T safe_to(lua_State* L, int idx)
    {
#   if LUA_VERSION_NUM >= 503
        APOLLO_DETAIL_CONSTCOND_BEGIN
        if (is_integral && lua_isinteger(L, idx))
            return static_cast<T>(lua_tointeger(L, idx));
        APOLLO_DETAIL_CONSTCOND_END
#   endif
        int isnum;
        auto n = lua_tonumberx(L, idx, &isnum);
        if (!isnum)
            BOOST_THROW_EXCEPTION(to_cpp_conversion_error());
        return static_cast<T>(n);
    }
#endif

private:
    // Inspired by http://stackoverflow.com/a/17251989.
    static bool fits_in_lua_integer(T n) {
#if LUA_VERSION_NUM >= 503
        // MSVC complains that n is unreferenced if it can determine the
        // result of the function at compile-time.
        (void)n;

        using llimits = std::numeric_limits<lua_Integer>;
        using tlimits = std::numeric_limits<T>;

        // C4309 'static_cast': truncation of constant value
        // Is emited for unsigned char and unsigned int here, but I have no
        // idea why -- after all min() in these cases is 0 for T.
        APOLLO_DETAIL_PUSHMSWARN(4309)
        auto const l_lo = static_cast<std::intmax_t>(llimits::min());
        auto const t_lo = static_cast<std::intmax_t>(tlimits::min());
        APOLLO_DETAIL_POPMSWARN

        auto const l_hi = static_cast<std::uintmax_t>(llimits::max());
        auto const t_hi = static_cast<std::uintmax_t>(tlimits::max());

        // C4309 again, but this time it might very well be that the
        // static_cast really truncates a value. However, in this case the
        // static_cast won't be evaluated because the first part of the || will
        // evaluate to true.
        APOLLO_DETAIL_PUSHMSWARN(4309)
        return (l_lo <= t_lo || n >= static_cast<T>(l_lo))  // Check underflow.
            && (l_hi >= t_hi || n <= static_cast<T>(l_hi)); // Check overflow.
        APOLLO_DETAIL_POPMSWARN
#else
        (void)n;
        return false; // Not worth the effort on Lua w/o integer support.
#endif
    }
};


// Enum converter //
template<typename T>
struct converter<T,
        typename std::enable_if<std::is_enum<T>::value>::type
    >: converter_base<T> {

    static int push(lua_State* L, T n)
    {
        lua_pushinteger(L, static_cast<lua_Integer>(n));
        return 1;
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        if (lua_type(L, idx) == LUA_TNUMBER)
            return 1;
        return no_conversion;
    }

    static T to(lua_State* L, int idx)
    {
        return static_cast<T>(lua_tointeger(L, idx));
    }
};


// Boolean converter //
template<>
struct converter<bool>: converter_base<converter<bool>> {

    static int push(lua_State* L, bool b)
    {
        lua_pushboolean(L, static_cast<int>(b));
        return 1;
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        // Convert non-boolean values to bool only as a last resort.
        return lua_isboolean(L, idx) ? 0 : no_conversion - 1;
    }

    static bool to(lua_State* L, int idx)
    {
        // Avoid MSVC "performance warning" about int to bool conversion with
        // ternary operator.
        return lua_toboolean(L, idx) ? true : false;
    }

    static bool safe_to(lua_State* L, int idx)
    {
        return to(L, idx);
    }
};

// void converter //
template<>
struct converter<void>: converter_base<converter<void>> {
    static unsigned n_conversion_steps(lua_State*, int)
    {
        return no_conversion - 1;
    }

    static void to(lua_State*, int)
    { }
};


// String converter //

namespace detail {

inline void push_string(lua_State* L, char c)
{
    lua_pushlstring(L, &c, 1);
}

template <std::size_t N>
void push_string(lua_State* L, char const (&s)[N])
{
    // Don't count null termination.
    lua_pushlstring(L, s, N - (s[N - 1] == 0));
}

// Need to make this a template too, so that the array overload is called in
// the appropriate cases. Also can't make it const T* because otherwise the
// call would be ambigous.
template <typename T>
inline void push_string(lua_State* L, T s)
{
    using T2 = typename remove_cvr<T>::type;
    static_assert(
        std::is_same<T2, char*>::value || std::is_same<T2, char const*>::value,
        "push_string called with non-string");
    lua_pushstring(L, s);
}

inline void push_string(lua_State* L, std::string const& s)
{
    lua_pushlstring(L, s.c_str(), s.size());
}

template <typename T>
struct to_string { // char*, char const*, char[N]
    using type = char const*;
    static type to(lua_State* L, int idx)
    {
        return lua_tostring(L, idx);
    }

    static type safe_to(lua_State* L, int idx)
    {
        if (char const* s = lua_tostring(L, idx))
            return s;
        BOOST_THROW_EXCEPTION(to_cpp_conversion_error());
    }
};

template <>
struct to_string<std::string> {
    using type = std::string;
    static type to(lua_State* L, int idx)
    {
        std::size_t len;
        char const* s = lua_tolstring(L, idx, &len);
        return std::string(s, len);
    }

    static type safe_to(lua_State* L, int idx)
    {
        std::size_t len;
        if (char const* s = lua_tolstring(L, idx, &len))
            return std::string(s, len);
        BOOST_THROW_EXCEPTION(to_cpp_conversion_error());
    }
};

template <>
struct to_string<char> {
    using type = char;
    static type to(lua_State* L, int idx)
    {
        std::size_t len;
        char const* s = lua_tolstring(L, idx, &len);
        BOOST_ASSERT(len == 1);
        return s[0];
    }

    static type safe_to(lua_State* L, int idx)
    {
        std::size_t len;
        char const* s = lua_tolstring(L, idx, &len);
        if (!s)
            BOOST_THROW_EXCEPTION(to_cpp_conversion_error());
        if (len != 1) {
            BOOST_THROW_EXCEPTION(to_cpp_conversion_error()
                << errinfo::msg("the string must be one byte long"));
        }
        return s[0];
    }
};

template <typename>
struct string_conversion_steps {
    static unsigned get(lua_State* L, int idx)
    {
        switch(lua_type(L, idx)) {
            case LUA_TSTRING:
                return 0;
            case LUA_TNUMBER:
                return 1;
            default:
                return no_conversion;
        }
        BOOST_UNREACHABLE_RETURN(no_conversion);
    }
};

template <>
struct APOLLO_API string_conversion_steps<char> {
    static unsigned get(lua_State* L, int idx);
};

} // namespace detail

template <typename T>
struct converter<T, typename std::enable_if<
        detail::lua_type_id<T>::value == LUA_TSTRING>::type
    >: converter_base<converter<T>, typename detail::to_string<T>::type> {

    static int push(lua_State* L, T const& s)
    {
        detail::push_string(L, s);
        return 1;
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
       return detail::string_conversion_steps<T>::get(L, idx);
    }

    static typename detail::to_string<T>::type to(lua_State* L, int idx)
    {
        return detail::to_string<T>::to(L, idx);
    }

    static typename detail::to_string<T>::type safe_to(lua_State* L, int idx)
    {
        return detail::to_string<T>::safe_to(L, idx);
    }
};

template <>
struct converter<void*>: converter_base<converter<void*>> {
    static int push(lua_State* L, void* p)
    {
        lua_pushlightuserdata(L, p);
        return 1;
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        return lua_islightuserdata(L, idx) ? 0 : no_conversion;
    }

    static void* to(lua_State* L, int idx)
    {
        return lua_touserdata(L, idx);
    }

    static void* safe_to(lua_State* L, int idx)
    {
        if (void* ud = lua_touserdata(L, idx))
            return ud;
        BOOST_THROW_EXCEPTION(to_cpp_conversion_error());
    }
};

} // namepace apollo

#endif // APOLLO_CONVERTERS_HPP_INCLUDED
