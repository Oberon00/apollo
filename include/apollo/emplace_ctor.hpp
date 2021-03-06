// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifndef APOLLO_EMPLACE_CTOR_HPP_INCLUDED
#define APOLLO_EMPLACE_CTOR_HPP_INCLUDED

#include <apollo/class.hpp>
#include <apollo/function_primitives.hpp>
#include <apollo/raw_function.hpp>

namespace apollo {

namespace detail {

template <typename T, typename ArgTuple, int... Is>
int emplace_ctor_wrapper_impl(lua_State* L, ArgTuple& args, iseq<Is...>)
{
    emplace_object<T>(L, unwrap_ref(std::get<Is>(args))...);
    (void)args;
    return 1;
}

template <typename T, typename... Args>
int emplace_ctor_wrapper(lua_State* L)
{
    auto args = to_tuple(L, 1, pull_converter_for<Args>()...);
    return emplace_ctor_wrapper_impl<T>(L, args, tuple_seq<decltype(args)>());
}

} // namespace detail

template <typename T, typename... Args>
BOOST_CONSTEXPR raw_function get_raw_emplace_ctor_wrapper() BOOST_NOEXCEPT
{
    return raw_function::caught<&detail::emplace_ctor_wrapper<T, Args...>>();
}



} // namespace apollo

#endif // APOLLO_EMPLACE_CTOR_HPP_INCLUDED
