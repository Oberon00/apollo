#ifndef APOLLO_OPERATOR_HPP_INCLUDED
#define APOLLO_OPERATOR_HPP_INCLUDED APOLLO_OPERATOR_HPP_INCLUDED

namespace apollo { namespace op {


#define APOLLO_DETAIL_BINOP(op, name)     \
    template <typename Lhs, typename Rhs> \
    auto name(Lhs lhs, Rhs rhs)           \
    -> decltype(lhs op rhs)               \
    {                                     \
        return lhs op rhs;                \
    }

#define APOLLO_DETAIL_PREFIXOP(op, name) \
    template <typename Operand>          \
    auto name(Operand operand)           \
     -> decltype(op operand)             \
    {                                    \
        return op operand;               \
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

} } // namespace apollo::op


#endif // APOLLO_OPERATOR_HPP_INCLUDED