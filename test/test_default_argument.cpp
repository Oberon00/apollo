#include <apollo/builtin_types.hpp>
#include <apollo/function.hpp>
#include <apollo/raw_function.hpp>
#include <apollo/default_argument.hpp>
#include <apollo/class.hpp>

#include <boost/test/test_tools.hpp>

#include <functional>

static void dummyproc() {}

template <typename T>
static void test_default(lua_State* L, T v)
{
    BOOST_TEST_MESSAGE("at " + boost::typeindex::type_id<T>().pretty_name());
    unsigned n_calls = 0;

    std::function<void(T const&)> const testproc = [v, &n_calls](T const& x) -> void {
        BOOST_CHECK_EQUAL(v, x);
        ++n_calls;
    };

    apollo::push(L, apollo::make_function_with(testproc,
        apollo::push_converter_for<void>(),
        apollo::make_default_arg(T(v)))); // rvalue
    apollo::pcall(L, 0, 0);
    BOOST_REQUIRE_EQUAL(n_calls, 1u);

    // Test that arguments are still picked up when supplied:
    apollo::push(L, apollo::make_function_with(testproc,
        apollo::push_converter_for<void>(),
        apollo::make_default_arg(T())));
    apollo::push(L, v);
    apollo::pcall(L, 1, 0);
    BOOST_REQUIRE_EQUAL(n_calls, 2u);

    // lvalue:
    apollo::push(L, apollo::make_function_with(testproc,
        apollo::push_converter_for<void>(),
        apollo::make_default_arg(v)));
    v = T(); // Should not change default argument.
    apollo::pcall(L, 0, 0);
    BOOST_REQUIRE_EQUAL(n_calls, 3u);
}

template <typename T>
struct testproc_container {
    static T v;
    static unsigned n_calls;
    static void f(T const& x)
    {
        BOOST_CHECK_EQUAL(v, x);
        ++n_calls;
    }
};

template <typename T>
T testproc_container<T>::v;

template <typename T>
unsigned testproc_container<T>::n_calls;

template <typename T>
static void test_default_light(lua_State* L, T v)
{
    BOOST_TEST_MESSAGE("at (light) " +
        boost::typeindex::type_id<T>().pretty_name());
    
    using tp_container = testproc_container<T>;
    tp_container::n_calls = 0;
    tp_container::v = v;

    apollo::push(L, APOLLO_MAKE_LIGHT_FUNCTION(&tp_container::f,
        apollo::push_converter_for<void>(),
        apollo::make_default_arg(T(v)))); // rvalue
    apollo::pcall(L, 0, 0);
    BOOST_REQUIRE_EQUAL(tp_container::n_calls, 1u);

    // Test that arguments are still picked up when supplied:
    apollo::push(L, APOLLO_MAKE_LIGHT_FUNCTION(&tp_container::f,
        apollo::push_converter_for<void>(),
        apollo::make_default_arg(T())));
    apollo::push(L, v);
    apollo::pcall(L, 1, 0);
    BOOST_REQUIRE_EQUAL(tp_container::n_calls, 2u);

    // lvalue:
    apollo::push(L, APOLLO_MAKE_LIGHT_FUNCTION(&tp_container::f,
        apollo::push_converter_for<void>(),
        apollo::make_default_arg(v)));
    v = T(); // Should not change default argument.
    apollo::pcall(L, 0, 0);
    BOOST_REQUIRE_EQUAL(tp_container::n_calls, 3u);
}


namespace {
struct teststruct {
    explicit teststruct(int v_ = 0): v(v_) {}
    bool operator== (teststruct const& other) const {
        return v == other.v;
    }
    int v;
};

static std::ostream& operator<< (std::ostream& os, teststruct const& ts)
{
    return os << "teststruct(" << ts.v << ')';
}

} // anonymous namespace

#include "test_prefix.hpp"

BOOST_AUTO_TEST_CASE(default_arg)
{
    test_default(L, 42);
    test_default(L, 42.0);
    test_default(L, "foo");
    test_default(L, std::string("foo"));
    test_default(L, 'a');
    test_default(L, &dummyproc);

    apollo::register_class<teststruct>(L);
    test_default(L, teststruct(42));
    teststruct ts(42);
    test_default(L, &ts);
}

BOOST_AUTO_TEST_CASE(default_arg_light)
{
    test_default_light(L, 42);
    test_default_light(L, 42.0);
    test_default_light(L, "foo");
    test_default_light(L, std::string("foo"));
    test_default_light(L, 'a');
    test_default_light(L, &dummyproc);

    apollo::register_class<teststruct>(L);
    test_default_light(L, teststruct(42));
    teststruct ts(42);
    test_default_light(L, &ts);
}


#include "test_suffix.hpp"
