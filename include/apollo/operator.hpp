#ifndef APOLLO_OPERATOR_HPP_INCLUDED
#define APOLLO_OPERATOR_HPP_INCLUDED APOLLO_OPERATOR_HPP_INCLUDED

#include <utility> // std::forward
#include <boost/lexical_cast.hpp>
#include <apollo/raw_function.hpp>

namespace apollo { namespace op {


// The get_raw_* functions are workarounds for MSVC; see get_raw_ctor_wrapper.
#define APOLLO_DETAIL_BINOP(op, name) \
    template <typename Lhs, typename Rhs>                                  \
    auto name(Lhs lhs, Rhs rhs)                                            \
    -> decltype(lhs op rhs)                                                \
    {                                                                      \
        return lhs op rhs;                                                 \
    }                                                                      \
                                                                           \
    template <typename Lhs, typename Rhs>                                  \
    raw_function get_raw_##name()                                          \
    {                                                                      \
        using rv_t = decltype(std::declval<Lhs>() op std::declval<Rhs>()); \
        return to_raw_function<rv_t(*)(Lhs, Rhs), &name<Lhs, Rhs>>();      \
    }

#define APOLLO_DETAIL_PREFIXOP(op, name) \
    template <typename Operand>                                     \
    auto name(Operand operand)                                      \
     -> decltype(op operand)                                        \
    {                                                               \
        return op operand;                                          \
    }                                                               \
    template <typename Operand>                                     \
    raw_function get_raw_##name()                                   \
    {                                                               \
        using rv_t = decltype(op std::declval<Operand>());          \
        return to_raw_function<rv_t(*)(Operand), &name<Operand>>(); \
    }

// Operator names follow the names of the
// corresponding Lua metamethods (if any).

// Arithmetic operations (all have corresponding metamethods)
APOLLO_DETAIL_BINOP(+, add);
APOLLO_DETAIL_BINOP(-, sub);
APOLLO_DETAIL_BINOP(*, mul);
APOLLO_DETAIL_BINOP(/, div);
APOLLO_DETAIL_BINOP(%, mod);
APOLLO_DETAIL_PREFIXOP(-, unm);

// Comparisons
APOLLO_DETAIL_BINOP(==, eq); // Has metamethod.
APOLLO_DETAIL_BINOP(!=, ne);
APOLLO_DETAIL_BINOP(>,  gt);
APOLLO_DETAIL_BINOP(<,  lt); // Has metamethod.
APOLLO_DETAIL_BINOP(>=, ge);
APOLLO_DETAIL_BINOP(<=, le); // Has metamethod.

// Logical operations
APOLLO_DETAIL_BINOP(&&, land);
APOLLO_DETAIL_BINOP(||, lor);
APOLLO_DETAIL_PREFIXOP(!,  lnot);

// Bitwise operations (since Lua 5.3, all have corresponding metamethods)
APOLLO_DETAIL_BINOP(&, band);
APOLLO_DETAIL_BINOP(|, bor);
APOLLO_DETAIL_BINOP(^, bxor);
APOLLO_DETAIL_PREFIXOP(~,  bnot);
APOLLO_DETAIL_BINOP(<<, shl);
APOLLO_DETAIL_BINOP(>>, shr);

#undef APOLLO_DETAIL_BINOP
#undef APOLLO_DETAIL_PREFIXOP

template <typename T>
std::string to_string(T const& v)
{
    return boost::lexical_cast<std::string>(v);
}

} } // namespace apollo::op


#endif // APOLLO_OPERATOR_HPP_INCLUDED