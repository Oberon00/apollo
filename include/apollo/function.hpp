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

#include <boost/exception/errinfo_type_info_name.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/function.hpp>
#include <lua.hpp>

#include <type_traits>

namespace apollo {

namespace detail {

template <typename R, typename... Args, int... Is>
R call_with_stack_args_impl(lua_State* L, detail::iseq<Is...>, R(*f)(Args...))
{
    (void)L; // Avoid MSVC's complainining when Args is empty.
    return f(from_stack<Args>(L, Is)...);
}

template <typename R, typename C, typename... Args, int... Is>
R call_with_stack_args_impl(
    lua_State* L, detail::iseq<Is...>,
    R(C::*f)(Args...))
{
    return (from_stack<C&>(L, 1).*f)(from_stack<Args>(L, Is)...);
}

template <typename R, template<class> class FObj, typename... Args, int... Is>
R call_with_stack_args_impl(
    lua_State* L, detail::iseq<Is...>,
    FObj<R(Args...)> const& f)
{
    (void)L; // Avoid MSVC's complainining when Args is empty.
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
        L, detail::iseq_n_t<sizeof...(Args), 2>(), f);
}

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
int call_with_stack_args_and_push_lerror(lua_State* L, F&& f)  BOOST_NOEXCEPT
{
    return exceptions_to_lua_errors_L(
        L, &call_with_stack_args_and_push<F>, std::forward<F>(f));
}

template <typename F, F FVal>
static int static_entry_point(lua_State* L) BOOST_NOEXCEPT
{
    return call_with_stack_args_and_push_lerror(L, FVal);
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
        return call_with_stack_args_and_push_lerror(L, f);
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
        return call_with_stack_args_and_push_lerror(L, f);
    }
};

template <typename FType>
struct mem_fn_ptr_holder { FType val; };

template <typename R, typename C, typename... Args>
struct function_dispatch<R(C::*)(Args...)>
{
private:
    using FType = R(C::*)(Args...);

public:
    static void push_upvalue(lua_State* L, FType f)
    {
        push_gc_object(L, mem_fn_ptr_holder<FType>{ f });
    }

    static int entry_point(lua_State* L) BOOST_NOEXCEPT
    {
        auto f = static_cast<mem_fn_ptr_holder<FType>*>(
            lua_touserdata(L, lua_upvalueindex(1)))->val;
        return call_with_stack_args_and_push_lerror(L, f);
    }
};


template <typename F>
void push_function(lua_State* L, F const& f)
{
    using fdispatch = detail::function_dispatch<F>;
    fdispatch::push_upvalue(L, f);
    lua_pushcclosure(L, &fdispatch::entry_point, 1);
}

// function_converter //

template <typename F, typename Enable=void>
struct function_converter;

template <template<class> class FObj, typename R, typename... Args>
struct function_converter<FObj<R(Args...)>> {
    using FType = FObj<R(Args...)>;

    static FType from_stack(lua_State* L, int idx)
    {
        // Exact same type in Lua? Then copy.
        lua_CFunction thunk = lua_tocfunction(L, idx);
        if (thunk == &function_dispatch<FType>::entry_point) {
            stack_balance balance(L);
            BOOST_VERIFY(lua_getupvalue(L, idx, 1));
            BOOST_ASSERT(lua_isuserdata(L, -1));
            return *static_cast<FType*>(lua_touserdata(L, -1));;
        }

        // Plain function pointer in Lua? Then construct from it.
        using plainfconv = function_converter<R(*)(Args...)>;
        if (plainfconv::n_conversion_steps(L, idx) != no_conversion)
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
        detail::is_plain_function<F>::value>::type>
{
    using FType = F;
    static FType from_stack(lua_State* L, int idx)
    {
        stack_balance balance(L);
        BOOST_VERIFY(lua_getupvalue(L, idx, 1));
        BOOST_ASSERT(lua_islightuserdata(L, -1));
        return reinterpret_cast<FType>(lua_touserdata(L, -1));
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        lua_CFunction thunk = lua_tocfunction(L, idx);
        return thunk == &function_dispatch<F>::entry_point ? 0 : no_conversion;
    }
};

template <typename F>
struct function_converter<F, typename std::enable_if<
        std::is_member_function_pointer<F>::value>::type>
{
    using FType = F;

    // TODO: Nearly duplicate of plain function converter
    static FType from_stack(lua_State* L, int idx)
    {
        stack_balance balance(L);
        BOOST_VERIFY(lua_getupvalue(L, idx, 1));
        BOOST_ASSERT(lua_type(L, -1) == LUA_TUSERDATA);
        return static_cast<mem_fn_ptr_holder<F>*>(
            lua_touserdata(L, -1))->val;
    }

    // TODO: Duplicate of plain function converter
    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        lua_CFunction thunk = lua_tocfunction(L, idx);
        return thunk == &function_dispatch<F>::entry_point ? 0 : no_conversion;
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
        detail::lua_type_id<T>::value == LUA_TFUNCTION>::type>: converter_base<T> {

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

    static typename fconverter::FType from_stack(lua_State* L, int idx)
    {
        return fconverter::from_stack(L, idx);
    }
};


} // namespace apollo

#endif // APOLLO_FUNCTION_HPP_INCLUDED
