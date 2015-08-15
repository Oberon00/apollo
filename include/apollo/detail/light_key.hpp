// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifndef APOLLO_LIGHT_KEY_HPP_INCLUDED
#define APOLLO_LIGHT_KEY_HPP_INCLUDED APOLLO_LIGHT_KEY_HPP_INCLUDED

#include <apollo/converters_fwd.hpp>

namespace apollo {

namespace detail {

struct light_key {
    operator void* () {
        return this;
    }
    operator void const* () const {
        return this;
    }
};

} // namespace detail

template<>
struct converter<detail::light_key>: converter_base<converter<detail::light_key>> {

    static int push(lua_State* L, detail::light_key const& lk)
    {
        lua_pushlightuserdata(L, const_cast<void*>(
            static_cast<void const*>(lk)));
        return 1;
    }
};

} // namespace apollo

#endif
