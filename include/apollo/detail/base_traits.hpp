#ifndef APOLLO_BASE_TRAITS_HPP_INCLUDED
#define APOLLO_BASE_TRAITS_HPP_INCLUDED APOLLO_BASE_TRAITS_HPP_INCLUDED

#include <type_traits>

namespace apollo { namespace detail {

// Adapted from TC++PL (4th ed.) 28.4.4 (p. 800).
template <typename Base, typename Derived>
struct is_virtual_base_aux {
private:
    template <typename D>
    static auto check(D* d) -> decltype(static_cast<Base*>(d));
    static void check(...);
public:
    // The result type of get_pointer(arg) if there exists a get_pointer()
    // overload for Arg (arg's type). void otherwise.
    using type = typename std::is_same<
        decltype(check(std::declval<Derived*>())), void>::type;
};

template <typename Base, typename Derived>
struct is_virtual_base: is_virtual_base_aux<Base, Derived>::type {};

} } // namespace apollo::detail

#endif // APOLLO_BASE_TRAITS_HPP_INCLUDED
