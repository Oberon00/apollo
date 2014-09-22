#if !BOOST_PP_IS_ITERATING
#   error "Do not include this internal header!"
#endif

#define NARGS BOOST_PP_ITERATION()
#if !NARGS
#   undef IF_ARGS
#   define IF_ARGS(x)
#else
#   undef IF_ARGS // Avoid warning.
#   define IF_ARGS(x) x
#endif

#define TMPL_PARAMS          BOOST_PP_ENUM_PARAMS(NARGS, typename A)
#define TRAILING_TMPL_PARAMS BOOST_PP_COMMA_IF(NARGS) TMPL_PARAMS
#define ARG_TYPES            BOOST_PP_ENUM_PARAMS(NARGS, A)


#undef PRINT_ARG
#define PRINT_ARG(z, n, _)       from_stack<BOOST_PP_CAT(A, n)>(L, n + 2)
#define ARGS_FROM_STACK          BOOST_PP_ENUM(NARGS, PRINT_ARG, ~)
#define PRINT_PUSH_ARG(z, n, _)  push(L, BOOST_PP_CAT(a, n));
#define ARGS_TO_STACK            BOOST_PP_REPEAT(NARGS, PRINT_PUSH_ARG, ~)
#define TRAILING_ARG_TYPES       BOOST_PP_COMMA_IF(NARGS) BOOST_PP_ENUM_PARAMS(NARGS, A)
#define TYPED_ARGS               BOOST_PP_ENUM_BINARY_PARAMS(NARGS, A, a)

template <class C, typename R TRAILING_TMPL_PARAMS>
R call_with_stack_args(lua_State* L, R(C::*f)(ARG_TYPES))
{
    return from_stack<C>.*f(ARGS_FROM_STACK);
}

namespace detail {

// Doesn't work with typename here.
template <template<class> class FObj, class R TRAILING_TMPL_PARAMS>
struct function_converter<FObj<R(ARG_TYPES)>> {
    typedef FObj<R(ARG_TYPES)> FType;
    static FType from_stack(lua_State* L, int idx)
    {
        registry_reference luaFunction(
            L, idx, registry_reference::ref_mode::copy);
        return [luaFunction](TYPED_ARGS) -> R {
            lua_State* L = luaFunction.L();
            stack_balance b(L);
            luaFunction.push();
            ARGS_TO_STACK
            pcall(L, NARGS, 1);
            R r = apollo::from_stack<R>(L, -1);
            return r;
        };
    }
};

} // namespace detail

