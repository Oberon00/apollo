Title: Apollo Readme  

# apollo
Convert anything from Lua to C++ and back!

[![Build Status](https://travis-ci.org/Oberon00/apollo.svg?branch=master)](https://travis-ci.org/Oberon00/apollo)

This library is in a quite usable alpha state. It can be built as static library
or DLL/.so and works with Lua 5.2 to 5.3. Documentation is here:

**http://oberon00.github.io/apollo/**


## Features

* Functions are first-class objects: You can push a function to Lua and retrieve
  it back. When you pushed it as a function pointer, you can retrieve it back as
  function pointer, std::function and boost::function are also fully supported
  (they can even hold functions written in Lua!)
* If you don't need to retrieve your functions back from Lua, apollo offers
  function wrappers that have *zero overhead* compared to a hand-written
  wrapper (`to_raw_function<F, FVal>()`).
* Supports converting classes with multiple bases to any unambiguous one
  (virtual inheritance is *not* supported).
* Access to primitives (`push`, `from_stack<T>`, `is_convertible<T>`) allows you
  to build the abstractions you need or hand-write a custom wrapper if the
  included high-level abstractions don't fit your needs.
* Useful Lua programming utilities like `registry_reference`, `stack_balance` or
  `exceptions_to_lua_errors` and the complementary throwing `pcall`.

Also, the implementation uses variadic templates instead of preprocessor
metaprogramming for faster builds and better auto-completion support.


## Requirements

* A up-to-date, reasonably C++11-compliant compiler and standard library. Tested
  with gcc 4.8, Clang 3.4 with libstdc++-4.8 (see Travis CI) and MSVC 12 (2013).
* [Lua](http://lua.org) in version 5.1, 5.2 or 5.3.
  be supported in the future to allow the use of [LuaJIT](http://luajit.org/).
* [Boost 1.56](http://boost.org) Presently used modules are Assert, Config,
  Core, Exception and TypeIndex. For the tests, Preprocessor and Test are used
  additionally.
* A reasonably recent [CMake](http://cmake.org) for building.
