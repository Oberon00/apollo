// Part of the apollo library -- Copyright (c) Christian Neumüller 2015
// This file is subject to the terms of the BSD 2-Clause License.
// See LICENSE.txt or http://opensource.org/licenses/BSD-2-Clause

#ifndef APOLLO_CONFIG_HPP_INCLUDED
#define APOLLO_CONFIG_HPP_INCLUDED APOLLO_CONFIG_HPP_INCLUDED

#include <apollo/build_information.hpp>
#include <boost/config.hpp>

#define APOLLO_VERSION_MAJOR 0
#define APOLLO_VERSION_MINOR 0
#define APOLLO_VERSION_PATCH 0

#ifndef APOLLO_API
#   ifdef APOLLO_DYNAMIC_LINK
#       if defined (BOOST_WINDOWS)
#           ifdef APOLLO_BUILDING
#               define APOLLO_API __declspec(dllexport)
#           else
#               define APOLLO_API __declspec(dllimport)
#           endif
#       elif defined (__CYGWIN__)
#           ifdef APOLLO_BUILDING
#               define APOLLO_API __attribute__ ((dllexport))
#           else
#               define APOLLO_API __attribute__ ((dllimport))
#           endif
#       elif defined(__GNUC__) && __GNUC__ >= 4
#           define APOLLO_API __attribute__ ((visibility("default")))
#       endif
#   else
#       define APOLLO_API
#   endif
#endif

#if !defined(APOLLO_NO_WSTRING) && (            \
        defined(BOOST_NO_INTRINSIC_WCHAR_T)     \
        || !defined(_WIN32)                     \
        && (                                    \
            defined(BOOST_NO_CXX11_HDR_CODECVT) \
            || defined(BOOST_NO_0X_HDR_CODECVT)))
#   define APOLLO_NO_WSTRING
#endif

#ifdef BOOST_MSVC
#   define APOLLO_DETAIL_PUSHMSWARN(id) \
        __pragma(warning(push))         \
        __pragma(warning(disable:id))
#   define APOLLO_DETAIL_POPMSWARN __pragma(warning(pop))
#else
#   define APOLLO_DETAIL_PUSHMSWARN(id)
#   define APOLLO_DETAIL_POPMSWARN
#endif

#define APOLLO_DETAIL_CONSTCOND_BEGIN APOLLO_DETAIL_PUSHMSWARN(4127)
#define APOLLO_DETAIL_CONSTCOND_END APOLLO_DETAIL_POPMSWARN

#endif // APOLLO_CONFIG_HPP_INCLUDED
