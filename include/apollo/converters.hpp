#ifndef APOLLO_CONVERTERS_HPP_INCLUDED
#define APOLLO_CONVERTERS_HPP_INCLUDED APOLLO_CONVERTERS_HPP_INCLUDED

#include <lua.hpp>
#include <type_traits>
#include <boost/assert.hpp>
#include <string>
#include <climits>
#include <functional>
#include <boost/function/function_fwd.hpp>

#include <apollo/error.hpp>
#include <boost/throw_exception.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/errinfo_type_info_name.hpp>

namespace apollo {


namespace detail {
// Lua type constants //

template <typename T, typename Enable=void> // Default to userdata.
struct lua_type_id: std::integral_constant<int, LUA_TUSERDATA> {};

template <typename T> // Any arithmetic type except bool is a number.
struct lua_type_id<T,
        typename std::enable_if<std::is_arithmetic<T>::value>::type>
        : std::integral_constant<int, LUA_TNUMBER> {};

template <> // boolean
struct lua_type_id<bool> {
    static int const value = LUA_TBOOLEAN;
};

// string
template <> struct lua_type_id<char*>
        : std::integral_constant<int, LUA_TSTRING> {};
template <> struct lua_type_id<char const*>: lua_type_id<char*> {};
template <> struct lua_type_id<char>: lua_type_id<char*> {};
template <std::size_t N> struct lua_type_id<char[N]>: lua_type_id<char*> {};
template <> struct lua_type_id<std::string>: lua_type_id<char*> {};


template<> // thread (lua_State*)
struct lua_type_id<lua_State*>: std::integral_constant<int, LUA_TTHREAD> {};

template <typename T>
struct is_plain_function: std::integral_constant<bool,
        std::is_function<typename std::remove_pointer<T>::type>::value>
{ };

// function (plain function (pointer), boost and std function templates)
template <typename T>
struct lua_type_id<T,
        typename std::enable_if<
        detail::is_plain_function<T>::value ||
        std::is_member_function_pointer<T>::value>::type>
        : std::integral_constant<int, LUA_TFUNCTION> {};
template <typename T> struct lua_type_id<std::function<T>>: lua_type_id<T> {};
template <typename T> struct lua_type_id<boost::function<T>>: lua_type_id<T> {};

// Helper type function remove_qualifiers
template <typename T>
struct remove_qualifiers {
    typedef typename std::remove_cv<
    typename std::remove_reference<T>::type
    >::type type;
};

} // namespace detail

template <typename T, typename Enable=void>
struct converter; // See the converters below for examples.

BOOST_CONSTEXPR_OR_CONST unsigned no_conversion = UINT_MAX;

template<typename T>
struct converter_base {
    typedef T type;
    typedef type to_type;
    static int BOOST_CONSTEXPR_OR_CONST lua_type_id =
        detail::lua_type_id<T>::value;
    static bool BOOST_CONSTEXPR_OR_CONST is_native =
        lua_type_id != LUA_TUSERDATA;
};

// Number converter //
template<typename T>
struct converter<T, typename std::enable_if<
    detail::lua_type_id<T>::value == LUA_TNUMBER>::type>: converter_base<T> {

    static void push(lua_State* L, T n)
    {
        lua_pushnumber(L, static_cast<lua_Number>(n));
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        if (lua_type(L, idx) == LUA_TNUMBER) // Actual number.
            return 0;
        if (lua_isnumber(L, idx)) // String convertible to number.
            return 1;
        return no_conversion;
    }

    static T from_stack(lua_State* L, int idx)
    {
#ifdef NDEBUG
        return static_cast<T>(lua_tonumber(L, idx));
#else
        int isnum;
        T n = static_cast<T>(lua_tonumberx(L, idx, &isnum));
        BOOST_ASSERT(isnum);
        return n;
#endif
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


// String converter //

namespace detail {

inline char const* push_string(lua_State* L, char c)
{
    return lua_pushlstring(L, &c, 1);
}

template <std::size_t N>
inline char const* push_string(lua_State* L, char const (&s)[N])
{
    // Don't count null termination.
    return lua_pushlstring(L, s, N - (s[N - 1] == 0));
}

// Need to make this a template too, so that the array overload is called in
// the appropriate cases. Also can't make it const T* because otherwise the
// call would be ambigous.
template <typename T>
inline char const* push_string(lua_State* L, T s)
{
    typedef typename remove_qualifiers<T>::type T2;
    static_assert(
        std::is_same<T2, char*>::value || std::is_same<T2, char const*>::value,
        "push_string called with non-string");
    return lua_pushstring(L, s);
}

inline char const* push_string(lua_State* L, std::string const& s)
{
    return lua_pushlstring(L, s.c_str(), s.size());
}

template <typename T>
struct to_string { // char*, char const*, char[N]
    typedef char const* type;
    static type from_stack(lua_State* L, int idx)
    {
        return lua_tostring(L, idx);
    }
};

template <>
struct to_string<std::string> {
    typedef std::string type;
    static type from_stack(lua_State* L, int idx)
    {
        std::size_t len;
        char const* s = lua_tolstring(L, idx, &len);
        return std::string(s, len);
    }
};

template <>
struct to_string<char> {
    typedef char type;
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
struct string_conversion_steps<char> {
    static unsigned get(lua_State* L, int idx);
};

} // namespace detail

template<typename T>
struct converter<T, typename std::enable_if<
        detail::lua_type_id<T>::value == LUA_TSTRING>::type>: converter_base<T> {
    typedef typename detail::to_string<T>::type to_type;

    static char const* push(lua_State* L, T const& s)
    {
        return detail::push_string(L, s);
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

template <typename T>
void push(lua_State* L, T&& v)
{
    converter<T>::push(L, std::forward<T>(v));
}

// For cv and/or reference qualified native types, ignore the qualifiers.
template<typename T>
struct converter<T, typename std::enable_if<
    (std::is_const<T>::value ||
     std::is_volatile<T>::value ||
     std::is_reference<T>::value) &&
    detail::lua_type_id<typename detail::remove_qualifiers<T>::type>::value
     != LUA_TUSERDATA>::type
>: converter<typename detail::remove_qualifiers<T>::type>
{};


template <typename T>
typename converter<T>::to_type unchecked_from_stack(lua_State* L, int idx)
{
    return converter<T>::from_stack(L, idx);
}

template <typename T>
bool is_convertible(lua_State* L, int idx)
{
    return converter<T>::n_conversion_steps(L, idx) != no_conversion;
}

template <typename T>
typename converter<T>::to_type from_stack(lua_State* L, int idx)
{
    if (!is_convertible<T>(L, idx)) {
        BOOST_THROW_EXCEPTION(to_cpp_conversion_error()
            << boost::errinfo_type_info_name(typeid(T).name())
            << errinfo::msg("conversion from Lua to C++ failed")
            << errinfo::stack_index(idx)
            << errinfo::lua_state(L));
    }
    return unchecked_from_stack<T>(L, idx);
}

template <typename T>
typename converter<T>::to_type from_stack(lua_State* L, int idx, T&& fallback)
{
    if (!is_convertible<T>(L, idx))
        return fallback;
    return unchecked_from_stack<T>(L, idx);
}



} // namepace apollo

#endif // APOLLO_CONVERTERS_HPP_INCLUDED
