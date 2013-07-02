#ifndef CPPLUA_FUNCTION_HPP_INCLUDED
#define CPPLUA_FUNCTION_HPP_INCLUDED CPPLUA_FUNCTION_HPP_INCLUDED

#include <functional>
#include <boost/function.hpp>

#include <lua.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/exception/get_error_info.hpp>
#include <boost/exception/errinfo_type_info_name.hpp>
#include <type_traits>

#include "error.hpp"
#include "converters.hpp"
#include "config.hpp"
#include "gc.hpp"

namespace cpplua {

namespace detail {

template <typename F>
int callWithStackArgsAndPush(lua_State* L, F&& f)
{
    push(L, callWithStackArgs(L, std::forward<F>(f)));
    return 1;
}

template <typename F>
int exceptionsToLuaErrors(lua_State* L, F&& f)
{
    int arg = 0;
    try {
        return callWithStackArgsAndPush(L, std::forward<F>(f));
    } catch(ToCppConversionError const& e) {
        // TODO: Check for nullptr.
        arg = *boost::get_error_info<errinfo::StackIndex>(e);
        lua_pushfstring(L, "%s [%s -> %s]",
            boost::get_error_info<errinfo::Msg>(e)->c_str(),
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
struct FunctionDispatch;

template <typename F>
struct FunctionDispatch<F, typename std::enable_if<
        std::is_member_function_pointer<F>::value ||
        std::is_pointer<F>::value &&
        std::is_function<
            typename std::remove_pointer<F>::type>::value>::type> {
    
    static void pushUpvalue(lua_State* L, F f)
    {
        lua_pushlightuserdata(L, f);
    }

    static int entryPoint(lua_State* L)
    {
        auto f = static_cast<F>(lua_touserdata(L, lua_upvalueindex(1)));
        return exceptionsToLuaErrors(L, f);
    }
};

template <typename F>
struct FunctionObjectDispatch {
    static void pushUpvalue(lua_State* L, F const& f)
    {
        pushGcObject(L, f);
    }

    static int entryPoint(lua_State* L)
    {
        auto& f = *static_cast<F*>(lua_touserdata(L, lua_upvalueindex(1)));
        return exceptionsToLuaErrors(L, f);
    }
};

template <typename F>
void pushFunction(lua_State* L, F const& f)
{
    typedef detail::FunctionDispatch<F> FDispatch;
    FDispatch::pushUpvalue(L, f);
    lua_pushcclosure(L, &FDispatch::entryPoint, 1);
}

} // namespace detail

#define BOOST_PP_ITERATION_LIMITS (0, CPPLUA_MAX_ARITY)
#define BOOST_PP_FILENAME_1 "detail/function_template.hpp"
#define FN_PARAM(name, r, arglist) r(*name)arglist
#include BOOST_PP_ITERATE()
#undef FN_PARAM

#define BOOST_PP_ITERATION_LIMITS (0, CPPLUA_MAX_ARITY)
#define BOOST_PP_FILENAME_1 "detail/function_template.hpp"
#define FN_PARAM(name, r, arglist) std::function<r arglist> const& name
#include BOOST_PP_ITERATE()
#undef FN_PARAM


#define BOOST_PP_ITERATION_LIMITS (0, CPPLUA_MAX_ARITY)
#define BOOST_PP_FILENAME_1 "detail/function_template.hpp"
#define FN_PARAM(name, r, arglist) boost::function<r arglist> const& name
#include BOOST_PP_ITERATE()
#undef FN_PARAM

#define BOOST_PP_ITERATION_LIMITS (0, CPPLUA_MAX_ARITY)
#define BOOST_PP_FILENAME_1 "detail/function_template.hpp"
// No definition of FN_PARAM: Member functions.
#include BOOST_PP_ITERATE()


#undef NARGS
#undef IF_ARGS
#undef TMPL_PARAMS
#undef TRAILING_TMPL_PARAMS
#undef ARG_TYPES
#undef PRINT_ARG
#undef ARGS_FROM_STACK

// Number converter //
template<typename T>
struct Converter<T, typename std::enable_if<
    LuaType<T>::value == LUA_TFUNCTION>::type>: ConverterBase<T> {
    
    static void push(lua_State* L, T const& f)
    {
        detail::pushFunction(L, f);
    }

    static unsigned nConversionSteps(lua_State* L, int idx)
    {
        if (lua_isfunction(L, idx))
            return 0;
        if (luaL_getmetafield(L, idx, "__call")) {
            lua_pop(L, 1);
            return 1;
        }
        return noConversion;
    }
    // toType is currently not supported.
};


} // namespace cpplua

#endif // CPPLUA_FUNCTION_HPP_INCLUDED
