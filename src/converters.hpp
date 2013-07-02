#ifndef CPPLUA_CONVERTERS_HPP_INCLUDED
#define CPPLUA_CONVERTERS_HPP_INCLUDED CPPLUA_CONVERTERS_HPP_INCLUDED

#include <lua.hpp>
#include <type_traits>
#include <boost/assert.hpp>
#include <string>
#include <climits>
#include <functional>
#include <boost/function/function_fwd.hpp>
#include <boost/call_traits.hpp>

#include "error.hpp"
#include <boost/throw_exception.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/errinfo_type_info_name.hpp>

namespace cpplua {


// Lua type constants //

template <typename T, typename Enable=void> // Default to userdata.
struct LuaType { static int const value = LUA_TUSERDATA; };

template <typename T> // Any arithmetic type except bool is a number.
struct LuaType<T,
        typename std::enable_if<std::is_arithmetic<T>::value>::type> {
    static int const value = LUA_TNUMBER;
};

template <> // boolean
struct LuaType<bool> { static int const value = LUA_TBOOLEAN; };

// string
template <>
struct LuaType<char*> { static int const value = LUA_TSTRING; };
template <>
struct LuaType<char const*>: LuaType<char*> {};
template <std::size_t N>
struct LuaType<char[N]>: LuaType<char*> {};
template <>
struct LuaType<std::string>: LuaType<char*> {};

template<> // thread (lua_State*)
struct LuaType<lua_State*> { static int const value = LUA_TTHREAD; };

// function (plain function (pointer), boost and std function templates)
template <typename T>
struct LuaType<T,
        typename std::enable_if<
            std::is_member_function_pointer<T>::value ||
            std::is_function<
                typename std::remove_pointer<T>::type>::value>::type> {
    static int const value = LUA_TFUNCTION;
};

template <typename T>
struct LuaType<std::function<T>>: LuaType<T> {};
template <typename T>
struct LuaType<boost::function<T>>: LuaType<T> {};



template <typename T, typename Enable=void>
struct Converter; // See the converters below for examples.

const unsigned noConversion = UINT_MAX;

template<typename T>
struct ConverterBase {
    typedef T type;
    typedef type to_type;
};

// Number converter //
template<typename T>
struct Converter<T, typename std::enable_if<
    LuaType<T>::value == LUA_TNUMBER>::type>: ConverterBase<T> {
    
    static void push(lua_State* L, T n)
    {
        lua_pushnumber(L, static_cast<lua_Number>(n));
    }

    static unsigned nConversionSteps(lua_State* L, int idx)
    {
        if (lua_type(L, idx) == LUA_TNUMBER) // Actual number.
            return 0;
        if (lua_isnumber(L, idx)) // String convertible to number.
            return 1;
        return noConversion;
    }

    static T toType(lua_State* L, int idx)
    {
#ifdef NDEBUG
        return static_cast<T>(lua_tonumber(L, idx));
#else
        int isnum;
        T n = static_cast<T>(lua_tonumberx(L, idx, &isnum));
        BOOST_ASSERT(isnum);
        return n;
#endif
    }
};


// Boolean converter //
template<>
struct Converter<bool>: ConverterBase<bool> {

    static void push(lua_State* L, bool b)
    {
        lua_pushboolean(L, static_cast<int>(b));
    }

    static unsigned nConversionSteps(lua_State* L, int idx)
    {
        // Convert non-boolean values to bool only as a last resort.
        return lua_isboolean(L, idx) ? 0 : noConversion - 1;
    }

    static bool toType(lua_State* L, int idx)
    {
        // Avoid MSVC "performance warning" about int to bool conversion with
        // ternary operator.
        return lua_toboolean(L, idx) ? true : false;
    }
};


// String converter //

namespace detail {

template <std::size_t N>
inline char const* pushString(lua_State* L, char const (&s)[N])
{
    // Don't count null termination.
    return lua_pushlstring(L, s, N - (s[N - 1] == 0));
}

// Need to make this a template too, so that the array overload is called in
// the appropriate cases. Also can't make it const T* because otherwise the
// call would be ambigous.
template <typename T>
inline char const* pushString(lua_State* L, T s)
{
    return lua_pushstring(L, s);
}

inline char const* pushString(lua_State* L, std::string const& s)
{
    return lua_pushlstring(L, s.c_str(), s.size());
}

template <typename T>
struct ToString { // char*, char const*, char[N]
    typedef char const* type;
    static type toString(lua_State* L, int idx)
    {
        return lua_tostring(L, idx);
    }
};

template <>
struct ToString<std::string> {
    typedef std::string type;
    static type toString(lua_State* L, int idx)
    {
        std::size_t len;
        char const* s = lua_tolstring(L, idx, &len);
        return std::string(s, len);
    }
};

} // namespace detail

template<typename T>
struct Converter<T, typename std::enable_if<
        LuaType<T>::value == LUA_TSTRING>::type>: ConverterBase<T> {
    typedef typename detail::ToString<T>::type to_type;

    static char const* push(
        lua_State* L,
        T const& s)
    {
        return detail::pushString(L, s);
    }

    static unsigned nConversionSteps(lua_State* L, int idx)
    {
        switch(lua_type(L, idx)) {
            case LUA_TSTRING: return 0;
            case LUA_TNUMBER: return 1;
            default: return noConversion;
        }
        BOOST_UNREACHABLE_RETURN(noConversion);
    }

    static to_type toType(lua_State* L, int idx)
    {
        // Avoid MSVC "performance warning" about int to bool conversion with
        // ternary operator.
        return detail::ToString<T>::toString(L, idx);
    }
};

template <typename T>
void push(lua_State* L, T&& v)
{
    Converter<T>::push(L, std::forward<T>(v));
}

template<typename T>
struct Converter<T, typename std::enable_if<
        std::is_const<T>::value ||
        std::is_volatile<T>::value ||
        std::is_reference<T>::value>::type
    >: Converter<
        typename std::remove_reference<
            typename std::remove_cv<T>::type>::type>
{};


template <typename T>
typename Converter<T>::to_type uncheckedToType(lua_State* L, int idx)
{
    return Converter<T>::toType(L, idx);
}

template <typename T>
typename Converter<T>::to_type toType(lua_State* L, int idx)
{
    if (Converter<T>::nConversionSteps(L, idx) == noConversion) {
        BOOST_THROW_EXCEPTION(ToCppConversionError()
            << boost::errinfo_type_info_name(typeid(T).name())
            << errinfo::Msg("conversion from Lua to C++ failed")
            << errinfo::StackIndex(idx)
            << errinfo::LuaState(L));
    }
    return uncheckedToType<T>(L, idx);
}


} // namepace cpplua

#endif // CPPLUA_CONVERTERS_HPP_INCLUDED
