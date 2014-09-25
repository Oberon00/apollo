#ifndef APOLLO_SMART_PTR_HPP_INCLUDED
#define APOLLO_SMART_PTR_HPP_INCLUDED APOLLO_SMART_PTR_HPP_INCLUDED

#include <boost/get_pointer.hpp>

#include <type_traits>

#ifdef BOOST_NO_CXX11_SMART_PTR
#    error No C++11 smart pointers.
#endif

namespace apollo {
namespace detail {

// Automatically recognize std::shared_ptr, unique_ptr and auto_ptr.
using boost::get_pointer;

// Adapted from TC++PL (4th ed.) 28.4.4 (p. 800).
template <typename Arg>
struct get_pointer_result {
private:
    template <typename X>
    static auto check(X const& x) -> decltype(get_pointer(x));
    static void check(...);
public:
    // The result type of get_pointer(arg) if there exists a get_pointer()
    // overload for Arg (arg's type). void otherwise.
    using type = decltype(check(std::declval<Arg>()));
};

template <typename T>
struct pointer_traits {
    using ptr_type = typename get_pointer_result<T>::type;

    // Exists a get_pointer() overload for T which returns a pointer?
    static bool const is_valid = !std::is_same<ptr_type, void>::value;
    static bool const is_smart = is_valid && !std::is_pointer<T>::value;
    using pointee_type = typename std::remove_pointer<ptr_type>::type;
};
}
} // namespace apollo::detail

#endif // APOLLO_SMART_PTR_HPP_INCLUDED
