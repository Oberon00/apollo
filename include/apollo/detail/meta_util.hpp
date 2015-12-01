// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifndef APOLLO_DETAIL_META_UTIL_HPP_INCLUDED
#define APOLLO_DETAIL_META_UTIL_HPP_INCLUDED APOLLO_DETAIL_META_UTIL_HPP_INCLUDED

#include <type_traits>

namespace apollo { namespace detail {

// Helper type function remove_cvr
template <typename T>
using remove_cvr = typename std::remove_cv<
    typename std::remove_reference<T>::type>::type;

template <typename T>
struct is_const_reference: std::integral_constant<bool,
    std::is_reference<T>::value &&
    std::is_const<typename std::remove_reference<T>::type>::value>
{};

template <typename F>
using is_mem_fn = std::is_member_function_pointer<detail::remove_cvr<F>>;

struct failure_t;

template <typename T>
using has_failed = std::is_same<T, failure_t>;

template <typename T>
T msvc_decltype_helper(T); // Intentionally not implemented.

}} // namespace apollo::detail

#define APOLLO_FN_DECLTYPE(...) decltype( \
    ::apollo::detail::msvc_decltype_helper(__VA_ARGS__))

#endif // APOLLO_DETAIL_META_UTIL_HPP_INCLUDED
