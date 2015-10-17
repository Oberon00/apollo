// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifndef APOLLO_DEFAULT_ARGUMENT_HPP_INCLUDED
#define APOLLO_DEFAULT_ARGUMENT_HPP_INCLUDED APOLLO_DEFAULT_ARGUMENT_HPP_INCLUDED

#include <apollo/converters.hpp>

namespace apollo {

template <typename T, typename Base=pull_converter_for<T>>
struct default_argument: Base {
private:
    typename detail::remove_cvr<T>::type m_default;

public:
    using to_type = typename Base::to_type;

    default_argument():
        m_default()
    {}

    explicit default_argument(T&& default_)
        : m_default(std::move(default_))
    {}

    explicit default_argument(T const& default_)
        : m_default(default_)
    {}

    unsigned idx_n_conversion_steps(lua_State* L, int idx, int* next_idx) const
    {
        if (lua_isnone(L, idx)) {
            // Do not increase next_idx.
            return no_conversion - 1;
        }
        return n_conversion_steps_with(base(), L, idx, next_idx);
    }

    to_type idx_to(lua_State* L, int idx, int* next_idx) const
    {
        if (lua_isnone(L, idx)) {
            // Do not increase next_idx.
            return static_cast<to_type>(m_default);
        }
        return unchecked_to_with(base(), L, idx, next_idx);
    }

    to_type idx_safe_to(lua_State* L, int idx, int* next_idx) const
    {
        if (lua_isnone(L, idx)) {
            // Do not increase next_idx.
            return static_cast<to_type>(m_default);
        }
        return to_with(base(), L, idx, next_idx);
    }

private:
    Base const& base() const
    {
        return *static_cast<Base const*>(this);
    }
};

template <typename T>
default_argument<typename detail::remove_cvr<T>::type>
make_default_arg(T&& default_)
{
    return default_argument<typename detail::remove_cvr<T>::type>(
        std::forward<T>(default_));
}

} // namespace apollo

#endif // APOLLO_DEFAULT_ARGUMENT_HPP_INCLUDED
