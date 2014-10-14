#ifndef APOLLO_DETAIL_VARIADIC_PASS_HPP_INCLUDED
#define APOLLO_DETAIL_VARIADIC_PASS_HPP_INCLUDED APOLLO_DETAIL_VARIADIC_PASS_HPP_INCLUDED

namespace apollo { namespace detail {

template <typename... Ts>
void variadic_pass(Ts&&...)
{}

} } // namespace apollo::detail

#endif // APOLLO_DETAIL_VARIADIC_PASS_HPP_INCLUDED
