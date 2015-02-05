#ifndef APOLLO_BUILTIN_TYPES_HPP_INCLUDED
#define APOLLO_BUILTIN_TYPES_HPP_INCLUDED APOLLO_BUILTIN_TYPES_HPP_INCLUDED

#include <apollo/config.hpp>
#include <apollo/converters.hpp>

#include <boost/assert.hpp>

#include <limits>
#include <string>

namespace apollo {

struct raw_function {
    /* implicit */ BOOST_CONSTEXPR
    raw_function(lua_CFunction f_) BOOST_NOEXCEPT
        : f(f_) {}

    template <lua_CFunction FVal>
    static BOOST_CONSTEXPR raw_function caught() BOOST_NOEXCEPT
    {
        return static_cast<lua_CFunction>([](lua_State* L) -> int {
            return exceptions_to_lua_errors_L(L, FVal);
        });
    }

    /* implicit */ BOOST_CONSTEXPR
    operator lua_CFunction() const BOOST_NOEXCEPT
    {
        return f;
    }

    lua_CFunction f;
};


// Number converter //
template<typename T>
struct converter<T, typename std::enable_if<
        !std::is_enum<T>::value
        && detail::lua_type_id<T>::value == LUA_TNUMBER>::type
    >: converter_base<T> {
private:
    using llimits = std::numeric_limits<lua_Integer>;
    static bool const is_integral = std::is_integral<T>::value;
    static bool const is_safe_integral = is_integral &&
        sizeof(T) >= sizeof(lua_Integer) &&
        std::is_signed<T>::value == std::is_signed<lua_Integer>::value;

public:
    static void push(lua_State* L, T n)
    {
#ifdef BOOST_MSVC
#   pragma warning(push)
#   pragma warning(disable:4127) // Conditional expression is constant.
#endif
        if (is_integral && (is_safe_integral
#if LUA_VERSION_NUM >= 503 // Not worth the effort on < 5.3.
            || (n >= llimits::min() && n <= llimits::max())
#endif // LUA_VERSION_NUM >= 503
        )) {
#ifdef BOOST_MSVC
#   pragma warning(pop)
#endif
            lua_pushinteger(L, static_cast<lua_Integer>(n));
        } else {
            lua_pushnumber(L, static_cast<lua_Number>(n));
        }
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        if (lua_type(L, idx) == LUA_TNUMBER) { // Actual number.
#if LUA_VERSION_NUM >= 503
            if ((lua_isinteger(L, idx) ? true : false) == is_integral)
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

    static T from_stack(lua_State* L, int idx)
    {
#if LUA_VERSION_NUM >= 503
#   ifdef BOOST_MSVC
#       pragma warning(push)
#       pragma warning(disable:4127) // Conditional expression is constant.
#   endif
        if (is_integral && lua_isinteger(L, idx))
            return static_cast<T>(lua_tointeger(L, idx));
#   ifdef BOOST_MSVC
#       pragma warning(pop)
#   endif
#endif
        return static_cast<T>(lua_tonumber(L, idx));
    }
};


// Enum converter //
template<typename T>
struct converter<T,
        typename std::enable_if<std::is_enum<T>::value>::type
    >: converter_base<T> {

    static void push(lua_State* L, T n)
    {
        lua_pushinteger(L, static_cast<lua_Integer>(n));
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        if (lua_type(L, idx) == LUA_TNUMBER)
            return 1;
        return no_conversion;
    }

    static T from_stack(lua_State* L, int idx)
    {
        return static_cast<T>(lua_tointeger(L, idx));
    }
};


// Boolean converter //
template<>
struct converter<bool>: converter_base<bool> {

    static void push(lua_State* L, bool b)
    {
        lua_pushboolean(L, static_cast<int>(b));
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        // Convert non-boolean values to bool only as a last resort.
        return lua_isboolean(L, idx) ? 0 : no_conversion - 1;
    }

    static bool from_stack(lua_State* L, int idx)
    {
        // Avoid MSVC "performance warning" about int to bool conversion with
        // ternary operator.
        return lua_toboolean(L, idx) ? true : false;
    }
};

// void converter //
template<>
struct converter<void>: converter_base<void> {
    static unsigned n_conversion_steps(lua_State*, int)
    {
        return no_conversion - 1;
    }

    static void from_stack(lua_State*, int)
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
    using T2 = typename remove_qualifiers<T>::type;
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
    static type from_stack(lua_State* L, int idx)
    {
        return lua_tostring(L, idx);
    }
};

template <>
struct to_string<std::string> {
    using type = std::string;
    static type from_stack(lua_State* L, int idx)
    {
        std::size_t len;
        char const* s = lua_tolstring(L, idx, &len);
        return std::string(s, len);
    }
};

template <>
struct to_string<char> {
    using type = char;
    static type from_stack(lua_State* L, int idx)
    {
        std::size_t len;
        char const* s = lua_tolstring(L, idx, &len);
        BOOST_ASSERT(len == 1);
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
        detail::lua_type_id<T>::value == LUA_TSTRING>::type>: converter_base<T> {
    using to_type = typename detail::to_string<T>::type;

    static void push(lua_State* L, T const& s)
    {
        detail::push_string(L, s);
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
       return detail::string_conversion_steps<T>::get(L, idx);
    }

    static to_type from_stack(lua_State* L, int idx)
    {
        return detail::to_string<T>::from_stack(L, idx);
    }
};

template <>
struct converter<raw_function>: converter_base<raw_function> {
    static void push(lua_State* L, raw_function const& rf)
    {
        lua_pushcfunction(L, rf.f);
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        return lua_iscfunction(L, idx) ? 0 : no_conversion;
    }

    static raw_function from_stack(lua_State* L, int idx)
    {
        return lua_tocfunction(L, idx);
    }
};

template <>
struct converter<void*>: converter_base<void*> {
    static void push(lua_State* L, void* p)
    {
        lua_pushlightuserdata(L, p);
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        return lua_islightuserdata(L, idx) ? 0 : no_conversion;
    }

    static void* from_stack(lua_State* L, int idx)
    {
        return lua_touserdata(L, idx);
    }
};

} // namepace apollo

#endif // APOLLO_CONVERTERS_HPP_INCLUDED
