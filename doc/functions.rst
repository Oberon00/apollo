Functions
=========

apollo supports pointers to functions (both free ones and member functions) as
well as ``boost::function`` and ``std::function``. In addition, apollo offers
facilities for raw functions: Pushing ``lua_CFunction``\ s as such and
converting other functions to them for zero-overhead compared to a handwritten
wrapper. Lastly, some wrapper functions are provided: For constructors,
operators and public data member getters and setters.

Function converter
------------------

Header::

   #include <apollo/function.hpp>

apollo's function converter supports pushing

   - pointers to free functions
   - pointers to member functions
   - ``boost::function``
   - ``std::function``

Arguments will be retrieved using :ref:`f-from_stack` and the return value
is pushed using :ref:`f-push`. Exceptions are converted to Lua errors using
:ref:`f-exceptions_to_lua_errors`. The ``this`` argument for member functions
of a class ``C`` is retrieved as ``C&`` for non-const member functions and as
``C const&`` for const member functions (see :ref:`sec-classes`).

Additionally, function pointers pushed with apollo can be retrieved back using
:ref:`f-from_stack` as the exact same type they were pushed. A ``boost::`` or
``std::function`` can be obtained from any Lua function or other value that has
a ``__call`` metamethod.  If the value was pushed as the exact same function
object type or as a pointer to a compatible free function, invoking the function
object will call the the C++ function directly. Otherwise, the function object
will contain a reference to the Lua function object and it will be called
through :ref:`f-pcall`, pushing any arguments onto the Lua stack using
:ref:`f-push`.

Implementation notes
^^^^^^^^^^^^^^^^^^^^

There is a bit of overhead associated with apollo's functions: First, in order
to be able to get the functions back from Lua, type information has to be saved,
needing two light userdata upvalues. Then, of course, the function pointer
itself has to be saved which is a light userdata for free functions and a full
userdata for most member function pointers and all function objects. When
calling a function pushed this way, (only) the function upvalue needs to be
accessed as an additional overhead. However, all of this can be avoided by using
``APOLLO_TO_RAW_FUNCTION``, as described in the next two sections.


Raw functions
-------------

Header::

   #include <apollo/raw_function.hpp>

Sometimes, you want to just directly push a raw ``lua_CFunction`` to Lua using
:ref:`f-push`. However, if you just do that, apollo will try to use
``from_stack`` for the ``lua_State*`` and ``push`` a single ``int`` back. To
inform apollo that it should just use ``lua_pushcfunction()``, you need to wrap
your function into an ``apollo::raw_function`` (defined in the
``<apollo/builtin_types.hpp>`` header)::

   struct raw_function {
       /* implicit */ constexpr raw_function(lua_CFunction f_) noexcept;
       /* implicit */ constexpr operator lua_CFunction() const noexcept;

       template <lua_CFunction /* explicit */ FVal>
       static constexpr raw_function caught() noexcept;

       lua_CFunction f;
   };

The constructor will just initialize the public ``f`` member with its argument,
the implicit conversion operator returns it. The static member function
``caught`` takes a ``lua_CFunction`` as template argument and returns a
``raw_function`` that contains the passed function wrapped in
:ref:`f-exceptions_to_lua_errors`.


Converting arbitrary C++ functions to raw functions
---------------------------------------------------

Header::

   #include <apollo/to_raw_function.hpp>

.. _f-to_raw_function:

``to_raw_function()``
^^^^^^^^^^^^^^^^^^^^^

::

   template <typename /* explicit */ F, F /* explicit */ FVal>
   constexpr raw_function to_raw_function() noexcept;

Takes a function pointer type and value as explicit template arguments and
returns a raw function that wraps it, converting arguments using
:ref:`f-from_stack` and the return value using :ref:`f-push`.

Note that a function pushed this way cannot be really retrieved back as it does
not contain any type information (which is the advantage at the same time: zero
upvalues). It can however be retrieved as ``boost::`` or ``std::function``, but
it will only be called via ``pcall`` then.

Usage example:

.. literalinclude:: examples/raw_function.cpp

.. _f-APOLLO_TO_RAW_FUNCTION:

``APOLLO_TO_RAW_FUNCTION()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   #define APOLLO_TO_RAW_FUNCTION(f) to_raw_function<decltype(f), f>()
   // Exposition only!

Since it is cumbersome to always write the ``decltype`` or manually spell the
function pointer, this macro does just that for you. It also works for template
function names containing unparenthesized commas and works around a bug in MSVC
where ``decltype`` breaks for addresses of template functions.


.. _f-APOLLO_PUSH_FUNCTION_STATIC:

``APOLLO_PUSH_FUNCTION_STATIC()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   #define APOLLO_PUSH_FUNCTION_STATIC(L, f) \
       lua_pushcfunction(L, APOLLO_TO_RAW_FUNCTION(f))
   // Exposition only!

Converts ``f`` to a raw function using :ref:`f-APOLLO_TO_RAW_FUNCTION` and
pushes it onto ``L``.


Wrappers for constructors
-------------------------

Header::

   #include <apollo/ctor_wrapper.hpp>

.. _f-ctor_wrapper:

``ctor_wrapper()``
^^^^^^^^^^^^^^^^^^

::

   template <typename T, typename... Args>
   T ctor_wrapper(Args... args);

Returns ``T`` constructed from ``args``. This is useful when you need to expose
constructors to Lua.


::

   template <typename T, typename... Args>
   see-below new_wrapper(Args... args);

If ``T`` is a (smart) pointer type, returns ``T`` constructed from ``new obj_t``
with the given ``args``, where ``obj_t`` is the pointee type of ``T``. Otherwise
returns ``new T`` constructed with ``args`` as ``T*``.


Wrappers for C++ operators
--------------------------

Header::

   #include <apollo/operator.hpp>

Apollo provides wrapper functions for all C++ operators, except these that do
in-place modifications to their operands (e.g. ``+=`` and ``--``) and the
pointer operators (unary ``*`` and ``->``). They take the following form::

    namespace apollo { namespace op {

    // For binary operators:
    template <typename Lhs, typename Rhs>
    auto name(Lhs lhs, Rhs rhs) -> decltype(lhs OP rhs)
    {
        return lhs OP rhs; // Exposition only!
    }

    // For unary operators:
    template <typename Operand>
    auto name(Operand operand) -> decltype(OP operand)
    {
        return OP operand; // Exposition only!
    }

    } } // namespace apollo::op

The names are the same as the ones of the corresponding Lua metamethods (without
leading underscores of course) or follow their naming convention when none
exists. In the following table, a “*” in the notes column indicates that there
exists no corresponding Lua metamethod, a “3” indicates that it exists only
since Lua 5.3 and a “u” indicates an unary (prefix) operator:

======  ====  =====
``OP``  name  Notes
======  ====  =====
``+``   add
``-``   sub
``*``   mul
``/``   div
``%``   mod
``-``   unm   u
``==``  eq
``!=``  ne    \*
``>``   gt    \*
``<``   lt
``>=``  ge    \*
``<=``  le
``&&``  land  \*
``||``  lor   \*
``!``   lnot  u*
``&``   band  3
``|``   bor   3
``^``   bxor  3
``~``   bnot  u3
``<<``  shl   3
``>>``  shr   3
======  ====  =====



Wrappers to get and set public data members
-------------------------------------------

Header::

   #include <apollo/property.hpp>

To expose public data members to Lua, apollo provides the following macros:


.. _f-APOLLO_MEMBER_SETTER:

``APOLLO_MEMBER_SETTER()``
^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   #define APOLLO_MEMBER_SETTER(m) /* implementation detail */

``m`` must be a member name of the form ``some_class::some_member``.

This macro expands to a function (not a function pointer!) of the following
form::

   void implementation-defined(some_class& obj, decltype(m) const& v);

This function sets ``some_member`` in ``obj`` to ``v``.


.. _f-APOLLO_MEMBER_GETTER:

``APOLLO_MEMBER_GETTER()``
^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   #define APOLLO_MEMBER_GETTER(m) /* implementation detail */

``m`` must be a member name of the form ``some_class::some_member``.

This macro expands to a function (not a function pointer!) of the following
form::

   decltype(m) const& implementation-defined(some_class const& obj);

This function returns a ``const&`` to ``some_member`` in ``obj``. Note that
pushing a ``const&`` using :ref:`f-push` will create a copy for Lua.
