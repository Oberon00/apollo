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


#endif // APOLLO_CONFIG_HPP_INCLUDED
