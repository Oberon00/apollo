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

template <typename T>
struct is_const_reference: std::integral_constant<bool,
    std::is_reference<T>::value &&
    std::is_const<typename std::remove_reference<T>::type>::value>
{};

template <typename F>
using is_mem_fn = std::is_member_function_pointer<
        typename detail::remove_qualifiers<F>::type>;

struct failure_t;

template <typename T>
using has_failed = std::is_same<T, failure_t>;

// Workaround MSVC not being able to call ctors in pack expansions:
template <typename T>
T default_constructed()
{
    return {};
}

}} // namespace apollo::detail

#endif // APOLLO_DETAIL_META_UTIL_HPP_INCLUDED
