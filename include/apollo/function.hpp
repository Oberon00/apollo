#ifndef APOLLO_FUNCTION_HPP_INCLUDED
#define APOLLO_FUNCTION_HPP_INCLUDED APOLLO_FUNCTION_HPP_INCLUDED

#include <apollo/config.hpp>
#include <apollo/converters.hpp>
#include <apollo/error.hpp>
#include <apollo/gc.hpp>
#include <apollo/lapi.hpp>
#include <apollo/reference.hpp>
#include <apollo/stack_balance.hpp>
#include <apollo/detail/integer_seq.hpp>
#include <apollo/detail/ref_binder.hpp>

#include <boost/exception/errinfo_type_info_name.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/function.hpp>
#include <lua.hpp>

#include <tuple>
#include <type_traits>
#include <typeinfo>

namespace apollo {

namespace detail {

std::pair<bool, std::type_info const*> function_type(lua_State* L, int idx);
bool is_light_function(lua_State* L, int idx);
void push_function_tag(lua_State* L, bool is_light);

int const
    fn_upval_fn = 1,
    fn_upval_tag = 2,
    fn_upval_type = 3,
    fn_upval_converters = 4;


template <typename Tuple>
using tuple_seq = iseq_n_t<std::tuple_size<Tuple>::value>;

template<bool... Bs> // http://stackoverflow.com/a/24687161/2128694
struct bool_and: std::is_same<iseq<Bs...>, iseq<(Bs || true)... >> {};

template <typename... Ts>
struct all_empty: bool_and<std::is_empty<Ts>::value...> {};

inline std::tuple<> from_stack_as_tuple(lua_State*, int)
{
    return {};
}

// MSVC Workaround
template <typename T>
T default_constructed()
{
    return {};
}

template <typename Converter0, typename... Converters>
std::tuple<to_type_of<Converter0>, to_type_of<Converters>...>
from_stack_as_tuple(
    lua_State* L, int i, Converter0&& conv0, Converters&&... convs)
{
    // Keep these statements separate to make sure the
    // recursive invocation receives the updated i.
    std::tuple<to_type_of<Converter0>> arg0(
        from_stack_with(std::forward<Converter0>(conv0), L, i, &i));
    static_assert(std::tuple_size<decltype(arg0)>::value == 1, "");
    return std::tuple_cat(std::move(arg0), from_stack_as_tuple(
        L, i, std::forward<Converters>(convs)...));
}

// Plain function pointer:
template <typename R, typename... Args>
std::tuple<push_converter_for<R>, pull_converter_for<Args>...>
default_converters(R(*)(Args...))
{
    return {};
}

// Function object (std::function, boost::function, etc.):
template <typename R, template<class> class FObj, typename... Args>
std::tuple<push_converter_for<R>, pull_converter_for<Args>...>
default_converters(FObj<R(Args...)> const&)
{
    return {};
}

// Member function pointer:
template <class C, typename R, typename... Args>
std::tuple<
    push_converter_for<R>,
    pull_converter_for<C&>,
    pull_converter_for<Args>...>
default_converters(R(C::*)(Args...))
{
    return {};
}

// Const member function pointer:
template <class C, typename R, typename... Args>
std::tuple<
    push_converter_for<R>,
    pull_converter_for<C const&>,
    pull_converter_for<Args>...>
default_converters(R(C::*)(Args...) const)
{
    return {};
}

// Plain function pointer:
template <typename R, typename... Args>
R return_type_of_impl(R(*)(Args...));

// Function object (std::function, boost::function, etc.):
template <template <class> class FObj, typename R, typename... Args>
R return_type_of_impl(FObj<R(Args...)> const&);

// Member function pointer:
template <class C, typename R, typename... Args>
R return_type_of_impl(R(C::*)(Args...));

// Const member function pointer:
template <class C, typename R,  typename... Args>
R return_type_of_impl(R(C::*)(Args...) const);

template <typename F>
using return_type_of = decltype(return_type_of_impl(std::declval<F>()));

// Plain function pointer or function object:
template <typename F, int... Is, typename... Converters>
typename std::enable_if<!detail::is_mem_fn<F>::value, return_type_of<F>>::type
call_with_stack_args_impl(
    lua_State* L, F&& f,
    detail::iseq<Is...>,
    Converters&&... convs
)
{
    auto args = from_stack_as_tuple(L, 1, std::forward<Converters>(convs)...);
    static_assert(std::tuple_size<decltype(args)>::value == sizeof...(Is), "");
    return f(unwrap_bound_ref(std::get<Is>(args))...);
}

// (Const) member function pointer:
template <
    typename ThisConverter, typename... Converters, int... Is, typename F>
typename std::enable_if<detail::is_mem_fn<F>::value, return_type_of<F>>::type
call_with_stack_args_impl(
    lua_State* L, F&& f,
    detail::iseq<Is...>,
    ThisConverter&& this_conv,
    Converters&&... convs
)
{
    int i0;
    to_type_of<ThisConverter> instance = from_stack_with(
        this_conv, L, 1, &i0);
    auto args = from_stack_as_tuple(L, i0, std::forward<Converters>(convs)...);
    return (unwrap_bound_ref(instance).*f)(
        unwrap_bound_ref(std::get<Is>(args))...);
}

} // namespace detail

template <typename F, typename... Converters>
detail::return_type_of<F> call_with_stack_args_with(
    lua_State* L, F&& f, Converters&&... converters)
{
    auto arg_seq = detail::iseq_n_t
        <sizeof...(Converters) - detail::is_mem_fn<F>::value>();
    return detail::call_with_stack_args_impl(
        L, std::forward<F>(f),
        arg_seq,
        std::forward<Converters>(converters)...);
}

namespace detail {

template <typename F, int... Is>
return_type_of<F> call_with_stack_args_def(lua_State* L, F&& f, iseq<Is...>)
{
    return call_with_stack_args_with(
        L, f,
        std::move(std::get<Is>(default_converters(f)))...);
}

} // namespace detail

// Plain function pointer:
template <typename F>
detail::return_type_of<F> call_with_stack_args(lua_State* L, F&& f)
{
    using all_converter_tuple = decltype(detail::default_converters(f));
    return detail::call_with_stack_args_def(
        L, f,
        // Skip result converter tuple:
        detail::iseq_n_t<std::tuple_size<all_converter_tuple>::value, 1>());
}

namespace detail {

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
int call_with_stack_args_and_push_lerror(
    lua_State* L, F&& f,
    ResultConverter&& rconverter, Converters&&... converters)  BOOST_NOEXCEPT
{
    return exceptions_to_lua_errors_L(
        L, &call_with_stack_args_and_push<F, ResultConverter, Converters...>,
        std::forward<F>(f),
        std::forward<ResultConverter>(rconverter),
        std::forward<Converters>(converters)...);
}

struct init_fn_tag {}; // To avoid being as for move/copy ctor.

template <typename F, typename ResultConverter, typename... ArgConverters>
struct converted_function {
public:
    using stores_converters_t = std::integral_constant<bool,
        !all_empty<ResultConverter, ArgConverters...>::value>;
    static bool const stores_converters = stores_converters_t::value;
    using fn_is_light_t = std::integral_constant<bool,
        detail::is_plain_function<F>::value>;
    static bool const fn_is_light = fn_is_light_t::value;

    using tuple_t = std::tuple<ResultConverter, ArgConverters...>;

    tuple_t converters;
    F f;

    template <
        typename FArg,
        typename = typename std::enable_if<
            std::is_convertible<FArg, F>::value>::type,
        typename... AllConverters>
    explicit converted_function(FArg&& f_, init_fn_tag, AllConverters&&... converters)
        : converters(std::forward<AllConverters>(converters)...)
        , f(std::forward<FArg>(f_))
    {}

    static int entry_point(lua_State* L) BOOST_NOEXCEPT
    {
        return entry_point_impl(L, fn_is_light_t());
    }

private:

    template <int... Is> // No stored convertes:
    static int call_with_stored_converters(
        lua_State* L, F& f, iseq<Is...>, std::false_type)
    {
        return call_with_stack_args_and_push_lerror(
            L, f, ResultConverter(), default_constructed<ArgConverters>()...);
    }

    template <int... Is> // Stored convertes:
    static int call_with_stored_converters(
        lua_State* L, F& f, iseq<Is...>, std::true_type)
    {
        auto& stored_converters = *static_cast<tuple_t*>(
            lua_touserdata(L, lua_upvalueindex(fn_upval_converters)));
        return call_with_stack_args_and_push_lerror(
            L, f, std::get<Is>(stored_converters)...);
    }

    // Non-light function:
    static int entry_point_impl(lua_State* L, std::false_type) BOOST_NOEXCEPT
    {
        auto& f = *static_cast<F*>(
            lua_touserdata(L, lua_upvalueindex(fn_upval_fn)));
        return call_with_stored_converters(
            L, f, tuple_seq<tuple_t>(), stores_converters_t());
    }

    // Light function:
    static int entry_point_impl(lua_State* L, std::true_type) BOOST_NOEXCEPT
    {
        auto f = reinterpret_cast<F>(
            lua_touserdata(L, lua_upvalueindex(fn_upval_fn)));
        return call_with_stored_converters(
            L, f, tuple_seq<tuple_t>(), stores_converters_t());
    }
};

} // namespace detail

template <typename F, typename ResultConverter, typename... ArgConverters>
detail::converted_function<
    typename detail::remove_qualifiers<F>::type,
    typename detail::remove_qualifiers<ResultConverter>::type,
    typename detail::remove_qualifiers<ArgConverters>::type...>
make_funtion_with(
    F&& f, ResultConverter&& rconv, ArgConverters&&... aconvs)
{
    return detail::converted_function<
            typename detail::remove_qualifiers<F>::type,
            typename detail::remove_qualifiers<ResultConverter>::type,
            typename detail::remove_qualifiers<ArgConverters>::type...>(
        std::forward<F>(f),
        detail::init_fn_tag(),
        std::forward<ResultConverter>(rconv),
        std::forward<ArgConverters>(aconvs)...);
}

template<typename F, typename ResultConverter, typename... Converters>
struct converter<detail::converted_function<F, ResultConverter, Converters...>>
    : converter_base<
        detail::converted_function<F, ResultConverter, Converters...>> {
public:
    using type = detail::converted_function<F, ResultConverter, Converters...>;

    static void push(lua_State* L, type&& f)
    {
        push_impl(L, std::move(f.f), typename type::fn_is_light_t());
        static_assert(detail::fn_upval_fn == 1, "");
        detail::push_function_tag(L, type::fn_is_light);
        static_assert(detail::fn_upval_tag == 2, "");
        lua_pushlightuserdata(L, const_cast<std::type_info*>(&typeid(F)));
        static_assert(detail::fn_upval_type == 3, "");
        int nups = 3;
#ifdef BOOST_MSVC
#   pragma warning(push)
#   pragma warning(disable:4127) // Conditional expression is constant.
#endif
        if (type::stores_converters) {
#ifdef BOOST_MSVC
#   pragma warning(pop)
#endif
            push_gc_object(L, std::move(f.converters));
            static_assert(detail::fn_upval_converters == 4, "");
            ++nups;
        }
        lua_pushcclosure(L, &f.entry_point, nups);
    }

private:
    static void push_impl(lua_State* L, F&& f, std::false_type)
    {
        push_gc_object(L, std::move(f));
    }

    static void push_impl(lua_State* L, F const& f, std::true_type)
    {
        lua_pushlightuserdata(L, reinterpret_cast<void*>(f));
    }
};

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

template <typename F, int... Is>
void push_function_impl(lua_State* L, F&& f, iseq<Is...>)
{
    auto def_converters = default_converters(f);
    push(L, make_funtion_with(
        std::forward<F>(f), std::move(std::get<Is>(def_converters))...));
}

template <typename F>
void push_function(lua_State* L, F&& f)
{
   push_function_impl(
       L, std::forward<F>(f), tuple_seq<decltype(default_converters(f))>());
}

// function_converter //

template <typename F, typename Enable=void>
struct function_converter;

template <template<class> class FObj, typename R, typename... Args>
struct function_converter<FObj<R(Args...)>> {
    using type = FObj<R(Args...)>;

    static type from_stack(lua_State* L, int idx)
    {
        auto const& fty = function_type(L, idx);
        if (*fty.second == typeid(type)) {
            BOOST_ASSERT(!fty.first);
            stack_balance balance(L);
            BOOST_VERIFY(lua_getupvalue(L, idx, detail::fn_upval_fn));
            BOOST_ASSERT(lua_isuserdata(L, -1));
            return *static_cast<type*>(lua_touserdata(L, -1));
        }

        // Plain function pointer in Lua? Then construct from it.
        using plainfconv = function_converter<R(*)(Args...)>;
        if (*fty.second == typeid(typename plainfconv::type))
            return plainfconv::from_stack(L, idx);

        // TODO?: optimization: Before falling back to the pcall lambda,
        // try boost::function and std::function.

        registry_reference luaFunction(L, idx, ref_mode::copy);
        return [luaFunction](Args... args) -> R {
            lua_State* L_ = luaFunction.L();
            stack_balance b(L_);
            luaFunction.push();
            // Use push_impl to allow empty Args.
            push_impl(L_, std::forward<Args>(args)...);
            pcall(L_, sizeof...(Args), 1);
            return apollo::from_stack<R>(L_, -1);
        };
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        if (lua_isfunction(L, idx))
            return 0;
        if (luaL_getmetafield(L, idx, "__call")) { // TODO: pcall
            lua_pop(L, 1);
            return 2;
        }
        return no_conversion;
    }
};

template <typename F>
struct function_converter<F, typename std::enable_if<
        detail::is_plain_function<F>::value ||
        std::is_member_function_pointer<F>::value>::type>
{
    using type = F;

    using is_plain = detail::is_plain_function<F>;

    static typename std::conditional<
        detail::is_plain_function<F>::value, F, F&>::type
    from_stack(lua_State* L, int idx)
    {
        stack_balance balance(L);
        bool const is_light =
               is_plain::value
            && detail::is_light_function(L, idx);
        BOOST_VERIFY(lua_getupvalue(L, idx, fn_upval_fn));
        BOOST_ASSERT(lua_isuserdata(L, -1));
        void* ud = lua_touserdata(L, -1);
        return from_stack_impl(is_light, ud, is_plain());
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        return *function_type(L, idx).second == typeid(type);
    }

private:
    // Plain function can be light:
    static F from_stack_impl(bool is_light, void* ud, std::true_type)
    {
        if (is_light)
            return reinterpret_cast<F>(ud);
        return from_stack_impl(is_light, ud, std::false_type());
    }

    // Non-plain functions are never light:
    static F& from_stack_impl(bool, void* ud, std::false_type)
    {
        return *static_cast<F*>(ud);
    }
};

} // namespace detail

template <typename F, F FVal>
BOOST_CONSTEXPR static raw_function to_raw_function() BOOST_NOEXCEPT
{
    return &detail::static_entry_point<F, FVal>;
}

#define APOLLO_TO_RAW_FUNCTION(f) apollo::to_raw_function<decltype(f), f>()
#define APOLLO_PUSH_FUNCTION_STATIC(L, f) \
    lua_pushcfunction(L, APOLLO_TO_RAW_FUNCTION(f))

template <typename T, typename... Args>
T ctor_wrapper(Args... args)
{
    return T(std::forward<Args>(args)...);
}

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


// Function converter //
template<typename T>
struct converter<T, typename std::enable_if<
        detail::lua_type_id<T>::value == LUA_TFUNCTION>::type>
    : converter_base<T> {

private:
    using fconverter = detail::function_converter<T>;

public:
    static void push(lua_State* L, T const& f)
    {
        detail::push_function(L, f);
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        return fconverter::n_conversion_steps(L, idx);
    }

    static typename fconverter::type from_stack(lua_State* L, int idx)
    {
        return fconverter::from_stack(L, idx);
    }
};

} // namespace apollo

#endif // APOLLO_FUNCTION_HPP_INCLUDED
