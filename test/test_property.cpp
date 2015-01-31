#include <apollo/property.hpp>
#include <memory>

#include "test_prefix.hpp"

namespace {

struct foo_cls {
    int fi;
};

struct bar_cls {
    foo_cls* pfoo;
    std::shared_ptr<foo_cls> spfoo;
    foo_cls foo;
};

} // anonymous namespace

BOOST_AUTO_TEST_CASE(properties)
{
    auto get_pfoo = APOLLO_MEMBER_GETTER(bar_cls::pfoo);
    auto get_pfoo_p = APOLLO_MEMBER_PTR_GETTER(bar_cls::pfoo);
    auto get_pfoo_cp = APOLLO_MEMBER_CPTR_GETTER(bar_cls::pfoo);
    auto set_pfoo = APOLLO_MEMBER_SETTER(bar_cls::pfoo);

    auto get_spfoo = APOLLO_MEMBER_GETTER(bar_cls::spfoo);
    auto get_spfoo_p = APOLLO_MEMBER_PTR_GETTER(bar_cls::spfoo);
    //auto get_spfoo_cp = APOLLO_MEMBER_CPTR_GETTER(bar_cls::spfoo); // Fails.
    auto set_spfoo = APOLLO_MEMBER_SETTER(bar_cls::spfoo);

    auto get_foo = APOLLO_MEMBER_GETTER(bar_cls::foo);
    auto get_foo_p = APOLLO_MEMBER_PTR_GETTER(bar_cls::foo);
    auto get_foo_cp = APOLLO_MEMBER_CPTR_GETTER(bar_cls::foo);
    auto set_foo = APOLLO_MEMBER_SETTER(bar_cls::foo);

    foo_cls f {42};
    bar_cls b {&f, nullptr, {7}};

    BOOST_CHECK_EQUAL(get_pfoo(b), b.pfoo);
    BOOST_CHECK_EQUAL(get_pfoo_p(b), b.pfoo);
    BOOST_CHECK_EQUAL(get_pfoo_cp(b), b.pfoo);

    set_pfoo(b, nullptr);
    BOOST_CHECK_EQUAL(b.pfoo, static_cast<foo_cls*>(nullptr));

    BOOST_CHECK_EQUAL(get_spfoo(b), b.spfoo);
    BOOST_CHECK_EQUAL(get_spfoo_p(b), b.spfoo);

    auto new_foo = std::make_shared<foo_cls>();
    new_foo->fi = -42;
    set_spfoo(b, new_foo);
    BOOST_CHECK_EQUAL(b.spfoo, new_foo);

    static_assert(std::is_same<
            decltype(get_foo(b)),
            foo_cls const&
        >::value, "");
    static_assert(std::is_same<
            decltype(get_foo_p(b)),
            foo_cls*
        >::value, "");
    static_assert(std::is_same<
            decltype(get_foo_cp(b)),
            foo_cls const*
        >::value, "");
    BOOST_CHECK_EQUAL(get_foo(b).fi, b.foo.fi);
    BOOST_CHECK_EQUAL(get_foo_p(b), &b.foo);
    BOOST_CHECK_EQUAL(get_foo_cp(b), &b.foo);

    set_foo(b, {3});
    BOOST_CHECK_EQUAL(b.foo.fi, 3);
}

#include "test_suffix.hpp"
