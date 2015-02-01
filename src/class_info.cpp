#include <apollo/converters_fwd.hpp> // no_conversion
#include <apollo/error.hpp>
#include <apollo/gc.hpp>
#include <apollo/detail/class_info.hpp>
#include <apollo/detail/light_key.hpp>
#include <apollo/typeid.hpp>

#include <boost/exception/errinfo_type_info_name.hpp>
#include <boost/exception/info.hpp>
#include <boost/throw_exception.hpp>

#include <limits>

static apollo::detail::light_key const class_info_map_key = {};
static std::ptrdiff_t const error_ambiguous_base =
    std::numeric_limits<std::ptrdiff_t>::min();



APOLLO_API std::size_t
apollo::detail::allocate_class_id(std::type_info const& cls)
{
    static std::unordered_map<std::type_index, std::size_t> ids;
    static std::size_t next_id = 0;
    auto inserted = ids.insert({cls, next_id});
    if (inserted.second)
        ++next_id;
    return inserted.first->second;
}

APOLLO_API apollo::detail::class_info_map&
apollo::detail::registered_classes(lua_State* L)
{
    lua_rawgetp(L, LUA_REGISTRYINDEX, class_info_map_key);
    auto classes = static_cast<class_info_map*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    if (classes)
        return *classes;
    classes = push_gc_object(L, class_info_map());
    lua_rawsetp(L, LUA_REGISTRYINDEX, class_info_map_key);
    return *classes;
}


APOLLO_API apollo::detail::class_info*
apollo::detail::registered_class_opt(
    lua_State* L, std::type_info const& type)
{
    auto& classes = registered_classes(L);
    auto i_cls = classes.find(std::type_index(type));
    return i_cls == classes.end() ? nullptr : &i_cls->second;
}

APOLLO_API apollo::detail::class_info&
apollo::detail::registered_class(
    lua_State* L, std::type_info const& type)
{
    auto cls = registered_class_opt(L, type);
    BOOST_ASSERT_MSG(cls, "Use of unregistered class.");
    return *cls;
}

APOLLO_API void*
apollo::detail::cast_class(
    void* obj, class_info const& from, std::size_t to)
{
    if (from.static_id == to)
        return obj;
    auto i_base_relation = from.bases.find(to);
    if (i_base_relation == from.bases.end()) {
        BOOST_THROW_EXCEPTION(apollo::class_conversion_error()
            << apollo::errinfo::msg("Requested type is no base class.")
            << apollo::errinfo::source_typeid_name(from.rtti_type->name()));
    }
    if (i_base_relation->second.offset == error_ambiguous_base) {
        BOOST_THROW_EXCEPTION(apollo::ambiguous_base_error()
            << apollo::errinfo::msg("Requested type is ambigous base class.")
            << apollo::errinfo::source_typeid_name(from.rtti_type->name()));
    }
    return static_cast<char*>(obj) + i_base_relation->second.offset;
}

APOLLO_API unsigned apollo::detail::n_class_conversion_steps(
    class_info const& from, std::size_t to)
{
    if (from.static_id == to)
        return 0;
    auto i_base_relation = from.bases.find(to);
    if (i_base_relation == from.bases.end())
        return no_conversion;
    return i_base_relation->second.n_intermediate_bases + 1;
}

APOLLO_API apollo::detail::class_info
apollo::detail::make_class_info_impl(
    const std::type_info* rtti_type,
    std::size_t static_id,
    std::vector<apollo::detail::base_info>& bases)
{
    class_info info(rtti_type, static_id);

    // Determine offsets to base subobjects and detect ambigous base classes.
    //
    // If multiple bases of the same type exist, the one with the least
    // intermediate bases (i.e. the most direct base) will be selected. If there
    // is no unique such class, the special error_ambiguous_base offset will be
    // assigned, subsequently causing cast_class to throw.
    for (auto& base: bases) {
        auto const base_offset = // Ugly hack, but hardly replaceable
            static_cast<char*>(base.cast(reinterpret_cast<void*>(1))) -
            reinterpret_cast<char*>(1);
        info.bases[base.type->static_id] = {base_offset, 0};
        for (auto& indirect_base: base.type->bases) {
            auto offset = base_offset + indirect_base.second.offset;
            auto n_intermediate_bases =
                indirect_base.second.n_intermediate_bases + 1;
            auto i_existing_base = info.bases.find(indirect_base.first);
            if (i_existing_base == info.bases.end()) {
                info.bases.insert({indirect_base.first,
                    {offset, n_intermediate_bases}});
            } else if (
                i_existing_base->second.n_intermediate_bases >
                indirect_base.second.n_intermediate_bases
            ) {
                i_existing_base->second = {offset, n_intermediate_bases};
            } else if (
                i_existing_base->second.n_intermediate_bases ==
                indirect_base.second.n_intermediate_bases
            ) {
                i_existing_base->second.offset = error_ambiguous_base;
            }
        }
    }
    return info;
}
