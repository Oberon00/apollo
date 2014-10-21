#ifndef APOLLO_CLASS_HPP_INCLUDED
#define APOLLO_CLASS_HPP_INCLUDED APOLLO_CLASS_HPP_INCLUDED

#include <apollo/converters.hpp>
#include <apollo/gc.hpp> // For push_bare_udata().
#include <apollo/smart_ptr.hpp>
#include <apollo/detail/class_info.hpp>
#include <apollo/detail/instance_holder.hpp>
#include <apollo/detail/light_key.hpp>

#include <memory>

namespace apollo {

namespace detail {
void push_instance_metatable(
    lua_State* L,
    class_info const& cls) BOOST_NOEXCEPT;

template <typename Ptr>
void push_instance(lua_State* L, Ptr&& ptr)
{
    using ptr_t = typename remove_qualifiers<Ptr>::type;
    using holder_t = ptr_instance_holder<ptr_t>;
    using cls_t = typename remove_qualifiers<
        typename pointer_traits<ptr_t>::pointee_type>::type;

    class_info const& cls = registered_class(L, typeid(cls_t));
    push_bare_udata(L, holder_t(std::forward<Ptr>(ptr), cls));
    push_instance_metatable(L, cls);
    lua_setmetatable(L, -2);
}

bool is_apollo_instance(lua_State* L, int idx);

inline instance_holder* as_holder(lua_State* L, int idx)
{
    BOOST_ASSERT(is_apollo_instance(L, idx));
    return static_cast<instance_holder*>(lua_touserdata(L, idx));
}

template <typename T, typename Enable=void>
struct object_converter: converter_base<T> {

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        return add_conversion_step(
            object_converter<T&>::n_conversion_steps(L, idx));
    }

    static T from_stack(lua_State* L, int idx)
    {
        return object_converter<T&>::from_stack(L, idx);
    }
};

template <typename T>
struct object_converter<T&, typename std::enable_if<
        !detail::pointer_traits<T>::is_valid
    >::type>: converter_base<T&> {

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        auto n_steps = object_converter<T*>::n_conversion_steps(L, idx);
        if (n_steps == no_conversion)
            return no_conversion;

        // Reject null pointers.
        return object_converter<T*>::from_stack(L, idx) ?
            n_steps : no_conversion;
    }

    static T& from_stack(lua_State* L, int idx)
    {
        return *object_converter<T*>::from_stack(L, idx);
    }
};

template <typename Ptr>
struct object_converter<
        Ptr,
        typename std::enable_if<pointer_traits<Ptr>::is_smart>::type
    >: converter_base<Ptr> {
private:
    using ptr_t = typename remove_qualifiers<Ptr>::type;
public:
    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        if (!is_apollo_instance(L, idx))
            return no_conversion;
        if (typeid(ptr_instance_holder<ptr_t>) == typeid(*as_holder(L, idx)))
            return 0;
        return no_conversion;
    }

    static Ptr from_stack(lua_State* L, int idx)
    {
        return static_cast<ptr_instance_holder<ptr_t>*>(
            lua_touserdata(L, idx))->get_outer_ptr();
    }
};

template <typename Ptr>
struct object_converter<
        Ptr,
        typename std::enable_if<std::is_pointer<Ptr>::value>::type
    >: converter_base<Ptr> {
private:
    using ptr_traits = pointer_traits<Ptr>;
    using pointee_t = typename ptr_traits::pointee_type;
    using obj_t = typename remove_qualifiers<pointee_t>::type;
public:
    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        if (!is_apollo_instance(L, idx))
            return no_conversion;

        auto holder = as_holder(L, idx);

#ifdef BOOST_MSVC
#   pragma warning(push)
#   pragma warning(disable:4127) // conditional expression is constant
#endif
        if (!ptr_traits::is_const && holder->is_const())
            return no_conversion;
#ifdef BOOST_MSVC
#   pragma warning(pop)
#endif

        return n_class_conversion_steps(
            holder->type(),
            registered_class(L, typeid(obj_t)));
    }

    static Ptr from_stack(lua_State* L, int idx)
    {
        auto holder = as_holder(L, idx);
        return static_cast<Ptr>(cast_class(holder->get(),
            holder->type(),
            registered_class(L, typeid(obj_t))));
    }
};

template <typename T>
typename std::enable_if<!pointer_traits<T>::is_valid>::type push_object(
    lua_State* L, T&& v)
{
    using obj_t = typename detail::remove_qualifiers<T>::type;
    push_instance(L, std::unique_ptr<obj_t>(new obj_t(std::forward<T>(v))));
}

template <typename Ptr>
typename std::enable_if<pointer_traits<Ptr>::is_valid>::type push_object(
    lua_State* L, Ptr&& p)
{
    push_instance(L, std::forward<Ptr>(p));
}

} // namespace detail

template <typename T>
void push_class_metatable(lua_State* L)
{
    using obj_t = typename detail::remove_qualifiers<T>::type;
    detail::push_instance_metatable(
        L,
        detail::registered_class(L, typeid(obj_t)));
}

template <typename T, typename... Bases>
void register_class(lua_State* L)
{
    auto& registry = detail::registered_classes(L);
    auto index = std::type_index(typeid(T));
    if (registry.find(index) != registry.end()) {
        BOOST_ASSERT_MSG(false, "Class already registered!");
    }
    registry.insert({index, detail::make_class_info<T, Bases...>(registry)});
}

// Userdata converterers //

template<typename T, typename Enable>
struct converter: detail::object_converter<T>
{
    static_assert(detail::lua_type_id<T>::value == LUA_TUSERDATA,
        "Builtin type treated as object.");
    // Did you forget to #include <apollo/function.hpp>?

    template <typename U>
    static void push(lua_State* L, U&& v)
    {
        static_assert(std::is_same<
            typename detail::remove_qualifiers<U>::type , T>::value, "");
        detail::push_object(L, std::forward<U>(v));
    }
};

} // namespace apollo

#endif // APOLLO_CLASS_HPP_INCLUDED
