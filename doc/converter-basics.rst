.. _sec-converters-basic:

Converters: Basics
==================

The basic facility in apollo is the *converter*. A converter is a class that
knows how to push an object of a certain type on the Lua stack or how to
retrieve it from there. apollo uses the converters that are specializations of
the ``apollo::converter<T>`` template by default. That's why there are headers
for each type: they define the appropriate specializations.

The converters are used not only by the functions below, but also to retrieve
the arguments of C++ functions that were pushed to Lua and to push their return
values.

A *PushConverter* is a converter used for pushing, a *PullConverter* is used
for retrieving values from the Lua stack.

.. seealso::

   :ref:`sec-basic-converters`
      A short tutorial that demonstrates the most important features.

   :ref:`sec-converters-advanced`
      More on converters.


Converter functions
-------------------

Header::

   #include <apollo/converters.hpp>

.. _f-push:

``push()``
^^^^^^^^^^

::

   template <typename T, typename... MoreTs>
   int push(lua_State* L, T&& v, MoreTs&&... more);

Pushes ``v`` (and optionally ``more``) on the stack of ``L`` using ``T``'s
default PushConverter. Returns how many values were pushed (this is not
necessarily the number of arguments, since some C++ values could be pushed as no
or multiple Lua values).

If multiple arguments are pushed, they are pushed in direct order (as required
for using this function with ``lua_call``), i.e. the first argument is pushed
first, then the second is pushed on top of it and so on, until the last one
becomes the new stack top.


.. _f-from_stack:

``from_stack()``
^^^^^^^^^^^^^^^^

::

   template <typename /* explicit */ T> // 1
   to_type_of<pull_converter_for<T>> from_stack(lua_State* L, int idx);

   template <typename T> // 2
   to_type_of<pull_converter_for<T>> from_stack(
       lua_State* L, int idx, T&& fallback)

Return the value at index ``idx`` on the stack of ``L`` converted to ``T``
using ``T``'s default PullConverter.

The actual return type is usually exactly ``T`` but converters can substitute
their own. This is currently used only by the converter for classes and is
documented there.

If the specified Lua value is not convertible to ``T`` (as determined with
:ref:`f-is_convertible`), a ``to_cpp_conversion_error`` is thrown for overload 1
and ``fallback`` is returned for overload 2.


.. _f-is_convertible:

``is_convertible()``
^^^^^^^^^^^^^^^^^^^^

::

   template <typename /* explicit */ T>
   bool is_convertible(lua_State* L, int idx);

Returns whether the value at index ``idx`` on the stack of ``L`` can be
converted to ``T`` using ``T``'s default PullConverter.



.. _f-unchecked_from_stack:

``unchecked_from_stack()``
^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   template <typename /* explicit */ T>
   to_type_of<pull_converter_for<T>> unchecked_from_stack(lua_State* L, int idx);


Just like the first overload of :ref:`f-from_stack`: Return the value at index
``idx`` on the stack of ``L`` converted to ``T`` using ``T``'s default
PullConverter.

The only difference is the error handling: While :ref:`f-from_stack` throws an
exception, this function has undefined behavior (e.g. exceptions or segfaults)
if the value is not convertible.

.. warning:: Use this function only if you have already checked with
   :ref:`f-is_convertible` or if you can otherwise be 100% sure that the
   specified Lua value is convertible to ``T``.


Lua builtin types
=================

Header::

   #include <apollo/builtin_types.hpp>

The following table shows how C++ types are converted to Lua types when pushed /
as which C++ type a Lua type can be retrieved:

===================================================  ========
C++ types                                            Lua type
===================================================  ========
all arithmetic types (except ``bool`` and ``char``)  number
enums                                                number
``std::string``, ``char[]``, ``char*``, ``char``     string
``bool``                                             boolean
===================================================  ========

There are a few things to note:

``char``
   A Lua string can only be converted to ``char`` if it is exactly one byte
   long. The result will then be the only byte in this string.

   Since numbers are implicitly converted to strings in Lua, apollo follows this
   convention and accepts integer numbers from 0 to 9. The result is the single
   digit of the number.

``unsigned char``, ``signed char``
   Although plain ``char`` is treated as a string, both ``signed char`` and
   ``unsigned char`` (these C++ types are both always distinct from ``char``)
   are treated as numbers.

string
   Pushing character arrays or ``std::string`` will preserve embedded
   null-bytes, while ``char*`` cannot. For ``std::string`` the ``size()`` is
   used for the length, for ``char[N]``, the length is ``N - 1`` if the last
   character is a null-byte and ``N`` otherwise.

   .. warning:: Be careful with that if you push a local
      string buffer that you want to be null-terminated::

         char buf[512];
         std::scanf("%511s", buf); 
         apollo::push(L, buf); // Wrong! String will contain junk after '\0'!
         apollo::push(L, &buf); // Correct! String will end at '\0'.

Lua 5.3 only: integer vs. number
   apollo will push an integral C++ type as integer if its value fits inside a
   ``lua_Integer``, or as a number otherwise. For retrieving a number that is
   an integer in Lua as a C++ integral type, apollo will always use
   ``lua_tointeger``.

   Enums are always pushed and retrieved as integers, without checking if the
   enumerator value fits, as enumerator values tend to be not that big.
