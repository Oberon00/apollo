#ifndef APOLLO_CONVERTERS_HPP_INCLUDED
#define APOLLO_CONVERTERS_HPP_INCLUDED APOLLO_CONVERTERS_HPP_INCLUDED

#include <apollo/converters_fwd.hpp>
#include <apollo/error.hpp>
#include <apollo/detail/meta_util.hpp>

#include <boost/assert.hpp>
#include <boost/exception/errinfo_type_info_name.hpp>
#include <boost/exception/info.hpp>
#include <boost/function/function_fwd.hpp>
#include <boost/throw_exception.hpp>
#include <lua.hpp>

#include <climits>
#include <functional>
#include <string>
#include <type_traits>

namespace apollo {

struct raw_function {
    /* implicit */ BOOST_CONSTEXPR
        raw_function(lua_CFunction f_) BOOST_NOEXCEPT
        : f(f_) {}
    /* implicit */ BOOST_CONSTEXPR
        operator lua_CFunction() const BOOST_NOEXCEPT
    {
        return f;
    }

    lua_CFunction f;
};

template <typename Converter>
using to_type_of = typename detail::remove_qualifiers<Converter>::type::to_type;


namespace detail {
// Lua type constants //

template <typename T, typename Enable=void> // Default to userdata.
struct lua_type_id: std::integral_constant<int, LUA_TUSERDATA> {};

template <typename T> // Any arithmetic type except bool is a number.
struct lua_type_id<T,
        typename std::enable_if<
            std::is_same<T, typename detail::remove_qualifiers<T>::type>::value
            && std::is_arithmetic<T>::value>::type>
        : std::integral_constant<int, LUA_TNUMBER> {};

template <> // boolean
struct lua_type_id<bool>: std::integral_constant<int, LUA_TBOOLEAN> {};

template <> struct lua_type_id<void>: std::integral_constant<int, LUA_TNIL> {};

// string
template <>
struct lua_type_id<char*>: std::integral_constant<int, LUA_TSTRING> {};
template <> struct lua_type_id<char const*>: lua_type_id<char*> {};
template <> struct lua_type_id<char>: lua_type_id<char*> {};
template <std::size_t N> struct lua_type_id<char[N]>: lua_type_id<char*> {};
template <> struct lua_type_id<std::string>: lua_type_id<char*> {};


template <> // thread (lua_State*)
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
template <typename T> struct lua_type_id<boost::function<T>>
    : lua_type_id<T> {};
template <> struct lua_type_id<raw_function>
    : std::integral_constant<int, LUA_TFUNCTION> {};

} // namespace detail

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
    using T2 = typename remove_qualifiers<T>::type;
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
struct string_conversion_steps<char> {
    static unsigned get(lua_State* L, int idx);
};

} // namespace detail

template <typename T>
struct converter<T, typename std::enable_if<
        detail::lua_type_id<T>::value == LUA_TSTRING>::type>: converter_base<T> {
    using to_type = typename detail::to_string<T>::type;

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

// Const references to primitive types are handled as non-references.
template<typename T>
struct converter<T, typename std::enable_if<
    detail::is_const_reference<T>::value &&
    detail::lua_type_id<typename detail::remove_qualifiers<T>::type>::value
     != LUA_TUSERDATA>::type
>: converter<typename detail::remove_qualifiers<T>::type>
{};

template <typename T>
using push_converter_for = converter<
    typename detail::remove_qualifiers<T>::type>;

template <typename T>
using pull_converter_for = converter<typename std::remove_cv<T>::type>;

namespace detail {

inline void push_impl(lua_State*)
{ }

template <typename Head, typename... Tail>
void push_impl(lua_State* L, Head&& head, Tail&&... tail)
{
    push_converter_for<Head>::push(
        L, std::forward<Head>(head));
    push_impl(L, std::forward<Tail>(tail)...);
}

} // namespace detail


template <typename T, typename... MoreTs>
void push(lua_State* L, T&& v, MoreTs&&... more)
{
    detail::push_impl(L, std::forward<T>(v), std::forward<MoreTs>(more)...);
}

namespace detail {

    failure_t converter_has_idx_param_impl(...);

    template <typename Converter>
    auto converter_has_idx_param_impl(Converter conv)
        -> decltype(conv.from_stack(
            std::declval<lua_State*>(), 0, std::declval<int*>()));

    template <typename Converter>
    struct converter_has_idx_param: std::integral_constant<bool, !has_failed<
            decltype(converter_has_idx_param_impl(std::declval<Converter>()))
        >::value>
    {};
}

template <typename Converter>
typename std::enable_if<
    !detail::converter_has_idx_param<Converter>::value,
    to_type_of<Converter>>::type
unchecked_from_stack_with(
    Converter&& conv, lua_State* L, int idx, int* next_idx = nullptr)
{
    (void)conv; // Silence MSVC.
    if (next_idx)
        *next_idx = idx + conv.n_consumed;
    return conv.from_stack(L, idx);
}

template <typename Converter>
typename std::enable_if<
    detail::converter_has_idx_param<Converter>::value,
    to_type_of<Converter>>::type
unchecked_from_stack_with(
    Converter&& conv, lua_State* L, int idx, int* next_idx = nullptr)
{
    return conv.from_stack(L, idx, next_idx);
}

template <typename T>
to_type_of<pull_converter_for<T>> unchecked_from_stack(lua_State* L, int idx)
{
    return unchecked_from_stack_with(pull_converter_for<T>(), L, idx);
}

template <typename Converter>
bool is_convertible_with(Converter const& conv, lua_State* L, int idx)
{
    (void)conv; // Silence MSVC.
    return conv.n_conversion_steps(L, idx) != no_conversion;
}

template <typename T>
bool is_convertible(lua_State* L, int idx)
{
    return is_convertible_with(pull_converter_for<T>(), L, idx);
}

template <typename Converter>
to_type_of<Converter> from_stack_with(
    Converter&& conv, lua_State* L, int idx, int* next_idx = nullptr)
{
    if (!is_convertible_with(conv, L, idx)) {
        BOOST_THROW_EXCEPTION(to_cpp_conversion_error()
            << boost::errinfo_type_info_name(
                typeid(to_type_of<Converter>).name())
            << errinfo::msg("conversion from Lua to C++ failed")
            << errinfo::stack_index(idx)
            << errinfo::lua_state(L));
    }
    return unchecked_from_stack_with(conv, L, idx, next_idx);
}

template <typename T>
to_type_of<pull_converter_for<T>> from_stack(lua_State* L, int idx)
{
    return from_stack_with(pull_converter_for<T>(), L, idx);
}

template <typename T>
to_type_of<pull_converter_for<T>> from_stack(
    lua_State* L, int idx, T&& fallback)
{
    if (!is_convertible<T>(L, idx))
        return fallback;
    return unchecked_from_stack<T>(L, idx);
}

} // namepace apollo

#endif // APOLLO_CONVERTERS_HPP_INCLUDED
