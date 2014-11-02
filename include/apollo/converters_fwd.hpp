#ifndef APOLLO_CONVERTERS_FWD_HPP_INCLUDED
#define APOLLO_CONVERTERS_FWD_HPP_INCLUDED APOLLO_CONVERTERS_FWD_HPP_INCLUDED

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

template <typename T>
struct converter_base {
    using type = T;
    using to_type = T;
    static BOOST_CONSTEXPR_OR_CONST int n_consumed = 1;
};

} // namepace apollo

#endif // APOLLO_CONVERTERS_FWD_HPP_INCLUDED
