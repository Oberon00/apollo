#ifndef APOLLO_MAKE_FUNCTION_HPP_INCLUDED
#define APOLLO_MAKE_FUNCTION_HPP_INCLUDED APOLLO_MAKE_FUNCTION_HPP_INCLUDED

#include <apollo/config.hpp>
#include <apollo/error.hpp>
#include <apollo/function_primitives.hpp>
#include <apollo/gc.hpp>

namespace apollo {

namespace detail {

APOLLO_API void push_function_tag(lua_State* L);

int const
    fn_upval_fn = 1,
    fn_upval_tag = 2,
    fn_upval_type = 3,
    fn_upval_converters = 4;

template <typename F>
struct light_function_holder {
    F f;
};

template <typename F>
struct is_light_function: std::integral_constant<bool,
    (is_plain_function<F>::value || is_mem_fn<F>::value)
    && sizeof(light_function_holder<F>) <= sizeof(void*)>
{ };

// void-returning f
template <typename F, typename ResultConverter, typename... Converters>
int call_with_stack_args_and_push_impl(
    lua_State* L, F&& f, std::true_type,
    ResultConverter&&, Converters&&... converters)
{
    call_with_stack_args_with(
        L, std::forward<F>(f),
        std::forward<Converters>(converters)...);
    return 0;
}

// non-void returning f
template <typename F, typename ResultConverter, typename... Converters>
int call_with_stack_args_and_push_impl(
    lua_State* L, F&& f, std::false_type,
    ResultConverter&& rconverter, Converters&&... converters)
{
    (void)rconverter; // Avoid MSVC warning if push is static.
     rconverter.push(L, call_with_stack_args_with(
        L, std::forward<F>(f),
        std::forward<Converters>(converters)...));
    return 1;
}

template <typename F, typename ResultConverter, typename... Converters>
int call_with_stack_args_and_push(
    lua_State* L, F&& f,
    ResultConverter&& rconverter, Converters&&... converters)
{
    auto is_void_returning = std::is_void<to_type_of<ResultConverter>>();
    return call_with_stack_args_and_push_impl(
        L, std::forward<F>(f),
        is_void_returning,
        std::forward<ResultConverter>(rconverter),
        std::forward<Converters>(converters)...);
}

template <typename F, typename ResultConverter, typename... Converters>
int invoke_with(
    lua_State* L, F&& f,
    ResultConverter&& rconverter, Converters&&... converters)  BOOST_NOEXCEPT
{
    return exceptions_to_lua_errors_L(
        L, &call_with_stack_args_and_push<F, ResultConverter, Converters...>,
        std::forward<F>(f),
        std::forward<ResultConverter>(rconverter),
        std::forward<Converters>(converters)...);
}

template <typename F, typename ConverterTuple, int... Is>
int invoke_with_tuple_impl(
    lua_State* L, F&& f,
    ConverterTuple&& converters,
    iseq<Is...>)  BOOST_NOEXCEPT
{
    return invoke_with(L, std::forward<F>(f), std::get<Is>(converters)...);
}

template <typename F, typename ConverterTuple>
int invoke_with_tuple(
    lua_State* L, F&& f,
    ConverterTuple&& converters)  BOOST_NOEXCEPT
{
    return invoke_with_tuple_impl(
        L, std::forward<F>(f),
        std::forward<ConverterTuple>(converters),
        tuple_seq<ConverterTuple>());
}

struct init_fn_tag {}; // To avoid being as for move/copy ctor.

template <typename F, typename ResultConverter, typename... ArgConverters>
struct function_dipatcher {
    using stores_converters_t = std::integral_constant<bool,
        !all_empty<ResultConverter, ArgConverters...>::value>;
    static bool const stores_converters = stores_converters_t::value;

    using tuple_t = std::tuple<ResultConverter, ArgConverters...>;

    static int call(
        lua_State* L,
        typename std::conditional<
            is_plain_function<F>::value || is_mem_fn<F>::value,
            F, F&>::type f,
        int cvt_up_idx) BOOST_NOEXCEPT
    {
        return call_with_stored_converters(
            L, f, cvt_up_idx, stores_converters_t());
    }

    static tuple_t* push_converters(lua_State* L, tuple_t&& converters)
    {
#ifdef BOOST_MSVC
#   pragma warning(push)
#   pragma warning(disable:4127) // Conditional expression is constant.
#endif
        if (stores_converters) {
#ifdef BOOST_MSVC
#   pragma warning(pop)
#endif
            return push_gc_object(L, std::move(converters));
        }
        return nullptr;
    }

private:
    static int call_with_stored_converters( // No stored convertes:
        lua_State* L, F& f, int, std::false_type)
    {
        return invoke_with(
            L, f, ResultConverter(), default_constructed<ArgConverters>()...);
    }

    static int call_with_stored_converters( // Stored convertes:
        lua_State* L, F& f, int up_idx, std::true_type)
    {
        auto& stored_converters = *static_cast<tuple_t*>(
            lua_touserdata(L, lua_upvalueindex(up_idx)));
        return invoke_with_tuple(L, f, stored_converters);
    }
};

template <typename F, typename ResultConverter, typename... ArgConverters>
struct converted_function {
public:

    using dispatch_t = function_dipatcher<
        F, ResultConverter, ArgConverters...>;
    using tuple_t = typename dispatch_t::tuple_t;

    tuple_t converters;

    template <
        typename FArg,
        typename = typename std::enable_if<
            std::is_convertible<FArg, F>::value>::type,
        typename... AllConverters>
    converted_function(FArg&& f_, init_fn_tag, AllConverters&&... converters_)
        : converters(std::forward<AllConverters>(converters_)...)
        , m_f(std::forward<FArg>(f_))
    {}

    static int entry_point(lua_State* L) BOOST_NOEXCEPT
    {
        return entry_point_impl(L, is_light_function<F>());
    }

    F& fn() { return m_f; }

private:
    F m_f;

    // Non-light function:
    static int entry_point_impl(lua_State* L, std::false_type) BOOST_NOEXCEPT
    {
        auto& f = *static_cast<F*>(
            lua_touserdata(L, lua_upvalueindex(fn_upval_fn)));
        return call(L, f);
    }

    // Light function:
    static int entry_point_impl(lua_State* L, std::true_type) BOOST_NOEXCEPT
    {
        auto voidholder = lua_touserdata(L, lua_upvalueindex(fn_upval_fn));
        auto f = reinterpret_cast<light_function_holder<F>&>(voidholder).f;
        return call(L, f);
    }

    static int call(lua_State* L, F& f) BOOST_NOEXCEPT
    {
        return dispatch_t::call(L, f, fn_upval_converters);
    }
};


} // namespace detail

template <typename F, typename ResultConverter, typename... ArgConverters>
detail::converted_function<
    typename detail::remove_qualifiers<F>::type,
    typename detail::remove_qualifiers<ResultConverter>::type,
    typename detail::remove_qualifiers<ArgConverters>::type...>
make_function_with(
    F&& f, ResultConverter&& rconv, ArgConverters&&... aconvs)
{
    return {
        std::forward<F>(f),
        detail::init_fn_tag(),
        std::forward<ResultConverter>(rconv),
        std::forward<ArgConverters>(aconvs)...};
}

namespace detail {

template <typename F, int... Is>
auto make_function_impl(F&& f, iseq<Is...>)
-> decltype(make_function_with(
    std::forward<F>(f), std::move(std::get<Is>(default_converters(f)))...))
{
    auto def_converters = default_converters(f);
    return make_function_with(
        std::forward<F>(f), std::move(std::get<Is>(def_converters))...);
}

} // namespace detail

template <typename F>
auto make_function(F&& f)
-> decltype(make_function_impl(
    std::forward<F>(f),
    detail::tuple_seq<decltype(detail::default_converters(f))>()))
{
    return make_function_impl(
        std::forward<F>(f),
        detail::tuple_seq<decltype(detail::default_converters(f))>());
}


template<typename F, typename ResultConverter, typename... Converters>
struct converter<detail::converted_function<F, ResultConverter, Converters...>>
    : converter_base<
        detail::converted_function<F, ResultConverter, Converters...>> {
public:
    using type = detail::converted_function<F, ResultConverter, Converters...>;

    static void push(lua_State* L, type&& f)
    {
        push_impl(L, std::move(f.fn()), detail::is_light_function<F>());
        static_assert(detail::fn_upval_fn == 1, "");
        detail::push_function_tag(L);
        static_assert(detail::fn_upval_tag == 2, "");
        lua_pushlightuserdata(L, const_cast<boost::typeindex::type_info*>(
            &boost::typeindex::type_id<F>().type_info()));
        static_assert(detail::fn_upval_type == 3, "");

        int nups = 3;
        static_assert(detail::fn_upval_converters == 4, "");
        if (type::dispatch_t::push_converters(L, std::move(f.converters)))
            ++nups;

        lua_pushcclosure(L, &f.entry_point, nups);
    }

private:
    // Nonlight function
    static void push_impl(lua_State* L, F&& f, std::false_type)
    {
        push_gc_object(L, std::move(f));
    }

    // Light function
    static void push_impl(lua_State* L, F const& f, std::true_type)
    {
        detail::light_function_holder<F> holder{f};
        lua_pushlightuserdata(L, reinterpret_cast<void*&>(holder));
    }
};

template <typename T, typename... Args>
T ctor_wrapper(Args... args)
{
    return T(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
T* new_wrapper(Args... args)
{
    return new T(std::forward<Args>(args)...);
}

} // namespace apollo

#endif // APOLLO_MAKE_FUNCTION_HPP_INCLUDED
