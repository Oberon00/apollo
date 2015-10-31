// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

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
    using ptr_t = typename remove_cvr<Ptr>::type;
    using holder_t = ptr_instance_holder<ptr_t>;
    using cls_t = typename remove_cvr<
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

    using cls_t = typename remove_cvr<obj_t>::type;
    class_info const& cls = registered_class(L,
        boost::typeindex::type_id<cls_t>().type_info());
    emplace_bare_udata<holder_t>(L, std::forward<T>(val), cls);
    push_instance_metatable(L, cls);
    lua_setmetatable(L, -2);
}

APOLLO_API bool is_apollo_instance(lua_State* L, int idx);

inline instance_holder* as_holder(lua_State* L, int idx)
{
    BOOST_ASSERT(lua_isnil(L, idx) || is_apollo_instance(L, idx));
    return static_cast<instance_holder*>(lua_touserdata(L, idx));
}


char const err_noinst[]
    = "Value is neither nil nor an apollo instance userdata.";

template <typename T, typename Enable=void>
struct object_converter: object_converter<T const&>
{};

template <typename T>
struct object_converter<T const&, typename std::enable_if<
        !detail::pointer_traits<T>::is_valid
    >::type>: converter_base<converter<T const&>, ref_binder<T const>> {
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
    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        auto n_steps = object_converter<T const*>::n_conversion_steps(L, idx);
        if (n_steps == no_conversion)
            return get_ctor_opt(L, idx) ? 1 : no_conversion;

        // Reject null pointers.
        return object_converter<T const*>::to(L, idx) ?
            n_steps : no_conversion;
    }

    static ref_binder<T const> to(lua_State* L, int idx)
    {
        char const* err;
        auto ptr = object_converter<T const*>::detail_try_safe_to(L, idx, err);
        if (BOOST_UNLIKELY(err != nullptr)) {
            return {
                static_cast<T*>(get_ctor_opt(L, idx)->to(L, idx)),
                true};
        }
        return {ptr, false};
    }

    static ref_binder<T const> safe_to(lua_State* L, int idx)
    {
        char const* err;
        auto ptr = object_converter<T const*>::detail_try_safe_to(L, idx, err);
        if (BOOST_UNLIKELY(err != nullptr)) {
            auto ctor = get_ctor_opt(L, idx);
            if (!ctor) {
                BOOST_THROW_EXCEPTION(to_cpp_conversion_error()
                    << errinfo::msg(err));
            }
            return {
                static_cast<T*>(ctor->to(L, idx)),
                true};
        }
        return {ptr, false};
    }
};

template <typename T>
struct object_converter<T&, typename std::enable_if<
           !detail::pointer_traits<T>::is_valid
        && !std::is_const<T>::value
    >::type>: converter_base<converter<T&>> {
public:

    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        auto n_steps = object_converter<T*>::n_conversion_steps(L, idx);
        if (n_steps == no_conversion)
            return no_conversion;

        // Reject null pointers.
        return object_converter<T*>::to(L, idx) ?
            n_steps : no_conversion;
    }

    static T& to(lua_State* L, int idx)
    {
        return *object_converter<T*>::to(L, idx);
    }

    static T& safe_to(lua_State* L, int idx)
    {
        auto ptr = object_converter<T*>::safe_to(L, idx);
        if (!ptr) {
            BOOST_THROW_EXCEPTION(to_cpp_conversion_error() << errinfo::msg(
                "Attempt to convert nullptr to reference."));
        }
        return *ptr;
    }
};

template <typename Ptr>
struct object_converter<
        Ptr,
        typename std::enable_if<pointer_traits<Ptr>::is_smart>::type
    >: converter_base<converter<Ptr>> {
private:
    using ptr_t = typename remove_cvr<Ptr>::type;
    using is_ref = std::is_reference<Ptr>;

    static Ptr make_nil_smart_ptr(std::true_type) {
        BOOST_THROW_EXCEPTION(to_cpp_conversion_error()
                << errinfo::msg(
                    "Attempt to convert nil to smart pointer ref."));
    }

    static Ptr make_nil_smart_ptr(std::false_type) {
        return Ptr();
    }

public:
    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        APOLLO_DETAIL_CONSTCOND_BEGIN
        if (!is_ref::value && lua_isnil(L, idx))
            return no_conversion - 1;
        APOLLO_DETAIL_CONSTCOND_END

        if (!is_apollo_instance(L, idx))
            return no_conversion;
        if (boost::typeindex::type_id<ptr_instance_holder<ptr_t>>()
            == boost::typeindex::type_id_runtime(*as_holder(L, idx)))
            return 0;
        return no_conversion;
    }

    static Ptr to(lua_State* L, int idx)
    {
        if (lua_isnil(L, idx))
            return make_nil_smart_ptr(is_ref());

        return static_cast<ptr_instance_holder<ptr_t>*>(
            lua_touserdata(L, idx))->get_outer_ptr();
    }

    static Ptr safe_to(lua_State* L, int idx)
    {
        if (BOOST_LIKELY(is_apollo_instance(L, idx))) {
            auto holder = as_holder(L, idx);
            if (boost::typeindex::type_id<ptr_instance_holder<ptr_t>>()
                != boost::typeindex::type_id_runtime(*holder)
            ) {
                BOOST_THROW_EXCEPTION(to_cpp_conversion_error()
                    << errinfo::msg(
                        "Invalid pointer type (when casting to smart pointers,"
                        " conversion to base or const variants"
                        " are not supported)."));
            }
            return static_cast<ptr_instance_holder<ptr_t>*>(
                lua_touserdata(L, idx))->get_outer_ptr();
        }

        if (lua_isnil(L, idx))
            return make_nil_smart_ptr(is_ref());

        BOOST_THROW_EXCEPTION(to_cpp_conversion_error()
            << errinfo::msg(err_noinst));
    }
};

template <typename Ptr>
struct object_converter<
        Ptr,
        typename std::enable_if<std::is_pointer<Ptr>::value>::type
    >: converter_base<converter<Ptr>> {
private:
    using ptr_traits = pointer_traits<Ptr>;
    using pointee_t = typename ptr_traits::pointee_type;
    using obj_t = typename remove_cvr<pointee_t>::type;
public:
    static unsigned n_conversion_steps(lua_State* L, int idx)
    {
        if (lua_isnil(L, idx))
            return no_conversion - 1;
        if (!is_apollo_instance(L, idx))
            return no_conversion;

        auto holder = as_holder(L, idx);

        if (!is_const_correct(holder))
            return no_conversion;

        return n_class_conversion_steps(
            holder->type(), static_class_id<obj_t>::id);
    }

    static Ptr to(lua_State* L, int idx)
    {
        auto holder = as_holder(L, idx);
        if (!holder) // This means not a userdata, so assume nil.
            return nullptr;
        return static_cast<Ptr>(cast_class(
            holder->get(),
            holder->type(),
            static_class_id<obj_t>::id));
    }

    static Ptr safe_to(lua_State* L, int idx)
    {
        char const* err;
        Ptr result = detail_try_safe_to(L, idx, err);
        if (BOOST_UNLIKELY(err != nullptr)) {
            BOOST_THROW_EXCEPTION(to_cpp_conversion_error()
                << errinfo::msg(err));
        }
        return result;
    }

    // For use by object_converter<T const&>.
    static Ptr detail_try_safe_to(lua_State* L, int idx, char const*& err)
    {
        err = nullptr;
        if (BOOST_LIKELY(is_apollo_instance(L, idx))) {
            auto holder = as_holder(L, idx);
            if (!is_const_correct(holder)) {
                err = "Class conversion loses const.";
                return nullptr;
            }

            return static_cast<Ptr>(try_cast_class(
                holder->get(),
                holder->type(),
                static_class_id<obj_t>::id,
                err));
        }
        if (lua_isnil(L, idx)) {
            return nullptr;
        }

        err = err_noinst;
        return nullptr;
    }

private:
    static bool is_const_correct(instance_holder const* holder)
    {
        // MSVC complains that holder is unused if ptr_traits::is_const is true.
        (void)holder;
        return ptr_traits::is_const || !holder->is_const();
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
    using cls_t = typename detail::remove_cvr<obj_t>::type;

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
    using obj_t = typename detail::remove_cvr<T>::type;
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
    registry.insert(std::make_pair(
        index, detail::make_class_info<T, Bases...>(registry)));
}


// Userdata converters //

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
            typename detail::remove_cvr<U>::type , T>::value, "");
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
auto to_object(lua_State* L, int idx)
-> decltype(detail::object_converter<
    typename std::remove_cv<T>::type>::to(L, idx))
{
    return detail::object_converter<
        typename std::remove_cv<T>::type>::to(L, idx);
}


} // namespace apollo

#endif // APOLLO_CLASS_HPP_INCLUDED
