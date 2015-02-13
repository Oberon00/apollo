#ifndef APOLLO_CLASS_HPP_INCLUDED
#define APOLLO_CLASS_HPP_INCLUDED APOLLO_CLASS_HPP_INCLUDED

#include <apollo/config.hpp>
#include <apollo/converters.hpp>
#include <apollo/gc.hpp> // For push_bare_udata().
#include <apollo/detail/smart_ptr.hpp>
#include <apollo/typeid.hpp>
#include <apollo/detail/class_info.hpp>
#include <apollo/detail/instance_holder.hpp>
#include <apollo/detail/light_key.hpp>
#include <apollo/detail/ref_binder.hpp>

namespace apollo {

namespace detail {
APOLLO_API void push_instance_metatable(
    lua_State* L,
    class_info const& cls) BOOST_NOEXCEPT;

template <typename Ptr>
void push_instance_ptr(lua_State* L, Ptr&& ptr)
{
    using ptr_t = typename remove_qualifiers<Ptr>::type;
    using holder_t = ptr_instance_holder<ptr_t>;
    using cls_t = typename remove_qualifiers<
        typename pointer_traits<ptr_t>::pointee_type>::type;

    class_info const& cls = registered_class(L,
        boost::typeindex::type_id<cls_t>().type_info());
    emplace_bare_udata<holder_t>(L, std::forward<Ptr>(ptr), cls);
    push_instance_metatable(L, cls);
    lua_setmetatable(L, -2);
}

template <typename T>
void push_instance_val(lua_State* L, T&& val)
{
    using obj_t = typename std::remove_reference<T>::type;
    using holder_t = value_instance_holder<obj_t>;

    using cls_t = typename remove_qualifiers<obj_t>::type;
    class_info const& cls = registered_class(L,
        boost::typeindex::type_id<cls_t>().type_info());
    emplace_bare_udata<holder_t>(L, std::forward<T>(val), cls);
    push_instance_metatable(L, cls);
    lua_setmetatable(L, -2);
}

APOLLO_API bool is_apollo_instance(lua_State* L, int idx);

inline instance_holder* as_holder(lua_State* L, int idx)
{
    BOOST_ASSERT(is_apollo_instance(L, idx));
    return static_cast<instance_holder*>(lua_touserdata(L, idx));
}

template <typename T, typename Enable=void>
struct object_converter: object_converter<T const&>
{};

template <typename T>
struct object_converter<T const&, typename std::enable_if<
        !detail::pointer_traits<T>::is_valid
    >::type>: converter_base<T const&> {
private:
    static implicit_ctor* get_ctor_opt(lua_State* L, int idx)
    {
        auto const& cls = registered_class(L,
            boost::typeindex::type_id<T>().type_info());
        auto const& ctors = cls.implicit_ctors;
        auto i_ctor = ctors.find(ltypeid(L, idx));
        return i_ctor == ctors.end() ? nullptr : i_ctor->second.get();
    }
public:
    using to_type = ref_binder<T const>;

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        auto n_steps = object_converter<T const*>::n_conversion_steps(L, idx);
        if (n_steps == no_conversion)
            return get_ctor_opt(L, idx) ? 1 : no_conversion;

        // Reject null pointers.
        return object_converter<T const*>::from_stack(L, idx) ?
            n_steps : no_conversion;
    }

    static ref_binder<T const> from_stack(lua_State* L, int idx)
    {
        auto n_steps = object_converter<T const*>::n_conversion_steps(L, idx);
        if (n_steps == no_conversion) {
            return {
                static_cast<T*>(get_ctor_opt(L, idx)->from_stack(L, idx)),
                true};
        }
        return {object_converter<T const*>::from_stack(L, idx), false};
    }
};

template <typename T>
struct object_converter<T&, typename std::enable_if<
           !detail::pointer_traits<T>::is_valid
        && !std::is_const<T>::value
    >::type>: converter_base<T&> {
public:

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
    using is_ref = std::is_reference<Ptr>;

    static Ptr make_nil_smart_ptr(std::true_type) {
        BOOST_ASSERT(false);
        BOOST_THROW_EXCEPTION(
            std::logic_error("attempt to convert nil to smart pointer ref"));
    }

    static Ptr make_nil_smart_ptr(std::false_type) {
        return Ptr();
    }

public:
    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
#ifdef BOOST_MSVC
#   pragma warning(push)
#   pragma warning(disable:4127) // conditional expression is constant
#endif
        if (!is_ref::value && lua_isnil(L, idx))
            return no_conversion - 1;
#ifdef BOOST_MSVC
#   pragma warning(pop)
#endif
        if (!is_apollo_instance(L, idx))
            return no_conversion;
        if (boost::typeindex::type_id<ptr_instance_holder<ptr_t>>()
            == boost::typeindex::type_id_runtime(*as_holder(L, idx)))
            return 0;
        return no_conversion;
    }

    static Ptr from_stack(lua_State* L, int idx)
    {
#ifdef BOOST_MSVC
#   pragma warning(push)
#   pragma warning(disable:4127) // conditional expression is constant
#endif
        if (lua_isnil(L, idx))
            return make_nil_smart_ptr(is_ref());
#ifdef BOOST_MSVC
#   pragma warning(pop)
#endif
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
        if (lua_isnil(L, idx))
            return no_conversion - 1;
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
            holder->type(), static_class_id<obj_t>::id);
    }

    static Ptr from_stack(lua_State* L, int idx)
    {
        if (lua_isnil(L, idx))
            return nullptr;
        auto holder = as_holder(L, idx);
        return static_cast<Ptr>(cast_class(
            holder->get(),
            holder->type(),
            static_class_id<obj_t>::id));
    }
};

} // namespace detail

template <typename T>
typename std::enable_if<!detail::pointer_traits<T>::is_valid>::type
push_object(lua_State* L, T&& v)
{
    detail::push_instance_val(L, std::forward<T>(v));
}

template <typename T, typename... Args>
void emplace_object(lua_State* L, Args&&... args)
{
    using obj_t = typename std::remove_reference<T>::type;
    using holder_t = detail::value_instance_holder<obj_t>;
    using cls_t = typename detail::remove_qualifiers<obj_t>::type;

    detail::class_info const& cls = detail::registered_class(L,
        boost::typeindex::type_id<cls_t>().type_info());
    emplace_bare_udata<holder_t>(L, cls, std::forward<Args>(args)...);
    detail::push_instance_metatable(L, cls);
    lua_setmetatable(L, -2);

}


template <typename Ptr>
typename std::enable_if<detail::pointer_traits<Ptr>::is_valid>::type
push_object(lua_State* L, Ptr&& p)
{
    detail::push_instance_ptr(L, std::forward<Ptr>(p));
}

template <typename T>
void push_class_metatable(lua_State* L)
{
    using obj_t = typename detail::remove_qualifiers<T>::type;
    detail::push_instance_metatable(L, detail::registered_class(L,
        boost::typeindex::type_id<obj_t>().type_info()));
}

template <typename T, typename... Bases>
void register_class(lua_State* L)
{
    auto& registry = detail::registered_classes(L);
    auto index = boost::typeindex::type_id<T>();
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
    static int push(lua_State* L, U&& v)
    {
        static_assert(std::is_same<
            typename detail::remove_qualifiers<U>::type , T>::value, "");
        push_object(L, std::forward<U>(v));
        return 1;
    }
};

template <typename T>
unsigned n_object_conversion_steps(lua_State* L, int idx)
{
    return detail::object_converter<
        typename std::remove_cv<T>::type>::n_conversion_steps(L, idx);
}

template <typename T>
auto object_from_stack(lua_State* L, int idx)
-> decltype(detail::object_converter<
    typename std::remove_cv<T>::type>::from_stack(L, idx))
{
    return detail::object_converter<
        typename std::remove_cv<T>::type>::from_stack(L, idx);
}


} // namespace apollo

#endif // APOLLO_CLASS_HPP_INCLUDED
