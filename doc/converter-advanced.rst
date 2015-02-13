.. _sec-converters-advanced:

Converters: Advanced topics
===========================

.. seealso:: Required reading: :ref:`sec-converters-basic`

Converter concepts
------------------

The two concepts of PushConverter and PullConverter, as described below form the
foundation of apollo. They can be used directly with the ``_with`` variants of
the basic converter functions (see :ref:`sec-converters-advanced-fns` below) or
they can be named in a special way to be used as default converters by the basic
converter functions (see :ref:`sec-converters-advanced-default`).


Converter
^^^^^^^^^

A Converter for a type ``T`` must be MoveConstructible and Destructible. It must
expose ``T`` as a member type alias named ``type``.


PushConverter
^^^^^^^^^^^^^

PushConverter is a refinement of the Converter concept.

A PushConverter for a type ``T`` (where ``T`` is always cvr-unqualified) defines
how to push a ``T`` onto the Lua stack.

It needs to implement only one member function: ``push`` must be callable with
arguments ``lua_State*`` and ``T`` and push the passed ``T`` onto the passed
``lua_State*``'s stack, returning as an ``int`` how many values were pushed
(this will usually be 1, but can also be 0 or more than 1). The ``push``
function may be non-static.


PullConverter
^^^^^^^^^^^^^

PullConverter is a refinement of the Converter concept.

A PushConverter for ``T`` (where ``T`` is always withouth top-level cv) defines
how to retrieve a ``T`` from the Lua stack.

A member type(def) ``to_type`` must be provided that specifies the return type
of ``from_stack``.

A data member ``n_consumed`` must be provided if the ``n_conversion_steps`` or
``from_stack`` member functions does not implement the overload having the
``int* next_idx`` parameter. It will then be used to determine what stack
elements have been processed by this converter (e.g. when converting function
arguments, the next argument's converter will receive ``idx + n_consumed`` as
its ``idx`` argument).

It needs to implement two member functions (both can be non-static and must not
change the number of elements on the Lua stack):

``n_conversion_steps`` must have a prototype compatible to either of::

   unsigned n_conversion_steps(lua_State* L, int idx); // 1
   unsigned n_conversion_steps(lua_State* L, int idx, int* next_idx); // 2

This function must return ``no_conversion`` (defined in
``<apollo/converters_fwd.hpp>`` which is always included by
``<apollo/converters.hpp>``) if the stack element at index ``idx`` on ``L`` is
not convertible to ``T``. Otherwise it should return a number in the inclusive
range from ``0`` to ``no_conversion - 1``, where ``0`` indicates a perfect
conversion (such as the conversion from ``string`` to ``char const*`` and
``no_conversion - 1`` indicates that the conversion should only be used as a
fallback (such as the conversion from userdata to ``bool``). This
information is used e.g. by :ref:`f-is_convertible`, the grading is used for
(currently undocumented) overload sets. The second overload must set
``*next_idx`` to the first stack index after ``idx`` that will not be used by
``from_stack`` if ``next_idx != nullptr``.

``from_stack`` must have a prototype compatible to either of::

   to_type from_stack(lua_State* L, int idx); // 1
   to_type from_stack(lua_State* L, int idx, int* next_idx); // 2

It must have the same form as ``n_conversion_steps`` (if one has the
``next_idx`` parameter, the other must too). This function must return the value
at stack index ``idx`` of ``L`` converted to a type for which
``unwrap_bound_ref`` returns a ``T``-compatible type (usually this type will be
just ``T``). It must only be called if ``n_conversion_steps`` would return a
value ≠ ``no_conversion`` (although ``n_conversion_steps`` will not necessarily
be called), thus it does not need to check for convertibility (the behavior may
be undefined if the precondition is violated).  The value on the Lua stack
should be left untouched. The semantics of ``next_idx`` are the same as for
``n_conversion_steps``.


FullConverter
^^^^^^^^^^^^^

A type fulfills the FullConverter concept if it is both a PushConverter and a
PullConverter.


.. _sec-converters-advanced-default:

Default converter protocol
--------------------------

A Converter used as default converter must be additionally DefaultConstructible.
It must be specialization of the following ``struct`` template::

   template <typename T, typename Enable=void>
   struct converter;

If it is not a PushConverter, ``push`` and related functions
(including pushing a function with ``T`` as return type) will not work, except
if ``T`` is cvr-qualified, because then the converter will never be selected for
pushing.
Similarly, if it is not a PullConverter, ``from_stack``, ``is_convertible`` and related
functions (including pushing a function with ``T`` parameters) will not work.

push-functionality will use the converter type specified by
``push_converter_for<T>``, and pull (``from_stack``) functionality will use
``pull_converter_for<T>``. They are defined as follows::

   template <typename T>
   using push_converter_for = converter<T-withouth-cvr-qualifications>;

   template <typename T>
   using pull_converter_for = converter<T-without-cv-qualifications>;

There is also the ``to_type_of<Conv>`` alias that evaluates to ``Con::to_type``
but removes cvr-qualifications from ``Conv`` itself.

.. _sec-converters-advanced-fns:

Advanced converter functions
----------------------------

The following functions are variants of the functions with the same name but
without the ``_with`` suffix, that take the converter to use as an additional
first argument:

- ``n_conversion_steps_with``
- ``is_convertible_with`` (see :ref:`f-is_convertible`)
- ``unchecked_from_stack_with`` (see :ref:`f-unchecked_from_stack`)
- ``from_stack_with`` (see :ref:`f-from_stack`).

There is no ``push_with`` since it is easy to directly use a converter's
``push`` member function. This is not the case for the functions above, since
the underlying converter functions may or may not support a ``next_idx``
argument. ``n_conversion_steps_with``, ``unchecked_from_stack_with`` and
``from_stack_with`` also have ``int* next_idx`` as an additional last parameter,
but with a default argument of ``nullptr``.


.. _f-n_conversion_steps:

``n_conversion_steps()``
^^^^^^^^^^^^^^^^^^^^^^^^

::

   template <typename /* explicit */ T>
   unsigned n_conversion_steps(lua_State* L, int idx);

Returns approximately how many conversion steps the default converter for ``T``
would need to convert the value at index ``idx`` on the stack of ``L`` or
``no_conversion`` if it is not convertible at all. ``0`` is the best conversion,
``no_conversion - 1`` means that the conversion is only usable as a fallback
(such as the conversion from userdata to ``bool``).

:ref:`f-is_convertible` is implemented in terms of this function,
``no_conversion`` leading to a return value of ``false`` and everything other to
``true``.
