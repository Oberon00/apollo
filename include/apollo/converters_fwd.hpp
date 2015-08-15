// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifndef APOLLO_CONVERTERS_FWD_HPP_INCLUDED
#define APOLLO_CONVERTERS_FWD_HPP_INCLUDED APOLLO_CONVERTERS_FWD_HPP_INCLUDED

#include <apollo/error.hpp>
#include <boost/throw_exception.hpp>
#include <apollo/detail/lua_state.hpp>

#include <boost/config.hpp>

#include <climits>

namespace apollo {

template <typename T, typename Enable=void>
struct converter;

BOOST_CONSTEXPR_OR_CONST unsigned no_conversion = UINT_MAX;

inline BOOST_CONSTEXPR unsigned
add_conversion_step(unsigned n_steps) BOOST_NOEXCEPT
{
    return n_steps == no_conversion ? no_conversion : n_steps + 1;
}

namespace detail {

template <typename Converter>
struct default_converter_traits;

template <template<class...> class ConverterTpl, typename T, typename... Other>
struct default_converter_traits<ConverterTpl<T, Other...>>
{
    using converter = ConverterTpl<T, Other...>;

    // converter may be incomplete, so we cannot use member typedefs.
    using to_type = T;
    using type = T;
};

} // namespace detail

template <
    typename Derived,
    typename ToType = typename detail::default_converter_traits<Derived>::type>
struct converter_base {
public:
    using type = typename detail::default_converter_traits<Derived>::type;
    using to_type = ToType;

    static BOOST_CONSTEXPR_OR_CONST int n_consumed = 1;

    to_type idx_to(lua_State* L, int idx, int* next_idx) const
    {
        advance_idx(idx, next_idx);
        return derived().to(L, idx);
    }

    to_type idx_safe_to(lua_State* L, int idx, int* next_idx) const
    {
        advance_idx(idx, next_idx);
        return derived().safe_to(L, idx);
    }

    to_type safe_to(lua_State* L, int idx) const
    {
        auto const n_steps = derived().idx_n_conversion_steps(L, idx, nullptr);
        if (n_steps == apollo::no_conversion)
            BOOST_THROW_EXCEPTION(to_cpp_conversion_error());
        return derived().idx_to(L, idx, nullptr);
    }

    unsigned idx_n_conversion_steps(lua_State* L, int idx, int* next_idx) const
    {
        advance_idx(idx, next_idx);
        return derived().n_conversion_steps(L, idx);
    }

private:
    Derived const& derived() const
    {
        return *static_cast<Derived const*>(this);
    }

    void advance_idx(int idx, int* next_idx) const
    {
        if (next_idx)
            *next_idx = idx + derived().n_consumed;
    }
};

} // namepace apollo

#endif // APOLLO_CONVERTERS_FWD_HPP_INCLUDED
