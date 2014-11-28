#ifndef APOLLO_DEFAULT_ARGUMENT_HPP_INCLUDED
#define APOLLO_DEFAULT_ARGUMENT_HPP_INCLUDED APOLLO_DEFAULT_ARGUMENT_HPP_INCLUDED

#include <apollo/converters.hpp>

namespace apollo {

template <typename T, typename Base=pull_converter_for<T>>
struct default_argument: Base {
private:
    typename detail::remove_qualifiers<T>::type m_default;

public:
    default_argument():
        m_default()
    {}

    explicit default_argument(T&& default_)
        : m_default(std::move(default_))
    {}

    explicit default_argument(T const& default_)
        : m_default(default_)
    {}
    
    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        if (lua_isnone(L, idx))
            return no_conversion - 1;
        return Base::n_conversion_steps(L, idx);
    }

    typename Base::to_type from_stack(lua_State* L, int idx)
    {
        if (lua_isnone(L, idx))
            return static_cast<typename Base::to_type>(m_default);
        return Base::from_stack(L, idx);
    }
};

template <typename T>
default_argument<typename detail::remove_qualifiers<T>::type>
make_default_arg(T&& default_)
{
    return default_argument<typename detail::remove_qualifiers<T>::type>(
        std::forward<T>(default_));
}

} // namespace apollo

#endif // APOLLO_DEFAULT_ARGUMENT_HPP_INCLUDED
