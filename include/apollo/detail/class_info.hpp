#ifndef APOLLO_CLASS_INFO_HPP_INCLUDED
#define APOLLO_CLASS_INFO_HPP_INCLUDED APOLLO_CLASS_INFO_HPP_INCLUDED

#include "for_each_type.hpp"
#include "base_traits.hpp"

#include <boost/assert.hpp>
#include <lua.hpp>

#include <vector>
#include <type_traits>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>

namespace apollo {
namespace detail {

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


    struct base_info {
        class_info const* type;
        cast_function cast;
    };
    std::vector<base_info> bases;
    std::type_info const* rtti_type;

    // Contains all casts, including indirect bases.
    std::unordered_map<std::type_index, std::ptrdiff_t> subobject_offsets;
};

using class_info_map = std::unordered_map<std::type_index, class_info>;

void* cast_class(void* obj, class_info const& cls, class_info const& to);


template <typename Derived>
struct base_adder
{
    base_adder(
        std::vector<class_info::base_info>& bases,
        class_info_map const& base_infos)
        : m_bases(&bases)
        , m_base_infos(&base_infos)
    {}

    template <typename Base>
    void apply()
    {
        class_info::base_info binfo;
        auto i_bcinfo = m_base_infos->find(std::type_index(typeid(Base)));
        BOOST_ASSERT_MSG(i_bcinfo != m_base_infos->end(),
                         "Base classes must be registered before derived ones.");
        binfo.type = &i_bcinfo->second;
        binfo.cast = &cast_static<Derived, Base>;
        m_bases->push_back(std::move(binfo));
    }

    std::vector<class_info::base_info>* m_bases;
    class_info_map const* m_base_infos;
};

template <typename T, typename Bases>
class_info make_class_info(class_info_map const& base_infos)
{
    class_info info(&typeid(T));
    for_each_type<Bases>(base_adder<T>(info.bases, base_infos));
    return info;
}

class_info_map& registered_classes(lua_State* L);
class_info* registered_class_opt(lua_State* L, std::type_info const& type);

// Only assert()s that the class is registered.
class_info& registered_class(lua_State* L, std::type_info const& type);

}
} // namespace apollo::detail


#endif // APOLLO_INSTANCE_HOLDER_HPP_INCLUDED
