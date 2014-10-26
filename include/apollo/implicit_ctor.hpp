#ifndef APOLLO_IMPLICIT_CTOR_HPP_INCLUDED
#define APOLLO_IMPLICIT_CTOR_HPP_INCLUDED APOLLO_IMPLICIT_CTOR_HPP_INCLUDED

#include <apollo/class.hpp>

#include <boost/any.hpp>

namespace apollo {

namespace detail {

template <typename From, typename Ctor>
class implicit_ctor_impl: public implicit_ctor {
public:
    implicit_ctor_impl(Ctor ctor): m_ctor(ctor) {}

    boost::any from_stack(lua_State* L, int idx) override
    {
        return m_ctor(std::forward<From>(apollo::from_stack<From>(L, idx)));
    }

private:
    Ctor m_ctor;
};

} // namespace detail

template <typename From, typename To>
void add_implicit_ctor(lua_State* L, To(*ctor)(From))
{
    auto const& to_tid = typeid(
        typename detail::remove_qualifiers<To>::type);
    auto const ltype = detail::lua_type_id<From>::value;
    auto const& from_tid = ltype == LUA_TUSERDATA ?
        typeid(typename detail::remove_qualifiers<From>::type) :
        lbuiltin_typeid(ltype);
    using ctor_f_t = decltype(ctor);
    using ctor_impl_t = detail::implicit_ctor_impl<From, ctor_f_t>;
    auto& cls = detail::registered_class(L, to_tid);
    std::unique_ptr<ctor_impl_t> ctor_impl(new ctor_impl_t(ctor));
    BOOST_VERIFY_MSG(
        cls.implicit_ctors.insert(
            {std::type_index(from_tid), std::move(ctor_impl)}).second,
        "A ctor with From -> To already exists.");
}

} // namespace apollo

#endif // APOLLO_IMPLICIT_CTOR_HPP_INCLUDED
