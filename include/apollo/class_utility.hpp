#ifndef APOLLO_CLASS_UTILITY_HPP_INCLUDED
#define APOLLO_CLASS_UTILITY_HPP_INCLUDED APOLLO_CLASS_UTILITY_HPP_INCLUDED

#include <apollo/class.hpp>
#include <apollo/create_table.hpp>
#include <apollo/function.hpp>
#include <apollo/implicit_ctor.hpp>

namespace apollo {

namespace detail {

template <typename Cls, typename Parent>
class class_creator;

template <typename Derived>
class basic_classes_creator: public basic_table_setter<Derived>
{
public:
      basic_classes_creator(lua_State* L, int table_idx)
        : basic_table_setter<Derived>(L, table_idx)
    { }

    template <typename Cls, typename... Bases, typename K>
    class_creator<Cls, Derived> cls(K&& key);
};

class classes_creator: public basic_classes_creator<classes_creator>
{
public:
      classes_creator(lua_State* L, int idx)
        : basic_classes_creator<classes_creator>(L, idx)
    { }
};

template <typename T, typename Parent=void>
class class_creator: public basic_classes_creator<class_creator<T, Parent>> {
public:
    class_creator(lua_State* L, Parent* parent)
        : basic_classes_creator<class_creator<T, Parent>>(L, lua_gettop(L))
        , m_parent(parent)
    { }

    ~class_creator()
    {
        if (!m_parent)
            this->pop_table();
    }

    template<typename... Args>
    class_creator&& ctor(char const* name = "new")
    {
        return (*this)(name, get_raw_ctor_wrapper<T, Args...>());
    }

    // Implicit constructors //

    template<typename... Args>
    class_creator&& implicit_ctor(char const* name)
    {
        implicit_only_ctor<Args...>();
        return ctor<Args...>(name);
    }

    template<typename F>
    class_creator&& implicit_ctor_f(char const* name, F f)
    {
        implicit_only_ctor_f(f);
        return (*this)(name, f);
    }

    template<typename... Args>
    class_creator&& implicit_only_ctor()
    {
        add_implicit_ctor(this->m_L, &ctor_wrapper<T, Args...>);
        return std::move(*this);
    }

    template<typename F>
    class_creator&& implicit_only_ctor_f(F f)
    {
        add_implicit_ctor(this->m_L, f);
        return std::move(*this);
    }

    // Misc //

    typename std::add_rvalue_reference<Parent>::type end_cls()
    {
        m_parent->end_subtable();
        return std::move(*m_parent);
        // If you get an error about void here, you called end_cls() after
        // using export_class, but this is only neccessary to end the classes
        // created by .cls().
    }

private:
    Parent* const m_parent; // variant class[es]_creator?

};

template <typename Derived>
template <typename Cls, typename... Bases, typename K>
class_creator<Cls, Derived> basic_classes_creator<Derived>::cls(K&& key)
{
    register_class<Cls, Bases...>(this->m_L);
    push_class_metatable<Cls>(this->m_L);
    this->top_subtable(std::forward<K>(key));
    return {this->m_L, static_cast<Derived*>(this)};
}

} // namespace detail

template <typename T>
struct converter<detail::class_creator<T>>
    : public converter<detail::table_setter>
{};

inline detail::classes_creator export_classes(lua_State* L, int into = 0)
{
    if (!into) {
        lua_newtable(L);
        into = lua_gettop(L);
    }
    return detail::classes_creator(L, lua_absindex(L, into));
}

template <typename T, typename... Bases>
detail::class_creator<T> export_class(lua_State* L)
{
    register_class<T, Bases...>(L);
    push_class_metatable<T>(L);
    return detail::class_creator<T>(L, nullptr);
}

} // namespace apollo

#endif // APOLLO_CLASS_UTILITY_HPP_INCLUDED
