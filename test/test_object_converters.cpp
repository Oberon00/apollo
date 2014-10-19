#include <apollo/class.hpp>
#include <apollo/lapi.hpp>
#include <apollo/function.hpp>

#include "test_prefix.hpp"

namespace {

struct foo_cls {
    int i;
    unsigned n_copies, n_moves;
    static unsigned n_destructions;
    static int last_k;

    foo_cls(int i_): i(i_), n_copies(0), n_moves(0) {}
    foo_cls(foo_cls const& other):
        i(other.i), n_copies(other.n_copies + 1), n_moves(other.n_moves)
    {}

    foo_cls(foo_cls&& other):
        i(other.i), n_copies(other.n_copies), n_moves(other.n_moves + 1)
    {}

    virtual void memfn(int k)
    {
        foo_cls::last_k = k + i;
    }

    virtual ~foo_cls() { ++foo_cls::n_destructions; }
};

unsigned foo_cls::n_destructions = 0;
int foo_cls::last_k = 0;

struct bar_cls {
    int b;
    bar_cls(): b(111) {}
    virtual ~bar_cls() {}
};

struct derived_cls: foo_cls, bar_cls {
    int j;
    derived_cls(int i_, int j_): foo_cls(i_), j(j_) {}

    void memfn(int k) override
    {
        foo_cls::last_k = k + j;
    }
};

} // anonymous namespace

BOOST_AUTO_TEST_CASE(object_converter)
{
    foo_cls::n_destructions = 0;

    // Check that subsequent calls retrieve the same object as the first one
    BOOST_REQUIRE_EQUAL(
        &apollo::detail::registered_classes(L),
        &apollo::detail::registered_classes(L));

    BOOST_CHECK_EQUAL(lua_gettop(L), 0);
    apollo::register_class<foo_cls>(L);
    apollo::register_class<bar_cls>(L);
    BOOST_CHECK_EQUAL(lua_gettop(L), 0);
    BOOST_REQUIRE_EQUAL(apollo::detail::registered_classes(L).size(), 2u);

    { // Scope for all foo_cls:
        foo_cls foo(42); // Dtor #1

        apollo::push(L, foo); // Dtor #2
        {
            auto& pushed_foo = apollo::from_stack<foo_cls&>(L, -1);
            BOOST_CHECK_EQUAL(pushed_foo.i, 42);
            BOOST_CHECK_EQUAL(pushed_foo.n_copies, 1u);
            BOOST_CHECK_EQUAL(pushed_foo.n_moves, 0u);
        }
        BOOST_CHECK_EQUAL(apollo::from_stack<foo_cls*>(L, -1)->i, 42);
        BOOST_CHECK_EQUAL(apollo::from_stack<foo_cls>(L, -1).i, 42); // Dtor #3
        auto& outer_ptr = apollo::from_stack<std::unique_ptr<foo_cls>&>(L, -1);
        BOOST_CHECK_EQUAL(outer_ptr->i, 42);
        apollo::from_stack<foo_cls&>(L, -1).i = 7;
        BOOST_CHECK_EQUAL(apollo::from_stack<foo_cls*>(L, -1)->i, 7);
        BOOST_CHECK(!apollo::is_convertible<bar_cls&>(L, -1));
        outer_ptr.reset();
        BOOST_CHECK(!apollo::from_stack<foo_cls*>(L, -1));
        BOOST_CHECK(!apollo::is_convertible<foo_cls&>(L, -1));
        lua_pop(L, 1);

        apollo::push(L, &foo);
        BOOST_CHECK_EQUAL(&apollo::from_stack<foo_cls&>(L, -1), &foo);
        lua_pop(L, 1);

        apollo::push(L, std::move(foo)); // Dtor #4
        {
            auto& pushed_foo = apollo::from_stack<foo_cls&>(L, -1);
            BOOST_CHECK_EQUAL(pushed_foo.i, 42);
            BOOST_CHECK_EQUAL(pushed_foo.n_copies, 0u);
            BOOST_CHECK_EQUAL(pushed_foo.n_moves, 1u);
        }
        lua_pop(L, 1);

        std::shared_ptr<foo_cls> pfoo(new foo_cls(7)); // Dtor #5

        apollo::push(L, pfoo);
        BOOST_CHECK_EQUAL(&apollo::from_stack<foo_cls&>(L, -1), pfoo.get());
        BOOST_CHECK_EQUAL(
            apollo::from_stack<std::shared_ptr<foo_cls>>(L, -1).get(),
            pfoo.get());
        BOOST_CHECK(!apollo::is_convertible<std::unique_ptr<foo_cls>>(L, -1));
        BOOST_CHECK_EQUAL(apollo::from_stack<foo_cls*>(L, -1), pfoo.get());
        lua_pop(L, 1);
    }

    lua_gc(L, LUA_GCCOLLECT, 0);
    lua_gc(L, LUA_GCCOLLECT, 0); // Just to be sure, collect a second time.
    BOOST_CHECK_EQUAL(foo_cls::n_destructions, 5u);
}


BOOST_AUTO_TEST_CASE(derived_converter)
{
    apollo::register_class<foo_cls>(L);
    apollo::register_class<bar_cls>(L);
    apollo::register_class<derived_cls, foo_cls, bar_cls>(L);

    derived_cls drv(21, 42);
    auto const check_derived_ok = [&drv, this] () -> void {
        BOOST_REQUIRE_EQUAL(&apollo::from_stack<derived_cls&>(L, -1), &drv);
        BOOST_CHECK_EQUAL(&apollo::from_stack<foo_cls&>(L, -1), &drv);
        BOOST_CHECK_EQUAL(&apollo::from_stack<bar_cls&>(L, -1), &drv);
        BOOST_CHECK_EQUAL(apollo::from_stack<bar_cls&>(L, -1).b, 111);
        BOOST_CHECK_EQUAL(apollo::from_stack<foo_cls&>(L, -1).i, 21);
        BOOST_CHECK_EQUAL(apollo::from_stack<derived_cls&>(L, -1).j, 42);
    };

    BOOST_TEST_MESSAGE("derived_cls*");
    apollo::push(L, &drv);
    check_derived_ok();
    lua_pop(L, 1);

    apollo::push(L, static_cast<foo_cls*>(&drv));
    BOOST_CHECK(!apollo::is_convertible<derived_cls*>(L, -1));
    BOOST_CHECK(!apollo::is_convertible<bar_cls*>(L, -1));
    BOOST_CHECK_EQUAL(apollo::from_stack<foo_cls*>(L, -1), &drv);
    lua_pop(L, 1);

    apollo::push(L, static_cast<bar_cls*>(&drv));
    BOOST_CHECK(!apollo::is_convertible<derived_cls*>(L, -1));
    BOOST_CHECK(!apollo::is_convertible<foo_cls*>(L, -1));
    BOOST_CHECK_EQUAL(apollo::from_stack<bar_cls*>(L, -1), &drv);
    lua_pop(L, 1);
}

BOOST_AUTO_TEST_CASE(memfns)
{
    apollo::register_class<foo_cls>(L);
    apollo::register_class<bar_cls>(L);
    apollo::register_class<derived_cls, foo_cls, bar_cls>(L);

    apollo::push(L, &foo_cls::memfn);
    lua_pushvalue(L, -1);
    apollo::push(L, foo_cls(2), 40);
    apollo::pcall(L, 2, 0);
    BOOST_CHECK_EQUAL(foo_cls::last_k, 42);

    apollo::push(L, derived_cls(2, -3), 40);
    apollo::pcall(L, 2, 0);
    BOOST_CHECK_EQUAL(foo_cls::last_k, 37);
}

#include "test_suffix.hpp"
