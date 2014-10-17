#ifndef APOLLO_CONVERTERS_FWD_HPP_INCLUDED
#define APOLLO_CONVERTERS_FWD_HPP_INCLUDED APOLLO_CONVERTERS_FWD_HPP_INCLUDED

#include <boost/config.hpp>

namespace apollo {

template <typename T, typename Enable=void>
struct converter;

BOOST_CONSTEXPR_OR_CONST unsigned no_conversion = UINT_MAX;

inline BOOST_CONSTEXPR unsigned add_conversion_step(unsigned n_steps)
{
    return n_steps == no_conversion ? no_conversion : n_steps + 1;
}

template<typename T>
struct converter_base {
    using to_type = T;
};

} // namepace apollo

#endif // APOLLO_CONVERTERS_FWD_HPP_INCLUDED
