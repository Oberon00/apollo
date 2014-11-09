#ifndef APOLLO_CONVERTERS_HPP_INCLUDED
#define APOLLO_CONVERTERS_HPP_INCLUDED APOLLO_CONVERTERS_HPP_INCLUDED

#include <apollo/converters_fwd.hpp>
#include <apollo/error.hpp>
#include <apollo/detail/meta_util.hpp>

#include <boost/exception/errinfo_type_info_name.hpp>
#include <boost/exception/info.hpp>
#include <boost/throw_exception.hpp>
#include <lua.hpp>

#include <type_traits>

namespace apollo {

struct raw_function;

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

template <template <class> class FObj, typename R, typename... Args>
struct lua_type_id<FObj<R(Args...)>>
    : std::integral_constant<int, LUA_TFUNCTION> {};

template <> struct lua_type_id<raw_function>
    : std::integral_constant<int, LUA_TFUNCTION> {};

} // namespace detail

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
