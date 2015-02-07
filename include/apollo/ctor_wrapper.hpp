#ifndef APOLLO_CTOR_WRAPPER_HPP_INCLUDED
#define APOLLO_CTOR_WRAPPER_HPP_INCLUDED

#include <utility>
#include <apollo/smart_ptr.hpp>

namespace apollo {

template <typename T, typename... Args>
T ctor_wrapper(Args... args)
{
    return T(std::forward<Args>(args)...);
}

template <
    typename T, typename... Args,
    typename _ptr = typename std::conditional<
        detail::pointer_traits<T>::is_valid, T, T*>::type>
_ptr new_wrapper(Args... args)
{
    using obj_t = typename detail::pointer_traits<T>::pointee_type;
    return static_cast<_ptr>(new obj_t(std::forward<Args>(args)...));
}

} // namespace apollo

#endif // APOLLO_CTOR_WRAPPER_HPP_INCLUDED
