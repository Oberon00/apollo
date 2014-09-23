#include <apollo/detail/class_info.hpp>
#include <apollo/detail/light_key.hpp>
#include <apollo/error.hpp>
#include <apollo/gc.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/errinfo_type_info_name.hpp>
#include "unordered_set"

#include <boost/throw_exception.hpp>

static light_key const class_info_map_key = {};

static void* cast_error_ambigous_base(void*)
{
    BOOST_THROW_EXCEPTION(apollo::ambiguous_base_error()
                          << apollo::errinfo::msg(
                              "class conversion from Lua to C++ failed:"
                              " requested base class is ambiguous"));
}

apollo::detail::class_info_map&
apollo::detail::registered_classes(lua_State* L)
{
    lua_rawgetp(L, LUA_REGISTRYINDEX, class_info_map_key);
    auto classes = static_cast<class_info_map*>(lua_touserdata(L, -1));
    if (classes)
        return *classes;
    classes = push_gc_object(L, class_info_map());
    lua_rawsetp(L, LUA_REGISTRYINDEX, class_info_map_key);
    return *classes;
}


apollo::detail::class_info*
apollo::detail::registered_class_opt(
    lua_State* L, std::type_info const& type)
{
    auto& classes = registered_classes(L);
    auto i_cls = classes.find(std::type_index(type));
    return i_cls == classes.end() ? nullptr : &i_cls->second;
}

apollo::detail::class_info&
apollo::detail::registered_class(
    lua_State* L, std::type_info const& type)
{
    auto cls = registered_class_opt(L, type);
    BOOST_ASSERT_MSG(cls, "Use of unregistered class.");
    return *cls;
}

void* apollo::detail::cast_class(void* obj, class_info const& from, class_info const& to)
{
    if (*from.rtti_type == *to.rtti_type)
        return obj;
    BOOST_THROW_EXCEPTION(class_conversion_error()
        << boost::errinfo_type_info_name(to.rtti_type->name())
        << errinfo::msg("not implemented"));
}
























