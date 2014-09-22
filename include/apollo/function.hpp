#ifndef APOLLO_FUNCTION_HPP_INCLUDED
#define APOLLO_FUNCTION_HPP_INCLUDED APOLLO_FUNCTION_HPP_INCLUDED

#include <functional>
#include <boost/function.hpp>

#include <lua.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/exception/errinfo_type_info_name.hpp>
#include <type_traits>

#ifdef BOOST_MSVC
#    include <boost/function.hpp>
#endif

#include <apollo/lapi.hpp>
#include <apollo/error.hpp>
#include <apollo/converters.hpp>
#include <apollo/config.hpp>
#include <apollo/gc.hpp>
#include <apollo/reference.hpp>
#include <apollo/stack_balance.hpp>
#include <apollo/detail/integer_seq.hpp>

namespace apollo {

namespace detail {

template <typename F, typename Enable=void>
struct function_converter;

template <typename F>
struct unmember_function;

template <typename R, typename C, typename... Args>
struct unmember_function<R(C::*)(Args...)> {
    typedef R(type)(C&, Args...);
};

template <typename R, typename C, typename... Args>
struct unmember_function<R(C::*)(Args...) const> {
    typedef R(type)(C const&, Args...);
};

template <typename F>
using unmember_function_t = typename unmember_function<F>::type;

template <typename R, typename... Args, int... Is>
R call_with_stack_args_impl(lua_State* L, detail::iseq<Is...>, R(*f)(Args...))
{
    return f(from_stack<Args>(L, Is)...);
}

template <typename R, typename C, typename... Args, int... Is>
R call_with_stack_args_impl(
    lua_State* L, detail::iseq<Is...>,
    R(C::*f)(Args...))
{
    return from_stack<C>.*f(from_stack<Args>(L, Is)...);
}

template <typename R, template<class> class FObj, typename... Args, int... Is>
R call_with_stack_args_impl(
    lua_State* L, detail::iseq<Is...>,
    FObj<R(Args...)> const& f)
{
    return f(from_stack<Args>(L, Is)...);
}

} // namespace detail

template <typename R, typename... Args>
R call_with_stack_args(lua_State* L, R(*f)(Args...))
{
    return detail::call_with_stack_args_impl(
        L, detail::iseq_n_t<sizeof...(Args)>(), f);
}

template <typename R, template<class> class FObj, typename... Args>
R call_with_stack_args(lua_State* L, FObj<R(Args...)> const& f)
{
    return detail::call_with_stack_args_impl(
        L, detail::iseq_n_t<sizeof...(Args)>(), f);
}

template <class C, typename R, typename... Args>
R call_with_stack_args(lua_State* L, R(C::*f)(Args...))
{
    return detail::call_with_stack_args_impl(
        L, detail::iseq_n_t<sizeof...(Args)>(), f);
}

namespace detail {

inline void push_args(lua_State*)
{
    // Push zero args: NOP.
}

template <typename Head, typename... Tail>
void push_args(lua_State* L, Head&& arg, Tail&&... tail)
{
    push(L, std::forward<Head>(arg));
    push_args(L, std::forward<Tail>(tail)...);
}

// Doesn't work with typename here.
template <template<class> class FObj, typename R, typename... Args>
struct function_converter<FObj<R(Args...)>> {
    typedef FObj<R(Args...)> FType;
    static FType from_stack(lua_State* L_, int idx)
    {
        registry_reference luaFunction(
            L_, idx, registry_reference::ref_mode::copy);
        return [luaFunction](Args... args) -> R {
            lua_State* L = luaFunction.L();
            stack_balance b(L);
            luaFunction.push();
            push_args(L, std::forward<Args>(args)...);
            pcall(L, sizeof...(Args), 1);
            R r = apollo::from_stack<R>(L, -1);
            return r;
        };
    }
};

} // namespace detail

namespace detail {

template <typename F> // f returns void
int call_with_stack_args_and_push_impl(lua_State* L, F&& f, std::true_type)
{
    call_with_stack_args(L, std::forward<F>(f));
    return 0;
}

template <typename F> // f returns non-void
int call_with_stack_args_and_push_impl(lua_State* L, F&& f, std::false_type)
{
    push(L, call_with_stack_args(L, std::forward<F>(f)));
    return 1;
}

template <typename F>
int call_with_stack_args_and_push(lua_State* L, F&& f)
{
    return call_with_stack_args_and_push_impl(
        L, std::forward<F>(f),
        typename std::is_void<decltype(
            call_with_stack_args(L, std::forward<F>(f)))>::type());
}

template <typename F>
int exceptions_to_lua_errors(lua_State* L, F&& f) BOOST_NOEXCEPT
{
    int arg = 0;
    try {
        return call_with_stack_args_and_push(L, std::forward<F>(f));
    } catch(to_cpp_conversion_error const& e) {
        // TODO: Check for nullptr.
        arg = *boost::get_error_info<errinfo::stack_index>(e);
        lua_pushfstring(L, "%s [%s -> %s]",
            boost::get_error_info<errinfo::msg>(e)->c_str(),
            luaL_typename(L, arg),
            boost::get_error_info<boost::errinfo_type_info_name>(e)->c_str());
    } catch(std::exception const& e) {
        lua_pushfstring(L, "exception: %s", e.what());
    } catch(...) {
        lua_pushliteral(L, "unknown exception");
    }
    if (arg)
        return luaL_argerror(L, arg, lua_tostring(L, -1));
    return lua_error(L);
}


template <typename F, typename Enable=void>
struct function_dispatch {
    static void push_upvalue(lua_State* L, F const& f)
    {
        push_gc_object(L, f);
    }

    static int entry_point(lua_State* L) BOOST_NOEXCEPT
    {
        auto& f = *static_cast<F*>(lua_touserdata(L, lua_upvalueindex(1)));
        return exceptions_to_lua_errors(L, f);
    }
};

template <typename F>
struct function_dispatch<F, typename std::enable_if<
        detail::is_plain_function<F>::value>::type> {

    static void push_upvalue(lua_State* L, F f)
    {
        lua_pushlightuserdata(L, reinterpret_cast<void*>(f));
    }

    static int entry_point(lua_State* L) BOOST_NOEXCEPT
    {
        auto f = reinterpret_cast<F>(lua_touserdata(L, lua_upvalueindex(1)));
        return exceptions_to_lua_errors(L, f);
    }
};

    template <typename F>
    using mem_fn_wrapper =
#ifdef BOOST_MSVC
        boost::function<F>;
#else
        std::function<F>;
#endif


template <typename F>
struct function_dispatch<F, typename std::enable_if<
        std::is_member_function_pointer<F>::value>::type>
        : function_dispatch<detail::mem_fn_wrapper<unmember_function_t<F>>>
{};

template <typename F>
void pushFunction(lua_State* L, F const& f)
{
    typedef detail::function_dispatch<F> fdispatch;
    fdispatch::push_upvalue(L, f);
    lua_pushcclosure(L, &fdispatch::entry_point, 1);
}

template <typename F>
struct function_converter<F, typename std::enable_if<
        detail::is_plain_function<F>::value>::type>:
        function_converter<std::function<typename std::remove_pointer<F>::type>>
{ };

template <typename F>
struct function_converter<F, typename std::enable_if<
        std::is_member_function_pointer<F>::value>::type>:
        function_converter<std::function<unmember_function_t<F>>>
{ };

} // namespace detail

// Function converter //
template<typename T>
struct converter<T, typename std::enable_if<
        detail::lua_type_id<T>::value == LUA_TFUNCTION>::type>: converter_base<T> {

private:
    typedef detail::function_converter<T> fconverter;

public:
    static void push(lua_State* L, T const& f)
    {
        detail::pushFunction(L, f);
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        if (lua_isfunction(L, idx))
            return 0;
        if (luaL_getmetafield(L, idx, "__call")) {
            lua_pop(L, 1);
            return 1;
        }
        return no_conversion;
    }

    static typename fconverter::FType from_stack(lua_State* L, int idx)
    {
        return fconverter::from_stack(L, idx);
    }
};


} // namespace apollo

#endif // APOLLO_FUNCTION_HPP_INCLUDED
