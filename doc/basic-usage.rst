Basic usage
===========

.. _sec-basic-converters:

Core primitives: ``push``, ``to`` & friends
---------------------------------------------------

apollo's core functions are ``push`` and ``to``, defined in the
``<converters.hpp>`` header::

   template <typename T, typename... MoreTs>
   int push(lua_State* L, T&& v, MoreTs&&... more);

   template <typename T>
   T to(lua_State* L, int idx);

That is, ``push`` takes an arbitrary number of values (at least one) and pushes
them on the stack of ``L`` in order, returning how many values were pushed on
the Lua stack. ``to`` takes an explicit template type parameter ``T``
and converts the value at the stack of ``L`` at the index ``idx`` to the type
``T``. If the given element is not convertible to ``T``, a
``to_cpp_conversion_error`` is thrown. To check if some stack element is
convertible, apollo provides the ``is_convertible`` function::

   template <typename T>
   bool is_convertible(lua_State* L, int idx);

Since after checking for convertibility the additional check by ``to``
is a unnecessary performance hit, ``unchecked_to`` can be used::

   template <typename T>
   T unchecked_to(lua_State* L, int idx);

This function is like ``to``, but has undefined behavior if the given
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

.. literalinclude:: examples/basic-push-to.cpp

.. warning:: There is a hidden dependency on the ``lua_State`` in the
   ``std::function``: In its destructor it will attempt to ``luaL_unref`` the
   referenced Lua function. Thus, you must not close the state before the
   function's destructor was run. To easily achieve this, this example uses
   ``apollo::closing_lstate``. This utility class closes the state in its
   destructor, which is fine since order of destruction is reverse of
   construction in C++.

Of course, higher-order functions (functions taking or returning functions) are
also supported, as are ``boost::function`` and ``std::function``.


.. _sec-basic-sugar:

Syntactic sugar: ``new_table`` and ``rawset_table``
---------------------------------------------------

Because exporting functions and other things by first pushing them and then
using ``lua_setfield``/``lua_setglobal`` is quite tendious and verbose, apollo
offers convenient syntactic sugar in the ``create_table.hpp`` header::

   implementation-defined rawset_table(lua_State* L, int table_idx);
   implementation-defined new_table(lua_State* L);

These functions return implementation defined proxy objects that support setting
table by chaining ``operator()`` calls. Nested tables are also supported using
``subtable(key)`` and ``end_subtable()``, as are metatables with ``metatable``
and ``end_metatable``.

.. literalinclude:: examples/basic-create_table.cpp


.. _sec-basic-cls:

The basics of using classes
---------------------------

apollo's class system only has the following two primitive functions::

   template <typename T, typename... Bases>
   void register_class(lua_State* L);

   template <typename T>
   void push_class_metatable(lua_State* L);

You first register a class ``T`` for usage in a certain ``lua_State* L`` by
calling ``register_class<T>(L)``. apollo will then wrap objects of this class in
an implementation defined *holder* and push it as a userdata object. To make
this usable from Lua, apollo always sets a per-type metatable that you can
access and change using ``push_class_metatable<T>(L)``. The only field in this
metatable that is preset is the ``__gc`` field that unsets the userdata's
metatable (to prevent access to the destroyed object) and calls ``T``'s
destructor. Otherwise you can use classes together with ``push``,
``to``, ``rawset_table``, etc. like any other type. There's one gotcha
though: For efficiency reasons (and to implement implicit constructors)
``to<T const&>`` and ``to<T>`` do not return the specified type
for classes but instead a implementation defined reference wrapper object. Use
it's ``get()`` member function or ``apollo::unwrap_ref()`` to obtain the
underlying reference. Note that when using implicit constructors, the lifetime
of the underlying class can be bound to the reference wrapper, so better keep it
alive as long as you use the object.

Since there is no direct way to access public data members of classes from Lua,
apollo provides wrapper getter and setter functions that can be used to expose
them in the ``property.hpp`` header. Namely, these are the
``APOLLO_MEMBER_SETTER(my_class::my_member)`` and the corresponding
``APOLLO_MEMBER_GETTER`` macros. They evaluate to a function (yes, a function not
a function pointer!) that takes the member type by ``const&`` and sets it or
gets it by value respectively.

A similar wrapper is provided for  constructors: ``ctor_wrapper<T, Args...>`` is
a function taht returns a ``T`` constructed from ``Args...``.

Let's see some class foo in action:

.. literalinclude:: examples/basic-class.cpp
