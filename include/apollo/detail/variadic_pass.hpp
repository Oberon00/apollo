// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifndef APOLLO_DETAIL_VARIADIC_PASS_HPP_INCLUDED
#define APOLLO_DETAIL_VARIADIC_PASS_HPP_INCLUDED APOLLO_DETAIL_VARIADIC_PASS_HPP_INCLUDED

namespace apollo { namespace detail {

template <typename... Ts>
void variadic_pass(Ts&&...)
{}

} } // namespace apollo::detail

#endif // APOLLO_DETAIL_VARIADIC_PASS_HPP_INCLUDED
