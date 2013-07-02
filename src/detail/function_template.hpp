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

#ifdef FN_PARAM
#   define PRINT_ARG(z, n, _) toType<BOOST_PP_CAT(A, n)>(L, n + 1)
#   define ARGS_FROM_STACK    BOOST_PP_ENUM(NARGS, PRINT_ARG, ~)

    template <typename R TRAILING_TMPL_PARAMS>
    R callWithStackArgs(lua_State* IF_ARGS(L), FN_PARAM(f, R, (ARG_TYPES)))
    {
        return f(ARGS_FROM_STACK);
    }

    namespace detail {

    IF_ARGS(template <TMPL_PARAMS>)
    inline int callWithStackArgsAndPush(lua_State* L, FN_PARAM(f, void, (ARG_TYPES)))
    {
        callWithStackArgs(L, f);
        return 0;
    }

    } // namespace detail
#else // FN_PARAM
#   undef PRINT_ARG
#   define PRINT_ARG(z, n, _) toType<BOOST_PP_CAT(A, n)>(L, n + 2)
#   define ARGS_FROM_STACK    BOOST_PP_ENUM(NARGS, PRINT_ARG, ~)

    template <class C, typename R TRAILING_TMPL_PARAMS>
    R callWithStackArgs(lua_State*, R(C::*f)(ARG_TYPES))
    {
        return toType<C>.*f(ARGS_FROM_STACK);
    }

    namespace detail {

    template <class C TRAILING_TMPL_PARAMS>
    int callWithStackArgsAndPush(lua_State* L, void(C::*f)(ARG_TYPES))
    {
        callWithStackArgs(L, f);
        return 0;
    }

    } // namespace detail

#endif // FN_PARAM / else
