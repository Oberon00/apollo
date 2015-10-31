// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifndef APOLLO_FUNCTION_PRIMITIVES_HPP_INCLUDED
#define APOLLO_FUNCTION_PRIMITIVES_HPP_INCLUDED APOLLO_FUNCTION_PRIMITIVES_HPP_INCLUDED

#include <apollo/converters.hpp>
#include <apollo/detail/integer_seq.hpp>
#include <apollo/detail/ref_binder.hpp>
#include <apollo/detail/signature.hpp>

#include <apollo/lua_include.hpp>

#include <tuple>
#include <type_traits>

namespace apollo {

namespace detail {

template <typename Tuple>
using tuple_seq = iseq_n_t<
    std::tuple_size<typename detail::remove_cvr<Tuple>::type>::value>;

template<bool... Bs> // http://stackoverflow.com/a/24687161/2128694
struct bool_and: std::is_same<iseq<Bs...>, iseq<(Bs || true)... >> {};

template <typename... Ts>
struct all_empty: bool_and<std::is_empty<Ts>::value...> {};

inline std::tuple<> to_tuple(lua_State*, int)
{
    return {};
}

template <typename Converter0, typename... Converters>
std::tuple<to_type_of<Converter0>, to_type_of<Converters>...>
to_tuple(
    lua_State* L, int i, Converter0&& conv0, Converters&&... convs)
{
    // Keep these statements separate to make sure the
    // recursive invocation receives the updated i.
    std::tuple<to_type_of<Converter0>> arg0(
        to_with(std::forward<Converter0>(conv0), L, i, &i));
    static_assert(std::tuple_size<decltype(arg0)>::value == 1, "");
    return std::tuple_cat(std::move(arg0), to_tuple(
        L, i, std::forward<Converters>(convs)...));
}

// Plain function pointer:
template <typename R, typename... Args>
std::tuple<push_converter_for<R>, pull_converter_for<Args>...>
default_converters(R(*)(Args...))
{
    return {};
}

// Function object (std::function, boost::function, etc.):
template <typename R, template<class> class FObj, typename... Args>
std::tuple<push_converter_for<R>, pull_converter_for<Args>...>
default_converters(FObj<R(Args...)> const&)
{
    return {};
}

// Member function pointer:
template <class C, typename R, typename... Args>
std::tuple<
    push_converter_for<R>,
    pull_converter_for<C&>,
    pull_converter_for<Args>...>
default_converters(R(C::*)(Args...))
{
    return {};
}

// Const member function pointer:
template <class C, typename R, typename... Args>
std::tuple<
    push_converter_for<R>,
    pull_converter_for<C const&>,
    pull_converter_for<Args>...>
default_converters(R(C::*)(Args...) const)
{
    return {};
}


// Plain function pointer or function object:
template <typename F, int... Is, typename... Converters>
typename std::enable_if<!is_mem_fn<F>::value, return_type_of<F>>::type
call_with_stack_args_impl(
    lua_State* L, F&& f, iseq<Is...>, Converters&&... convs)
{
    auto args = to_tuple(L, 1, std::forward<Converters>(convs)...);
    static_assert(std::tuple_size<decltype(args)>::value == sizeof...(Is), "");
    return f(unwrap_ref(std::get<Is>(args))...);
}

// (Const) member function pointer:
template <
    typename ThisConverter, typename... Converters, int... Is, typename F>
typename std::enable_if<is_mem_fn<F>::value, return_type_of<F>>::type
call_with_stack_args_impl(
    lua_State* L, F&& f,
    iseq<Is...>,
    ThisConverter&& this_conv,
    Converters&&... convs
)
{
    int i0;
    to_type_of<ThisConverter> instance = to_with(this_conv, L, 1, &i0);
    auto args = to_tuple(L, i0, std::forward<Converters>(convs)...);
    (void)args; // Silence gcc's -Wunused-but-set-variable
    return (unwrap_ref(instance).*f)(unwrap_ref(std::get<Is>(args))...);
}

} // namespace detail

template <typename F, typename... Converters>
detail::return_type_of<F> call_with_stack_args_with(
    lua_State* L, F&& f, Converters&&... converters)
{
    auto arg_seq = detail::iseq_n_t
        <sizeof...(Converters) - detail::is_mem_fn<F>::value>();
    return detail::call_with_stack_args_impl(
        L, std::forward<F>(f),
        arg_seq,
        std::forward<Converters>(converters)...);
}

namespace detail {

template <typename F, int... Is>
return_type_of<F> call_with_stack_args_def(lua_State* L, F&& f, iseq<Is...>)
{
    return call_with_stack_args_with(
        L, f,
        std::move(std::get<Is>(default_converters(f)))...);
}

} // namespace detail

template <typename F>
detail::return_type_of<F> call_with_stack_args(lua_State* L, F&& f)
{
    using all_converter_tuple = decltype(detail::default_converters(f));
    return detail::call_with_stack_args_def(
        L, f,
        // Skip result converter tuple:
        detail::iseq_n_t<std::tuple_size<all_converter_tuple>::value, 1>());
}

} // namespace apollo

#endif // APOLLO_FUNCTION_PRIMITIVES_HPP_INCLUDED
