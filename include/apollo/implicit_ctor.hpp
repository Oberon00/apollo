#ifndef APOLLO_IMPLICIT_CTOR_HPP_INCLUDED
#define APOLLO_IMPLICIT_CTOR_HPP_INCLUDED APOLLO_IMPLICIT_CTOR_HPP_INCLUDED

#include <apollo/class.hpp>

namespace apollo {

namespace detail {

template <typename Ctor>
class implicit_ctor_impl: public implicit_ctor {
    using to_t = return_type_of<Ctor>;
    using from_t = signature_element<1, Ctor>;

    // Non-pointer to_t
    void* to_impl(lua_State* L, int idx, std::false_type)
    {
         return new to_t(std::move(m_ctor(
            unwrap_bound_ref(apollo::to<from_t>(L, idx)))));
    }

    // Pointer to_t
    void* to_impl(lua_State* L, int idx, std::true_type)
    {
         return m_ctor(
            unwrap_bound_ref(apollo::to<from_t>(L, idx)));
    }


public:
    implicit_ctor_impl(Ctor ctor): m_ctor(ctor) {}

    void* to(lua_State* L, int idx) override
    {
        return to_impl(L, idx, std::is_pointer<to_t>());
    }

private:
    Ctor m_ctor;
};

} // namespace detail

template <typename From, typename To>
void add_implicit_ctor(lua_State* L, To(*ctor)(From))
{
    auto const& to_tid = boost::typeindex::type_id<
        typename std::remove_pointer<typename detail::remove_qualifiers<
            To>::type>::type>().type_info();
    auto const ltype = detail::lua_type_id<From>::value;
    auto const& from_tid = ltype == LUA_TUSERDATA ?
        boost::typeindex::type_id<
            typename detail::remove_qualifiers<From>::type>().type_info() :
        lbuiltin_typeid(ltype);
    using ctor_f_t = decltype(ctor);
    using ctor_impl_t = detail::implicit_ctor_impl<ctor_f_t>;
    auto& cls = detail::registered_class(L, to_tid);
    std::unique_ptr<ctor_impl_t> ctor_impl(new ctor_impl_t(ctor));
    BOOST_VERIFY_MSG(
        cls.implicit_ctors.emplace(
            boost::typeindex::type_index(from_tid),
            std::move(ctor_impl)).second,
        "A ctor with From -> To already exists.");
}

} // namespace apollo

#endif // APOLLO_IMPLICIT_CTOR_HPP_INCLUDED
