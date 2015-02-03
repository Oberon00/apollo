Basic usage
===========

apollo's core functions are ``push`` and ``from_stack``, defined in the
``<converters.hpp>`` header::

   template <typename T, typename... MoreTs>
   void push(lua_State* L, T&& v, MoreTs&&... more);


   template <typename T>
   T from_stack(lua_State* L, int idx);

That is, ``push`` takes an arbitrary number of values (at least one) and pushes
them on the stack of ``L`` in order. ``from_stack`` takes an explicit template
type parameter ``T`` and converts the value at the stack of ``L`` at the index
``idx`` to the type ``T``. If the given element is not convertible to ``T``, a
``to_cpp_conversion_error`` is thrown. To check if some stack element is
convertible, apollo provides the ``is_convertible`` function::

   template <typename T>
   bool is_convertible(lua_State* L, int idx);

Since after checking for convertibility the additional check by ``from_stack``
is a unnecessary performance hit, ``unchecked_from_stack`` can be used::

   template <typename T>
   T unchecked_from_stack(lua_State* L, int idx);

This function is like ``from_stack``, but has undefined behavior if the given
element is not convertible to ``T``.

To make these functions actually work, you need to include headers for the types
you want to use (each of these includes ``converters.hpp`` already, so there is
no need to do this manually):

========================== =====================
Type                       Header
========================== =====================
builtin types (``int``, …) ``builtin_types.hpp``
C++ functions              ``function.hpp``
user-defined types         ``class.hpp``
========================== =====================

Let's try this out:

.. literalinclude:: examples/basic-push-from_stack.cpp

.. warning:: There is a hidden dependency on the ``lua_State`` in the
   ``std::function``: In its destructor it will attempt to ``luaL_unref`` the
   referenced Lua function. Thus, you must not close the state before the
   function's destructor was run. To easily achieve this, this example uses
   ``apollo::closing_lstate``. This utility class closes the state in its
   destructor, which is fine since order of destruction is reverse of
   construction in C++.

Of course, higher-order functions (functions taking or returning functions) are
also supported, as are ``boost::function`` and ``std::function``.
