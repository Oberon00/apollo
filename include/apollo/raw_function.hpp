#ifndef APOLLO_RAW_FUNCTION_HPP_INCLUDED
#define APOLLO_RAW_FUNCTION_HPP_INCLUDED APOLLO_RAW_FUNCTION_HPP_INCLUDED

#include <apollo/make_function.hpp>

namespace apollo {

namespace detail {

template <typename F, F FVal, int... Is>
inline int static_entry_point_impl(lua_State* L, iseq<Is...>) BOOST_NOEXCEPT
{
    auto def_converters = default_converters(FVal);
    return call_with_stack_args_and_push_lerror(
        L, FVal, std::move(std::get<Is>(def_converters))...);
}

template <typename F, F FVal>
inline int static_entry_point(lua_State* L) BOOST_NOEXCEPT
{
    return static_entry_point_impl<F, FVal>(
        L, tuple_seq<decltype(default_converters(FVal))>());
}

} // namespace detail

template <typename F, F FVal>
BOOST_CONSTEXPR static raw_function to_raw_function() BOOST_NOEXCEPT
{
    return &detail::static_entry_point<F, FVal>;
}

#define APOLLO_TO_RAW_FUNCTION(f) apollo::to_raw_function<decltype(f), f>()
#define APOLLO_PUSH_FUNCTION_STATIC(L, f) \
    lua_pushcfunction(L, APOLLO_TO_RAW_FUNCTION(f))

// Because MSVC fails at deducing the type of a (variadic?) template function
// address (i.e. decltype(&ctor_wrapper<T, Args...> fails), this function
// is needed.
template <typename T, typename... Args>
BOOST_CONSTEXPR raw_function get_raw_ctor_wrapper() BOOST_NOEXCEPT
{
    return to_raw_function<
        T(*)(Args...),
        &apollo::ctor_wrapper<T, Args...>>();
}

} // namespace apollo

#endif // APOLLO_RAW_FUNCTION_HPP_INCLUDED
