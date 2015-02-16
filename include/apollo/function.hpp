#ifndef APOLLO_FUNCTION_HPP_INCLUDED
#define APOLLO_FUNCTION_HPP_INCLUDED APOLLO_FUNCTION_HPP_INCLUDED

#include <apollo/config.hpp>
#include <apollo/lapi.hpp>
#include <apollo/make_function.hpp>
#include <apollo/reference.hpp>
#include <apollo/stack_balance.hpp>

#include <boost/type_index.hpp>

namespace apollo {

namespace detail {

APOLLO_API boost::typeindex::type_info const& function_type(
    lua_State* L, int idx);

// function_converter //

template <typename F, typename Enable=void>
struct function_converter;

template <template<class> class FObj, typename R, typename... Args>
struct function_converter<FObj<R(Args...)>> {
    using type = FObj<R(Args...)>;

    static type to(lua_State* L, int idx)
    {
        auto const& fty = function_type(L, idx);
        if (fty == boost::typeindex::type_id<type>()) {
            stack_balance balance(L);
            BOOST_VERIFY(lua_getupvalue(L, idx, detail::fn_upval_fn));
            BOOST_ASSERT(lua_type(L, -1) == LUA_TUSERDATA); // Not light!
            return *static_cast<type*>(lua_touserdata(L, -1));
        }

        // Plain function pointer in Lua? Then construct from it.
        using plainfconv = function_converter<R(*)(Args...)>;
        if (fty == boost::typeindex::type_id<typename plainfconv::type>())
            return plainfconv::to(L, idx);

        // TODO?: optimization: Before falling back to the pcall lambda,
        // try boost::function and std::function.

        registry_reference luaFunction(L, idx, ref_mode::copy);
        return [luaFunction](Args... args) -> R {
            lua_State* L_ = luaFunction.L();
            stack_balance b(L_);
            int const oldtop = lua_gettop(L_);
            luaFunction.push();
            int const n_args = push_impl(L_, std::forward<Args>(args)...);
            pcall(L_, n_args, LUA_MULTRET);
            return apollo::to<R>(L_, oldtop + 1);
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

    using is_light = is_light_function<F>;

    static F to(lua_State* L, int idx)
    {
        stack_balance balance(L);
        BOOST_VERIFY(lua_getupvalue(L, idx, fn_upval_fn));
        BOOST_ASSERT(lua_isuserdata(L, -1));
        void* ud = lua_touserdata(L, -1);
        return to_impl(ud, is_light());
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        return function_type(L, idx) == boost::typeindex::type_id<type>() ?
            0 : no_conversion;
    }

private:
    // Light function:
    static F to_impl(void* ud, std::true_type)
    {
        return reinterpret_cast<light_function_holder<F>&>(ud).f;
    }

    // Non-light function:
    static F& to_impl(void* ud, std::false_type)
    {
        return *static_cast<F*>(ud);
    }
};

} // namespace detail


// Function converter //
template<typename T>
struct converter<T, typename std::enable_if<
        detail::lua_type_id<T>::value == LUA_TFUNCTION>::type>
    : converter_base<T> {

private:
    // Work around a MSVC 12 (2013) bug, where
    // decltype(&fn_templateX<>) == decltype(fn_template<T>)
    // (another workaround would be to wrap the & in a function).
    using fn_t = typename std::conditional<
        std::is_function<T>::value, T*, T>::type;

    using fconverter = detail::function_converter<fn_t>;

public:
    static int push(lua_State* L, fn_t const& f)
    {
        apollo::push(L, make_function(f));
        return 1;
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        return fconverter::n_conversion_steps(L, idx);
    }

    static typename fconverter::type to(lua_State* L, int idx)
    {
        return fconverter::to(L, idx);
    }
};

} // namespace apollo

#endif // APOLLO_FUNCTION_HPP_INCLUDED
