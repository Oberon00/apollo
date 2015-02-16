#ifndef APOLLO_CLASS_INFO_HPP_INCLUDED
#define APOLLO_CLASS_INFO_HPP_INCLUDED APOLLO_CLASS_INFO_HPP_INCLUDED

#include <apollo/config.hpp>
#include <apollo/detail/variadic_pass.hpp>

#include <boost/assert.hpp>
#include <boost/functional/hash.hpp>
#include <boost/type_index.hpp>
#include <apollo/lua_include.hpp>

#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>


namespace apollo { namespace detail {

// Static class ID optimization taken from luabinds registered_class<T> {{{
APOLLO_API std::size_t allocate_class_id(
    boost::typeindex::type_info const& cls);

template <typename T>
struct static_class_id {
    static std::size_t const id;
};

#ifdef BOOST_CLANG
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wglobal-constructors"
#endif
template <typename T>
std::size_t const static_class_id<T>::id = allocate_class_id(
    boost::typeindex::type_id<T>().type_info());
#ifdef BOOST_CLANG
#    pragma clang diagnostic pop
#endif
// }}}

template <typename From, typename To>
void* cast_static(void* src)
{
    return static_cast<To*>(static_cast<From*>(src));
}

using cast_function = void*(*)(void*);

class implicit_ctor {
public:
    // Returns a owning pointer to the object allocated with new.
    virtual void* to(lua_State* L, int idx) = 0;
    virtual ~implicit_ctor() {}
};

struct class_info {
    class_info(
        boost::typeindex::type_info const* rtti_type_, std::size_t static_id_)
        : rtti_type(rtti_type_)
        , static_id(static_id_)
    { }

    // Explicit move and copy operations for MSVC

    class_info(class_info&& rhs)
        : bases(std::move(rhs.bases))
        , rtti_type(std::move(rhs.rtti_type))
        , static_id(std::move(rhs.static_id))
    {}

    class_info(class_info const& rhs)
        : bases(rhs.bases)
        , rtti_type(rhs.rtti_type)
        , static_id(rhs.static_id)
    {}

    class_info& operator= (class_info&& rhs)
    {
        bases = std::move(rhs.bases);
        rtti_type = std::move(rhs.rtti_type);
        static_id = std::move(rhs.static_id);
        return *this;
    }

    class_info& operator= (class_info const& rhs)
    {
        bases = rhs.bases;
        rtti_type = rhs.rtti_type;
        static_id = rhs.static_id;
        return *this;
    }

    // Contains all casts, including indirect bases.
    struct base_relation {
        std::ptrdiff_t offset;
        unsigned n_intermediate_bases;
    };

    std::unordered_map<std::size_t, base_relation> bases;

    boost::typeindex::type_info const* rtti_type;

    std::unordered_map<
        boost::typeindex::type_index,
        std::unique_ptr<implicit_ctor>,
        boost::hash<boost::typeindex::type_index>
    > implicit_ctors;

    std::size_t static_id;
};

using class_info_map = std::unordered_map<
    boost::typeindex::type_index,
    class_info,
    boost::hash<boost::typeindex::type_index>>;

APOLLO_API void* cast_class(
    void* obj, class_info const& cls, std::size_t to);
APOLLO_API unsigned n_class_conversion_steps(
    class_info const& from, std::size_t to);

APOLLO_API class_info_map& registered_classes(lua_State* L);
APOLLO_API class_info* registered_class_opt(
    lua_State* L, boost::typeindex::type_info const& type);

// Only assert()s that the class is registered.
APOLLO_API class_info& registered_class(
    lua_State* L, boost::typeindex::type_info const& type);

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
    auto i_bcinfo = base_infos.find(boost::typeindex::type_id<Base>());
    BOOST_ASSERT_MSG(i_bcinfo != base_infos.end(),
                        "Base classes must be registered before derived ones.");
    binfo.type = &i_bcinfo->second;
    binfo.cast = &cast_static<Derived, Base>;
    bases.push_back(std::move(binfo));
    return int();
}

APOLLO_API class_info make_class_info_impl(
    boost::typeindex::type_info const* rtti_type,
    std::size_t static_id,
    std::vector<base_info>& bases);

template <typename T, typename... Bases>
class_info make_class_info(class_info_map const& base_infos)
{
    (void)base_infos; // Avoid MSVC warning when Bases is empty.
    std::vector<base_info> bases;
    variadic_pass(add_base_helper<T, Bases>(bases, base_infos)...);
    return make_class_info_impl(
        &boost::typeindex::type_id<T>().type_info(),
        static_class_id<T>::id, bases);
}

} } // namespace apollo::detail


#endif // APOLLO_INSTANCE_HOLDER_HPP_INCLUDED
