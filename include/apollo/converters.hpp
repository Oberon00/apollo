// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifndef APOLLO_CONVERTERS_HPP_INCLUDED
#define APOLLO_CONVERTERS_HPP_INCLUDED APOLLO_CONVERTERS_HPP_INCLUDED

#include <apollo/converters_fwd.hpp>
#include <apollo/error.hpp>
#include <apollo/detail/meta_util.hpp>
#include <apollo/detail/ref_binder.hpp>

#include <boost/exception/errinfo_type_info_name.hpp>
#include <boost/exception/info.hpp>
#include <boost/throw_exception.hpp>
#include <boost/type_index.hpp>
#include <apollo/lua_include.hpp>

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
            && (
                std::is_arithmetic<T>::value
                || std::is_enum<T>::value)>::type>
        : std::integral_constant<int, LUA_TNUMBER> {};

template <> // boolean
struct lua_type_id<bool>: std::integral_constant<int, LUA_TBOOLEAN> {};

template <> struct lua_type_id<void>: std::integral_constant<int, LUA_TNIL> {};

template <>
struct lua_type_id<void*>: std::integral_constant<int, LUA_TLIGHTUSERDATA> {};

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

template <typename T, typename Enable=void>
struct convert_cref_by_val: std::integral_constant<bool,
    detail::lua_type_id<T>::value != LUA_TUSERDATA>
{};

// Const references to primitive types are handled as non-references.
template<typename T>
struct converter<T, typename std::enable_if<
        detail::is_const_reference<T>::value &&
        convert_cref_by_val<typename detail::remove_qualifiers<T>::type>::value
    >::type
>: converter<typename detail::remove_qualifiers<T>::type>
{};

template <typename T>
using push_converter_for = converter<
    typename detail::remove_qualifiers<T>::type>;

template <typename T>
using pull_converter_for = converter<typename std::remove_cv<T>::type>;

namespace detail {

inline int push_impl(lua_State*)
{
    return 0;
}

template <typename Head, typename... Tail>
int push_impl(lua_State* L, Head&& head, Tail&&... tail)
{
    int const n_pushed = push_converter_for<Head>().push(
        L, std::forward<Head>(head));
    return n_pushed + push_impl(L, std::forward<Tail>(tail)...);
}

} // namespace detail


template <typename T, typename... MoreTs>
int push(lua_State* L, T&& v, MoreTs&&... more)
{
    return detail::push_impl(L,
        std::forward<T>(v), std::forward<MoreTs>(more)...);
}

template <typename Converter>
to_type_of<Converter> unchecked_to_with(
    Converter&& conv, lua_State* L, int idx, int* next_idx = nullptr)
{
    return conv.idx_to(L, idx, next_idx);
}

template <typename T>
to_type_of<pull_converter_for<T>> unchecked_to(lua_State* L, int idx)
{
    return unchecked_to_with(pull_converter_for<T>(), L, idx);
}


template <typename Converter>
unsigned n_conversion_steps_with(
    Converter&& conv, lua_State* L, int idx, int* next_idx = nullptr)
{
    return conv.idx_n_conversion_steps(L, idx, next_idx);
}


template <typename T>
unsigned n_conversion_steps(lua_State* L, int idx)
{
    return n_conversion_steps_with(pull_converter_for<T>(), L, idx);
}

template <typename Converter>
bool is_convertible_with(Converter const& conv, lua_State* L, int idx)
{
    (void)conv; // Silence MSVC.
    return n_conversion_steps_with(conv, L, idx) != no_conversion;
}

template <typename T>
bool is_convertible(lua_State* L, int idx)
{
    return is_convertible_with(pull_converter_for<T>(), L, idx);
}

template <typename Converter>
to_type_of<Converter> to_with(
    Converter&& conv, lua_State* L, int idx, int* next_idx = nullptr)
{
    if (!is_convertible_with(conv, L, idx)) {
        BOOST_THROW_EXCEPTION(to_cpp_conversion_error()
            << boost::errinfo_type_info_name(
                boost::typeindex::type_id<to_type_of<Converter>>()
                .pretty_name())
            << errinfo::msg("conversion from Lua to C++ failed")
            << errinfo::stack_index(idx)
            << errinfo::lua_state(L));
    }
    return unchecked_to_with(conv, L, idx, next_idx);
}

template <typename T>
to_type_of<pull_converter_for<T>> to(lua_State* L, int idx)
{
    return to_with(pull_converter_for<T>(), L, idx);
}

// Cannot use function here, it would potentially return a reference to local.
#define APOLLO_TO_ARG(L, idx, ...) \
    ::apollo::unwrap_ref(::apollo::to<__VA_ARGS__>(L, idx))

template <typename T>
to_type_of<pull_converter_for<T>> to(lua_State* L, int idx, T&& fallback)
{
    if (!is_convertible<T>(L, idx))
        return std::forward<T>(fallback);
    return unchecked_to<T>(L, idx);
}

} // namepace apollo

#endif // APOLLO_CONVERTERS_HPP_INCLUDED
