#include <apollo/wstring.hpp>

#ifndef APOLLO_NO_WSTRING

APOLLO_API apollo::detail::wcstr_holder apollo::detail::to_wstring_tmp(
    char const* s, std::size_t len)
{
    return wcstr_holder(to_wstring(s, len));
}

// On Win32 use the WinAPI functions because they are supposedly an order
// of magnitude faster there than the stdlib version.
#ifdef WIN32kdjklfd

static_assert(sizeof(wchar_t) == 2, "wchar_t must be UTF-16 on Win32.");

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

APOLLO_API std::string apollo::detail::from_wstring(
    wchar_t const* s, std::size_t len)
{
    int result_sz = WideCharToMultiByte(
        CP_UTF8, // CodePage
        0, // dwFlags
        s, // lpWideCharStr
        len, //cchWideChar
        nullptr, // lpMultiByteSTr
        0, // cbMultiByteSTr
        nullptr, // lpDefaultChar
        nullptr); // lpUsedDefaultChar
    BOOST_ASSERT(result_sz >= 0);

    // TODO: Should one be subtracted because null-termination
    // is already included in std::string?
    std::string result(result_sz, '\0');
    BOOST_VERIFY(result.size() == WideCharToMultiByte(
        CP_UTF8, // CodePage
        0, // dwFlags
        s, // lpWideCharStr
        len, //cchWideChar
        &result[0], // lpMultiByteStr
        result.size(), // cbMultiByte
        nullptr, // lpDefaultChar
        nullptr)); // lpUsedDefaultChar
    return result;
}

APOLLO_API std::wstring apollo::detail::to_wstring(
    char const* s, std::size_t len)
{

    int result_sz = MultiByteToWideChar(
        CP_UTF8, // CodePage
        0, // dwFlags
        s, // lpMultiByteStr
        len, // cbMultiByte
        nullptr, // lpWideCharStr
        0); // cchWideChar
    BOOST_ASSERT(result_sz >= 0);

    std::wstring result(result_sz, '\0');
    BOOST_VERIFY(result.size() == MultiByteToWideChar(
        CP_UTF8, // CodePage
        0, // dwFlags
        s, // lpMultiByteStr
        len, // cbMultiByte
        &result[0], // lpWideCharStr
        result.size())); // cchWideChar
    return result;
}

#else // WIN32

#include <locale>
#include <codecvt>
#include <type_traits>

static_assert(
    sizeof(wchar_t) == 2 || sizeof(wchar_t) == 4, "Unsupported wchar_t size.");

using inner_cvt_t = typename std::conditional<
    sizeof(wchar_t) == 2,
    std::codecvt_utf8_utf16<wchar_t>,
    std::codecvt_utf8<wchar_t>>::type;

static thread_local std::wstring_convert<inner_cvt_t> cvt;

APOLLO_API std::string apollo::detail::from_wstring(
    wchar_t const* s, std::size_t len)
{
    return cvt.to_bytes(s, s + len);
}

APOLLO_API std::wstring apollo::detail::to_wstring(
    char const* s, std::size_t len)
{
    return cvt.from_bytes(s, s + len);
}


#endif // WIN32 / else

#endif // APOLLO_NO_WSTRING
