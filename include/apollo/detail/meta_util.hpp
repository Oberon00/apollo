#ifndef APOLLO_DETAIL_META_UTIL_HPP_INCLUDED
#define APOLLO_DETAIL_META_UTIL_HPP_INCLUDED APOLLO_DETAIL_META_UTIL_HPP_INCLUDED

#include <type_traits>

namespace apollo { namespace detail {

// Helper type function remove_qualifiers
template <typename T>
struct remove_qualifiers {
    typedef typename std::remove_cv<
    typename std::remove_reference<T>::type
    >::type type;
};

}} // namespace apollo::detail

#endif // APOLLO_DETAIL_META_UTIL_HPP_INCLUDED
