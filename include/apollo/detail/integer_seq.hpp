// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifndef APOLLO_INTEGER_SEQ_HPP_INCLUDED
#define APOLLO_INTEGER_SEQ_HPP_INCLUDED APOLLO_INTEGER_SEQ_HPP_INCLUDED

// Integer sequence adapted from http://stackoverflow.com/a/17426611/2128694

namespace apollo { namespace detail {

template<int...> struct iseq{ using type = iseq; };

template<class S1, class S2> struct concat_iseq;

template<int... I1, int... I2>
struct concat_iseq<iseq<I1...>, iseq<I2...>>
  : iseq<I1..., (sizeof...(I1)+I2)...>{};


template<int N, int Lo>
struct gen_iseq : concat_iseq<
    typename gen_iseq<N/2, Lo>::type,
    typename gen_iseq<N - N/2, Lo>::type>
    {};

template<int Lo> struct gen_iseq<0, Lo> : iseq<>{};
template<int Lo> struct gen_iseq<1, Lo> : iseq<Lo>{};

template <int N, int Lo = 0>
using iseq_n_t = typename gen_iseq<N, Lo>::type;

}} // namespace apollo::detail

#endif // APOLLO_INTEGER_SEQ_HPP_INCLUDED
