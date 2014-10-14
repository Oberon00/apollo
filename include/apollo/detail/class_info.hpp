#ifndef APOLLO_CLASS_INFO_HPP_INCLUDED
#define APOLLO_CLASS_INFO_HPP_INCLUDED APOLLO_CLASS_INFO_HPP_INCLUDED

#include <apollo/detail/variadic_pass.hpp>

#include <boost/assert.hpp>
#include <lua.hpp>

#include <vector>
#include <type_traits>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>

namespace apollo { namespace detail {

template <typename From, typename To>
void* cast_static(void* src)
{
    return static_cast<To*>(static_cast<From*>(src));
}

using cast_function = void*(*)(void*);

struct class_info {
    class_info(std::type_info const* rtti_type_)
        : rtti_type(rtti_type_)
    { }

    // Explicit move and copy operations for MSVC

    class_info(class_info&& rhs)
        : bases(std::move(rhs.bases))
        , rtti_type(std::move(rhs.rtti_type))
    {}

    class_info(class_info const& rhs)
        : bases(rhs.bases)
        , rtti_type(rhs.rtti_type)
    {}

    class_info& operator= (class_info&& rhs)
    {
        bases = std::move(rhs.bases);
        rtti_type = std::move(rhs.rtti_type);
        return *this;
    }

    class_info& operator= (class_info const& rhs)
    {
        bases = rhs.bases;
        rtti_type = rhs.rtti_type;
        return *this;
    }

    // Contains all casts, including indirect bases.
    struct base_relation {
        std::ptrdiff_t offset;
        unsigned n_intermediate_bases;
    };
    std::unordered_map<class_info const*, base_relation> bases;

    std::type_info const* rtti_type;
};

using class_info_map = std::unordered_map<std::type_index, class_info>;

void* cast_class(void* obj, class_info const& cls, class_info const& to);
unsigned n_class_conversion_steps(class_info const& from, class_info const& to);


class_info_map& registered_classes(lua_State* L);
class_info* registered_class_opt(lua_State* L, std::type_info const& type);

// Only assert()s that the class is registered.
class_info& registered_class(lua_State* L, std::type_info const& type);

struct base_info {
    class_info const* type;
    cast_function cast;
};

template <typename Derived, typename Base>
int add_base_helper(
    std::vector<base_info>& bases,
    class_info_map const& base_infos)
{
    static_assert(std::is_base_of<Base, Derived>::value,
        "register_class: Base is no base of Derived");
    // If you see this error you passed a class in the Bases template
    // argument of register_class<T, Bases...>() that is not really a base of T.

    base_info binfo;
    auto i_bcinfo = base_infos.find(std::type_index(typeid(Base)));
    BOOST_ASSERT_MSG(i_bcinfo != base_infos.end(),
                        "Base classes must be registered before derived ones.");
    binfo.type = &i_bcinfo->second;
    binfo.cast = &cast_static<Derived, Base>;
    bases.push_back(std::move(binfo));
    return int();
}

class_info make_class_info_impl(
    std::type_info const* rtti_type, std::vector<base_info>& bases);

template <typename T, typename... Bases>
class_info make_class_info(class_info_map const& base_infos)
{
    std::vector<base_info> bases;
    variadic_pass(add_base_helper<T, Bases>(bases, base_infos)...);
    return make_class_info_impl(&typeid(T), bases);
}

} } // namespace apollo::detail


#endif // APOLLO_INSTANCE_HOLDER_HPP_INCLUDED
