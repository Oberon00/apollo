#ifndef APOLLO_LIGHT_KEY_HPP_INCLUDED
#define APOLLO_LIGHT_KEY_HPP_INCLUDED APOLLO_LIGHT_KEY_HPP_INCLUDED

namespace apollo { namespace detail {

struct light_key {
    operator void* () {
        return this;
    }
    operator void const* () const {
        return this;
    }
};

} } // namespace apollo::detail

#endif
