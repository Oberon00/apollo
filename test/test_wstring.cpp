// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#include <apollo/builtin_types.hpp>
#include <apollo/raw_function.hpp>
#include <apollo/stack_balance.hpp>
#include <apollo/stack_balance.hpp>
#include <apollo/wstring.hpp>


using apollo::detail::to_wstring;
using apollo::detail::from_wstring;

namespace std {

    inline std::ostream& operator<<(std::ostream& out, const std::wstring& value)
    {
        return out << from_wstring(value.c_str(), value.size());
    }

}
#include "test_prefix.hpp"


static std::wstring tow(std::string s) {
    return to_wstring(s.c_str(), s.size());
}

static std::string fromw(std::wstring s) {
    return from_wstring(s.c_str(), s.size());
}

BOOST_AUTO_TEST_CASE(to_wstring_backend)
{
    BOOST_CHECK_EQUAL(tow(""), L"");
    BOOST_CHECK_EQUAL(tow("abc"), L"abc");
    BOOST_CHECK_EQUAL(tow(u8"Öß"), L"Öß");
}

BOOST_AUTO_TEST_CASE(from_wstring_backend)
{
    BOOST_CHECK_EQUAL(fromw(L""), "");
    BOOST_CHECK_EQUAL(fromw(L"abc"), "abc");
    BOOST_CHECK_EQUAL(fromw(L"Öß"), u8"Öß");
}

#include "test_suffix.hpp"
