.. _sec-classes:

Classes (user-defined types)
============================

Header::

   #include <apollo/class.hpp>

apollo allows you to use class types with :ref:`f-push` and :ref:`f-from_stack`,
provided you tell the library about the type first using
:ref:`f-register_class`. However, to make the type usable in Lua, you also need
to set the appropriate metatable fields (usually at least ``__index``).

.. seealso::

  :ref:`sec-basic-cls`
    A short tutorial that demonstrates the most important features.

  :ref:`sec-ctor`, :ref:`sec-op`, :ref:`sec-property`
    Useful wrapper functions.


Pointer and reference semantics, smart pointer support
------------------------------------------------------

If you push a object by value (or by reference; that is indistinguishable) it
will be copied (or moved for rvalue references) to Lua and the copy/moved-to
object will live in memory allocated by ``lua_newuserdata``. Such objects are
never ``const``.

If you push by pointer, only the pointer is copied (or moved for rvalue
references) to Lua. apollo does not make any assumptions about ownership: The
only thing that happens when the containing Lua userdata is collected is that
the pointer's destructor is called. That means, if you want to pass ownership to
Lua, use an ``std::unique_ptr``, if you want to share ownership with Lua use an
``std::shared_ptr``, if you just want to pass a reference to Lua use a raw
pointer and make sure that the referenced object stays alive as long as any
references to it exist in Lua (which usually means, if you want to be sure about
it, as long as the ``lua_State`` is not closed).

Pointers to ``const`` will make the Lua representation of the object act as
``const`` (i.e. it is only convertible to ``const&``, ``const*`` or the value
type, which also means that only const member functions can be called on it,
since apollo will use ``from_stack<C&>`` for non-const member functions).


.. _sec-cls-from_stack:

Using ``from_stack`` with class types
--------------------------------------

For a class type ``C``, ``from_stack`` can be used with a template argument of
``C*``, ``C&`` to obtain a mutable pointer/reference to the object on the Lua
stack, as expected. ``C const*``, also as expected, obtains a pointer to const.

However, just ``from_stack<C>`` or ``C const&`` (both equivalent) return not
exactly what you asked for but instead they return an implementation defined
reference wrapper object. This is necessary for ``const&`` because the object
retrieved might not actually live in Lua but be an :ref:`implicitly constructed
object <sec-cls-implicit>`. In that case, the lifetime of the object is bound to
the reference wrapper's. So how do you use an object wrapped in such a reference
wrapper? First, make sure the wrapper stays alive for the time you use the
object if you use it as ``const&`` (C++ ensures that objects stay alive inside
the *full expression* in which they were constructed, which is already
enough for cases where you just pass the result of ``from_stack`` to another
function that does not store a reference to its argument). Then you can use the
wrapped object by:

- Employing the implicit conversion of the reference wrapper to ``C`` (copies
  the value).
- Using the ``get()`` member function that returns a ``C const&``.
- Passing the wrapper to the ``unwrap_bound_ref`` function, which returns the
  same as ``get()`` but is useful in generic code, because if the argument is not
  a reference wrapper, it is returned unchanged.

Note that an object that is ``const`` in Lua is only convertible to  ``C``, ``C
const&`` and ``C const*``.

A nil value is converted to a ``nullptr`` / default-constructed smart pointer.

Smart pointer retrieval limitations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

While normal pointers, references or value types can even be converted from
derived types to registered bases thereof, retrieving an object as a smart
pointer from Lua only works if it was pushed as the exact same smart pointer
type. E.g. a ``std::shared_ptr<Derived>`` in Lua can be converted to only:

- ``Derived``, ``Derived*``, ``Derived&`` and const variants.
- ``Base`` (yes, that's slicing!), ``Base*``, ``Base&`` and const variants.
- ``std::shared_ptr<Derived>``
- ``std::shared_ptr<Derived>&``
- ``std::shared_ptr<Derived> const&``

(Would it not be a ``std::shared_ptr`` but another type, e.g. a value or a
plain pointer or a ``unique_ptr``, the last two were also not possible.)  It
can *not* be converted to e.g.:

- ``std::shared_ptr<Derived const>``
- ``std::shared_ptr<Base>``

Note that for a move-only smart pointer such as ``std::unique_ptr`` only
conversion to a (const) reference is possible.

.. todo:: Lift the const restriction.

Function reference
------------------

.. _f-register_class:

``register_class()``
^^^^^^^^^^^^^^^^^^^^

::

   template <typename /* explicit */ T, typename... /* explicit */ Bases>
   void register_class(lua_State* L);

Registers the class ``T`` for usage with apollo, allowing conversions to
``Bases``, bases thereof (if any) and so on.

This function needs to be called before any object of the class ``T`` can be
pushed or a retrieval attempted. apollo will save the type information and
allocate a metatable for objects of this type (see
:ref:`f-push_class_metatable`).

Base classes must be registered before derived ones. If you don't need
conversions to a base, you can leave it out. You may, however, not specify types
as bases that are none. Virtual bases are not supported.

.. _f-push_class_metatable:

``push_class_metatable()``
^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   template <typename /* explicit */ T>
   void push_class_metatable(lua_State* L);

Pushes the metatable onto the stack of ``L`` that newly pushed objects of type
``T`` will use when pushed.

``T`` needs to be registered in ``L`` (see :ref:`f-register_class`).

The metatable initially only contains a ``__gc`` metamethod that calls the
objects destructor and frees internal apollo type information. You may set
your own ``__gc`` metamethod but this method must call the original one.

You will usually want to set at least the ``__index`` metafield.

.. _f-emplace_object:

``emplace_object()``
^^^^^^^^^^^^^^^^^^^^

::

   template <typename /* explicit */ T, typename... Args>
   void emplace_object(lua_State* L, Args&&... args);

Like ``push(L, T(std::forward<Args>(args)...))`` but constructs the object
directly in Lua, withouth copying or even moving it. Very handy for types that
are not moveable.

Note that this always uses apollo's default object converter, even if you wrote
your own specializations for ``converter<T>``.


.. _f-get_raw_emplace_ctor_wrapper:

``get_raw_emplace_ctor_wrapper()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Header::

  #include <apollo/emplace_ctor.hpp>
  
::

  template <typename /* explicit */ T, typename... /* explicit */ Args>
  constexpr raw_function get_raw_emplace_ctor_wrapper() noexcept;

Returns a raw function that constructs ``T`` in Lua using
:ref:`f-emplace_object` from argument types ``Args``. When exposing constructors
to Lua, this should be preferred for efficiency reasons. It is necessary when
``T`` is not moveable.

.. _sec-cls-implicit:

Implicit constructors/conversion support
----------------------------------------

Header::

  #include <apollo/implicit_ctor.hpp>


.. _f-add_implicit_ctor:

``add_implicit_ctor()``
^^^^^^^^^^^^^^^^^^^^^^^

::

   template <typename From, typename To>
   void add_implicit_ctor(lua_State* L, To(*ctor)(From));

By adding an implicit constructor / conversion function from ``From`` to ``To``,
:ref:`f-from_stack` will be able to convert types that have the type ``From`` in
Lua to a value type of just ``To`` or to a const reference ``To const&`` (that's
why a :ref:`reference wrapper <sec-cls-from_stack>` is returned for these two
kinds of types).

``To`` can also be a raw pointer to a class type. This is actually recommended,
since it is more efficient with the current implementation. If a value type is
returned, it needs to be moveable.

.. seealso:: :ref:`sec-ctor`
