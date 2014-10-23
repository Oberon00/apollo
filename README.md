Title: Apollo Readme  

# apollo
Convert anything from Lua to C++ and back!

[![Build Status](https://travis-ci.org/Oberon00/apollo.svg?branch=master)](https://travis-ci.org/Oberon00/apollo)

This library is in early pre-alpha, I do *not* recommend to use it now: it lacks
documentation and DLL/so-Support. Also, API-breaking changes are to be expected.


## Features

* Access to primitives (`push`, `from_stack<T>`, `is_convertible<T>`) allows you
  to build the abstractions you need.
* Functions are first-class objects: You can push a function to Lua and retrieve
  it back. When you pushed it as a function pointer, you can retrieve it back as
  function pointer, std::function and boost::function are also fully supported
  (they can even hold functions written in Lua!)
* If you don't need to retrieve your functions back from Lua, apollo offers
  function wrappers that have *zero overhead* compared to a hand-written
  wrapper (`to_raw_function<F, FVal>()`).
* Supports converting classes with multiple bases to any unambiguous one.
* Useful Lua programming utilities like `registry_reference`, `stack_balance` or
  `exceptions_to_lua_errors` and the complementary throwing `pcall`.

Also, the implementation uses variadic templates instead of preprocessor
metaprogramming for faster builds and better auto-completion support.


## Requirements

* A up-to-date, reasonably C++11-compliant compiler and standard library. Tested
  with Clang 3.4 + libstdc++-4.8 (see Travis CI) and MSVC 12 (2013).
* [Lua 5.2](http://lua.org) (5.3 untested, 5.1 lacks required APIs)
* [Boost 1.56](http://boost.org) (Currently, older versions might also work but
  it is planned to use Type Index for better DLL/so support)
* A reasonably recent [CMake](http://cmake.org) for building.

