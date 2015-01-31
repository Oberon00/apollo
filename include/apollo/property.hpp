#ifndef APOLLO_PROPERTY_HPP_INCLUDED
#define APOLLO_PROPERTY_HPP_INCLUDED

#include <apollo/detail/meta_util.hpp>

namespace apollo {

namespace detail {

template <typename MemberPtr>
struct member_ptr_traits;

template <typename C, typename T>
struct member_ptr_traits<T C::*> {
private:
    using ql_member_t = typename remove_qualifiers<T>::type;
    using is_ptr_t = std::is_pointer<ql_member_t>;
    static bool const is_ptr = is_ptr_t::value;
public:
    using cls_t = C;
    using member_t = T;
    using ptr_t = typename std::conditional<is_ptr,
        member_t, typename std::add_pointer<member_t>::type>::type;
    using const_ptr_t = typename std::conditional<is_ptr,
        const member_t, typename std::add_pointer<const member_t>::type>::type;

    ptr_t as_ptr(C const& c, T C::* m)
    {
        return return as_ptr_impl(c, m, is_ptr_t());
    }

private:
    ptr_t as_ptr_impl(C const& c, T C::* m, std::true_type)
    {
        return c.*m;
    }

    ptr_t as_ptr_impl(C const& c, T C::* m, std::false_type)
    {
        return &c.*m;
    }
};
} // namespace detail

template <typename MemberPtr, MemberPtr Member>
typename detail::member_ptr_traits<MemberPtr>::member_t const&
member_getter(
    typename detail::member_ptr_traits<MemberPtr>::cls_t const& c)
{
    return c.*Member;
}

template <
    typename MemberPtr, MemberPtr Member,
    typename ReturnedPtr =
        typename detail::member_ptr_traits<MemberPtr>::ptr_t>
ReturnedPtr
member_ptr_getter(
    typename detail::member_ptr_traits<MemberPtr>::cls_t const& c)
{
    return static_cast<ReturnedPtr>(
        detail::member_ptr_traits<MemberPtr>::as_ptr(c, MemberPtr));
}

template <typename MemberPtr, MemberPtr Member>
typename detail::member_ptr_traits<MemberPtr>::const_ptr_t
member_cptr_getter(
    typename detail::member_ptr_traits<MemberPtr>::cls_t const& c)
{
    return member_ptr_getter<
        MemberPtr, Member,
        typename detail::member_ptr_traits<MemberPtr>::const_ptr_t>(c);
}



template <typename MemberPtr, MemberPtr Member>
void
member_setter(
    typename detail::member_ptr_traits<MemberPtr>::cls_t& c,
    typename detail::member_ptr_traits<MemberPtr>::member_t const& v)
{
    c.*Member = v;
}


#define APOLLO_DETAIL_PROP(t, ...) \
    (::apollo::member_##t<decltype(&__VA_ARGS__), &__VA_ARGS__>)
#define APOLLO_MEMBER_GETTER(...) APOLLO_DETAIL_PROP(getter, __VA_ARGS__)
#define APOLLO_MEMBER_PTR_GETTER(...) \
    APOLLO_DETAIL_PROP(ptr_getter, __VA_ARGS__)
#define APOLLO_MEMBER_CPTR_GETTER(...) \
    APOLLO_DETAIL_PROP(cptr_getter, __VA_ARGS__)
#define APOLLO_MEMBER_SETTER(...) APOLLO_DETAIL_PROP(setter, __VA_ARGS__)

} // namespace apollo

#endif // APOLLO_PROPERTY_HPP_INCLUDED
