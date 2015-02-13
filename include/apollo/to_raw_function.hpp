#ifndef APOLLO_TO_RAW_FUNCTION_HPP_INCLUDED
#define APOLLO_TO_RAW_FUNCTION_HPP_INCLUDED APOLLO_RAW_FUNCTION_HPP_INCLUDED

#include <apollo/make_function.hpp>
#include <apollo/raw_function.hpp>

namespace apollo {

namespace detail {

template <typename F, F FVal, typename... Converters>
int static_entry_point(lua_State* L) BOOST_NOEXCEPT
{
    return invoke_with(
        L, FVal, std::move(detail::default_constructed<Converters>())...);
}

template <
    typename F, F FVal,
    typename ResultConverter, typename... ArgConverters>
struct light_converted_function {
public:
    using dispatch_t = function_dipatcher<
        F, ResultConverter, ArgConverters...>;
    using tuple_t = typename dispatch_t::tuple_t;

    tuple_t converters;

    template <typename... AllConverters>
    light_converted_function(init_fn_tag, AllConverters&&... converters_)
        : converters(std::forward<AllConverters>(converters_)...)
    {}

    static int entry_point(lua_State* L) BOOST_NOEXCEPT
    {
        return dispatch_t::call(L, FVal, 1);
    }

    F fn() const { return FVal; }
};

} // namespace detail

template<
    typename F, F FVal,
    typename ResultConverter, typename... Converters>
struct converter<detail::light_converted_function<
        F, FVal, ResultConverter, Converters...>>
    : converter_base<detail::light_converted_function<
        F, FVal, ResultConverter, Converters...>> {
public:
    using type = detail::light_converted_function<
        F, FVal, ResultConverter, Converters...>;

    static int push(lua_State* L, type&& f)
    {
        lua_pushcclosure(
            L,
            &f.entry_point,
            type::dispatch_t::push_converters(L, std::move(f.converters)) ?
                1 : 0);
        return 1;
    }
};

template <
    typename F, F FVal,
    typename ResultConverter, typename... ArgConverters>
BOOST_CONSTEXPR raw_function to_raw_function_with_ts() BOOST_NOEXCEPT
{
    return &detail::static_entry_point<F, FVal, ResultConverter, ArgConverters...>;
}

template <
    typename F, F FVal,
    typename ResultConverter, typename... ArgConverters>
detail::light_converted_function<
    typename detail::remove_qualifiers<F>::type,
    FVal,
    typename detail::remove_qualifiers<ResultConverter>::type,
    typename detail::remove_qualifiers<ArgConverters>::type...>
make_light_funtion_with(
    ResultConverter&& rconv, ArgConverters&&... aconvs)
{
    return {
        detail::init_fn_tag(),
        std::forward<ResultConverter>(rconv),
        std::forward<ArgConverters>(aconvs)...};
}

#define APOLLO_MAKE_LIGHT_FUNCTION(f, rconv, ...) \
    apollo::make_light_funtion_with<APOLLO_FN_DECLTYPE(f), f>(rconv, __VA_ARGS__)

namespace detail {

template <typename F, F FVal, int... Is>
BOOST_CONSTEXPR raw_function to_raw_function_impl(
    iseq<Is...>) BOOST_NOEXCEPT
{
    using cvts = decltype(default_converters(FVal));
    return to_raw_function_with_ts<F, FVal,
        // MSVC fails recognizing Is as compile time constant with tuple_element
        decltype(std::get<Is>(std::declval<cvts>()))...>();
}

} // namespace detail

template <typename F, F FVal>
BOOST_CONSTEXPR raw_function to_raw_function() BOOST_NOEXCEPT
{
    using cvts = decltype(detail::default_converters(FVal));
    return detail::to_raw_function_impl<F, FVal>(detail::tuple_seq<cvts>());
}

#define APOLLO_TO_RAW_FUNCTION(...) \
    apollo::to_raw_function<APOLLO_FN_DECLTYPE(__VA_ARGS__), __VA_ARGS__>()
#define APOLLO_PUSH_FUNCTION_STATIC(L, ...) \
    lua_pushcfunction(L, APOLLO_TO_RAW_FUNCTION(__VA_ARGS__))
#define APOLLO_PUSH_FUNCTION_STATIC_WITH(L, f, rconv, ...) \
    lua_pushcfunction(L, APOLLO_MAKE_LIGHT_FUNCTION(f, rconv, __VA_ARGS__))

} // namespace apollo

#endif // APOLLO_TO_RAW_FUNCTION_HPP_INCLUDED
