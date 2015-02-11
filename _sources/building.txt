Building apollo
===============

apollo requires that you have installed the following libraries:

- Boost_ in version 1.56 (later versions may/should also work).
- Lua_ in version 5.1 to 5.3.


CMake_ in a reasonably recent version is used as the build system.

.. _Boost: http://www.boost.org/
.. _Lua: http://www.lua.org/
.. _CMake: http://www.cmake.org/


Especially on Windows, make sure that any environment variables neccessary to
let CMake find the librarys are set correctly: ``BOOST_ROOT`` for Boost and
``LUA_DIR`` for Lua.

As an alternative to setting these variables you can also install the libraries
to the (CMake) standard locations or add the containing directories to
``CMAKE_PREFIX_PATH``.

What's left to do is a standard CMake build. The following contains nothing
special, so if you are familiar with CMake, you can just skip the rest.

Unix-like
---------

.. highlight:: sh

Navigate to the apollo root directory (with the ``include`` and ``src``
subfolders) in your shell, then execute the following commands::

    mkdir build # Name basically arbitrary, but build is already in .gitignore
    cd build    #
    cmake .. # Use the -i flag to see build options (e.g. debug build).
    make
    sudo make install # Optional.
    
Visual Studio
-------------

Use the Visual Studio command prompt as your shell and do as in Unix, with the
following modifications: You may need to add ``-G "Visual Studio 12"`` (or 13
for Visual Studio 2015) to the cmake command line. Then use ``msbuild
ALL_BUILD.vcxproj`` instead of ``make`` and ``msbuild INSTALL.vcxproj`` instead
of ``sudo make install`` if you want to install apollo. As in Unix, you
will need administrative rights for the latter, but because Windows has no
``sudo`` equivalent, you may need to e.g.  launch a new VS command prompt as
administrator. Also, use the ``/p:Configuration=RelWithDebInfo`` (or
``=Release``) option to build the release version instead of the default Debug
configuration.


Configuring your compiler for using apollo
==========================================

Nothing special: You need to add the ``include`` directory to your compiler's
include paths and link the apollo library for release and the apollo-d library
for debug builds. For Boost, no addidional libraries need to be linked, just
making the includes available is enough. Of course you will also need to add
Lua's include directory and link the library.

If you use CMake, you can use ``FindApollo.cmake``: Make it available to CMake
by either altering the ``CMAKE_MODULE_PATH`` CMake variable or copying it to a
standard location. Then you can use ``find_package(apollo REQUIRED)`` and it
will look in standard locations and in the ones specified by the ``APOLLO_DIR``
environment or CMake variable. It then makes ``APOLLO_INCLUDE_DIRS`` and
``APOLLO_LIBRARIES`` available.
