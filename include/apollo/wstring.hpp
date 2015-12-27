#ifndef APOLLO_WSTRING_HPP_INCLUDED
#define APOLLO_WSTRING_HPP_INCLUDED

#include <apollo/config.hpp>

#ifndef APOLLO_NO_WSTRING

#include <utility> // move
#include <string>
#include <apollo/lua_include.hpp>
#include <apollo/builtin_types.hpp>

namespace apollo {

namespace detail {

APOLLO_API std::string from_wstring(wchar_t const* s, std::size_t len);
APOLLO_API wcstr_holder to_wstring_tmp(char const* s, std::size_t len);
APOLLO_API std::wstring to_wstring(char const* s, std::size_t len);


inline void push_string(lua_State* L, wchar_t c)
{
    push_string(L, from_wstring(&c, 1));
}

template <std::size_t N>
void push_string(lua_State* L, wchar_t const (&s)[N])
{
    // Don't count null termination.
    size_t const len = N - (s[N - 1] == 0);
    push_string(L, from_wstring(s, len));
}
template <typename T>
inline typename std::enable_if <
    std::is_convertible<T, wchar_t const*>::value
    >::type push_string(lua_State* L, T s)
{
    push_string(L, from_wstring(s, wcslen(s)));
}

inline void push_string(lua_State* L, std::wstring const& s)
{
    push_string(L, from_wstring(s.c_str(), s.size()));
}


class wcstr_holder {
public:
    explicit wcstr_holder(std::wstring&& s)
        : m_s(std::move(s))
    {}

    wcstr_holder(wcstr_holder&& other)
        : m_s(std::move(other.m_s))
    {
        other.m_s = nullptr;
    }

    wcstr_holder(wcstr_holder const&) = delete;
    wcstr_holder& operator= (wcstr_holder const&) = delete;

    // Note: Since C++11, null termination is guaranteed,
    // see http://stackoverflow.com/a/6077274/2128694.
    wchar_t* get() { return &m_s[0]; }

private:
    std::wstring m_s;
};


inline wchar_t* unwrap_ref(detail::wcstr_holder&& wh) { return wh.get(); }

template <typename T>
struct to_string<T, typename std::enable_if<
        std::is_convertible<T, wchar_t const*>::value
    >::type> { // wchar_t*, wchar_t const*, wchar_t[N]
    using type = wcstr_holder;
    static type to(lua_State* L, int idx)
    {
        std::size_t len;
        char const* s = lua_tolstring(L, idx, &len);
        return to_wstring_tmp(s, len);
    }

    static type safe_to(lua_State* L, int idx)
    {
        std::size_t len;
        if (char const* s = lua_tolstring(L, idx, &len))
            return to_wstring_tmp(s, len);
        BOOST_THROW_EXCEPTION(to_cpp_conversion_error());
    }
};

template <>
struct to_string<std::wstring> {
    using type = std::wstring;
    static type to(lua_State* L, int idx)
    {
        std::size_t len;
        char const* s = lua_tolstring(L, idx, &len);
        return to_wstring(s, len);
    }

    static type safe_to(lua_State* L, int idx)
    {
        std::size_t len;
        if (char const* s = lua_tolstring(L, idx, &len))
            return to_wstring(s, len);
        BOOST_THROW_EXCEPTION(to_cpp_conversion_error());
    }
};

template <>
struct to_string<wchar_t> {
    using type = wchar_t;
    static type to(lua_State* L, int idx)
    {
        std::size_t len;
        char const* s = lua_tolstring(L, idx, &len);
        return to_wstring_tmp(s, len).get()[0];
    }

    static type safe_to(lua_State* L, int idx)
    {
        std::size_t len;
        char const* s = lua_tolstring(L, idx, &len);
        if (!s)
            BOOST_THROW_EXCEPTION(to_cpp_conversion_error());
        std::wstring ws = to_wstring(s, len);
        if (ws.size() != 1) {
            BOOST_THROW_EXCEPTION(to_cpp_conversion_error()
                << errinfo::msg("the string must be one wide character long"));
        }

        return ws[0];
    }
};

template <typename T>
struct string_conversion_steps<T, typename std::enable_if<
    std::is_convertible<T, wchar_t const*>::value
    || std::is_same<T, std::wstring>::value
>::type> {
    static unsigned get(lua_State* L, int idx)
    {
        switch(lua_type(L, idx)) {
            case LUA_TSTRING:
                return 1;
            case LUA_TNUMBER:
                return 3;
            default:
                return no_conversion;
        }
        BOOST_UNREACHABLE_RETURN(no_conversion);
    }
};

template <>
struct APOLLO_API string_conversion_steps<wchar_t> {
    static unsigned get(lua_State* L, int idx)
    {
        switch (lua_type(L, idx)) {
            case LUA_TNUMBER:
                return add_conversion_step(string_conversion_steps<char>::get(L, idx));
            case LUA_TSTRING:
                return to_string<std::wstring>::to(L, idx).size() == 1 ? 1 : no_conversion;
            default:
                return no_conversion;
        }
    }
};

} // namespace detail

} // namespace apollo

#endif // APOLLO_NO_WSTRING

#endif // APOLLO_WSTRING_HPP_INCLUDED
