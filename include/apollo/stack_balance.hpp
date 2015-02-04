#ifndef APOLLO_STACK_BALANCE_HPP_INCLUDED
#define APOLLO_STACK_BALANCE_HPP_INCLUDED APOLLO_STACK_POP_HPP_INCLUDED

#include <apollo/config.hpp>

#include <apollo/lua_include.hpp>

namespace apollo {

class APOLLO_API stack_balance {
public:
    enum action { pop = 1, push_nil = 2, adjust = pop | push_nil, debug = 4 };
    explicit stack_balance(
        lua_State* L,
        int diff = 0,
        int act = pop|debug);
    ~stack_balance();

    // MSVC complained that the assignment operator could not be generated.
    stack_balance& operator=(stack_balance const&) = delete;

private:
    lua_State* const m_L;
    int const m_desired_top;
    int const m_action;
};

} // namespace apollo

#endif // APOLLO_STACK_POP_HPP_INCLUDED
