#ifndef APOLLO_CLASS_HPP_INCLUDED
#define APOLLO_CLASS_HPP_INCLUDED APOLLO_CLASS_HPP_INCLUDED

#include "converters.hpp"
#include "smart_ptr.hpp"
#include "detail/instance_holder.hpp"
#include "detail/class_info.hpp"
#include "detail/light_key.hpp"
#include <memory>

namespace apollo {

namespace detail {
void push_instance_metatable(
    lua_State* L,
    class_info const& cls);

template <typename Ptr>
void push_instance(lua_State* L, Ptr&& ptr)
{
    typedef typename remove_qualifiers<Ptr>::type ptr_t;
    typedef ptr_instance_holder<ptr_t> holder_t;
    typedef typename pointer_traits<Ptr>::pointee_type cls_t;

    class_info const& cls = registered_class(L, typeid(cls_t));
    void* storage = lua_newuserdata(L, sizeof(holder_t));
    new(storage) holder_t(std::forward<Ptr>(ptr), cls);
    push_instance_metatable(L, cls);
    lua_setmetatable(L, -2);
}

bool is_apollo_instance(lua_State* L, int idx);
} // namespace detail

template <typename T>
void push_metatable(lua_State* L)
{
    typedef typename detail::remove_qualifiers<T>::type obj_t;
    detail::push_instance_metatable(
        L,
        detail::registered_class(L, typeid(obj_t)));
}

// Userdata converterers //

template<typename T, typename Enable> // Pass by value (copy). Both own their own copy.
struct converter: converter_base<T> {

    static void push(lua_State* L, T const& obj)
    {
        typedef typename detail::remove_qualifiers<T>::type obj_t;
        std::unique_ptr<obj_t> ptr(new obj_t(obj));
        detail::push_instance(L, std::move(ptr));
    }

    static unsigned n_conversion_steps(lua_State*, int)
    {
        BOOST_ASSERT("not implemented" && false);
    }

    static T& from_stack(lua_State*, int)
    {
        BOOST_ASSERT("not implemented" && false);
    }
};

template<typename T> // Pass by rvalue reference (move). The moved-to owns.
struct converter<T&&>: converter_base<T&&> {

    static void push(lua_State* L, T&&)
    {

    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
    }

    static T& from_stack(lua_State* L, int idx)
    {
    }
};

template<typename T> // Pass by (smart) pointer.
//  Ownership semantics are the pointer's (which can be copied or moved).
struct converter<T, typename std::enable_if<
        detail::pointer_traits<T>::isValid
        >::type> : converter_base<T> {

    static void push(lua_State* L, T const&)
    {
    }

    static void push(lua_State* L, T&&)
    {
    }

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
    }

    static T& from_stack(lua_State* L, int idx)
    {
    }
};

} // namespace apollo

#endif // APOLLO_CLASS_HPP_INCLUDED
