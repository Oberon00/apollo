#ifndef APOLLO_CLASS_UTILITY_HPP_INCLUDED
#define APOLLO_CLASS_UTILITY_HPP_INCLUDED APOLLO_CLASS_UTILITY_HPP_INCLUDED

#include <apollo/class.hpp>
#include <apollo/function.hpp>
#include <apollo/create_table.hpp>

namespace apollo {

namespace detail {

template <typename T>
class class_creator: public basic_table_setter<class_creator<T>> {
public:
    class_creator(lua_State* L)
        : basic_table_setter<class_creator<T>>(L, lua_gettop(L), true)
    {
        m_pending_removals.insert(m_table_idx);
    }

    template<typename... Args>
    class_creator&& ctor(char const* name = "new")
    {
        return (*this)(name, get_raw_ctor_wrapper<T, Args...>());
    }

};

} // namespace detail

template <typename T, typename... Bases>
detail::class_creator<T> export_class(lua_State* L)
{
    register_class<T, Bases...>(L);
    push_class_metatable<T>(L);
    return detail::class_creator<T>(L);
}

} // namespace apollo

#endif // APOLLO_CLASS_UTILITY_HPP_INCLUDED
