Lower level utilities
=====================

Safely calling Lua functions from C++
-------------------------------------

Header::

   #include <apollo/lapi.hpp>

.. _f-pcall:

``pcall()``
^^^^^^^^^^^

::

   void pcall(lua_State* L, int nargs, int nresults, int msgh); // 1
   void pcall(lua_State* L, int nargs, int nresults); // 2

The first overload is like ``lua_pcall`` but instead of returning an error code
on failure, it throws an ``lua_api_error``. This class inherits from
``boost::exception`` and, when thrown by ``pcall`` always contains the
following error information objects (defined in header ``<apollo/error.hpp>``:

- The ``lua_State*`` (``L``) at ``errinfo::lua_state``.
- The string on top of the Lua stack (the error message) as a ``std::string`` at
  ``errinfo::lua_msg``. If there was no string on top of the stack, it will
  contain the string ``"(no error message)"``.
- The error code (e.g. ``LUA_ERRUN`` or ``LUA_ERRGCMM``) that ``lua_pcall``
  returned at ``errinfo::lua_error_code``.
- The message ``"lua_pcall() failed"`` as a ``std::string`` at ``errinfo::msg``.

The second overload is like calling the first without an message handler (``msgh
= 0``) if no message handler was set (see below). Otherwise it is like calling
the first overload with the set message handler.


.. _f-set_error_msg_handler:

``set_error_msg_handler()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   void set_error_msg_handler(lua_State* L);

Pops the value at the top of the stack and saves it as error message handler for
``pcall``.


.. _f-push_error_msg_handler:

``push_error_msg_handler()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   bool push_error_msg_handler(lua_State* L);

If you previously set an error message handler using
:ref:`f-set_error_msg_handler`, pushes that on top of the stack and returns
``true``. Otherwise pushes nothing and returns ``false``.


Safely calling C++ functions from Lua
-------------------------------------

.. _f-exceptions_to_lua_errors:

``exceptions_to_lua_errors()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   template <typename F, typename... Args>
   auto exceptions_to_lua_errors(lua_State* L, F&& f, Args&&... args) noexcept
       -> decltype(f(std::forward<Args>(args)...));

Calls ``f`` with ``args`` and returns its return value. If an exception is
thrown by ``f``, calls ``lua_error()`` with a string describing the exception
(usually of the form ``exception: <what()>``, but more descriptive for e.g.
``to_cpp_conversion_error``\ s).

.. seealso:: The ``caught<F>()`` method of :ref:`raw_function <sec-fn-raw>` is
   more convenient to use if you want to wrap a function otherwise (i.e. except
   for error handling) conforming to Lua calling conventions for usage from Lua.

.. warning:: Make sure that the move/copy constructors of the arguments and the
   return value do not throw! Since these operations happen before/after
   ``exceptions_to_lua_errors`` is executed, such exceptions cannot be
   translated to Lua errors (they will not even trigger the ``noexcept`` so
   ``std::terminate()`` wont be called either).


.. _f-exceptions_to_lua_errors_L:

``exceptions_to_lua_errors_L()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   template <typename F, typename... Args>
   auto exceptions_to_lua_errors_L(lua_State* L, F&& f, Args&&... args) noexcept
       -> decltype(f(L, std::forward<Args>(args)...));

Just like :ref:`f-exceptions_to_lua_errors` but calls ``f`` with ``L`` as an
additional first argument.



Garbage collection and low level userdata utilities
---------------------------------------------------

Header::

   #include <apollo/gc.hpp>


.. _f-push_bare_udata:

``push_bare_udata()``
^^^^^^^^^^^^^^^^^^^^^

::

   template <typename T>
   T-without-cvr* push_bare_udata(lua_State* L, T&& o);

Pushes a new userdata with ``sizeof(T)`` onto ``L`` and constructs a ``T`` into
it's copy/move constructor with ``o``. Returns a pointer to that userdata.

Equivalent to ``emplace_bare_udata<T>(L, o)``.


.. _f-emplace_bare_udata:

``emplace_bare_udata()``
^^^^^^^^^^^^^^^^^^^^^^^^

::

   template <typename /* explicit */ T, typename... Args>
   T-without-cvr* emplace_bare_udata(lua_State* L, Args&&... ctor_args);

Pushes a new userdata with ``sizeof(T)`` onto ``L`` and constructs a ``T`` into
it, calling its constructor with ``args``. If the constructor throws an
exception, the userdata will be popped from the stack before rethrowing.


.. _f-gc_object:

``gc_object()``
^^^^^^^^^^^^^^^

::

   template <typename /* explicit */ T>
   int gc_object(lua_State* L) noexcept;

Intended for use as ``lua_CFunction``. Calls ``~T`` on the first Lua argument
(i.e. the value at index 1 on ``L``'s stack) without any checking. Always
returns ``0`` (= no Lua return values).


.. _f-push_gc_object:

``push_gc_object()``
^^^^^^^^^^^^^^^^^^^^

::

   template <typename T>
   T-without-cvr* push_gc_object(lua_State* L, T&& o);

Just like :ref:`f-push_bare_udata`, but if the cvr-unqualified ``T`` is not
trivially destructible, creates a new table, sets its ``__gc`` field to
:ref:`f-gc_object` (with appropriate ``T``) and sets the new table as metatable
of the newly created userdata.

``closing_lstate``
------------------

Header::

   #include <apollo/closing_lstate.hpp>

Synopsis::

   class closing_lstate {
   public:
       explicit closing_lstate(lua_State* L); // 1
       closing_lstate(); // 2
       closing_lstate(closing_lstate&& other);
       closing_lstate& operator= (closing_lstate&& other);
       ~closing_lstate();

       operator lua_State* ();
       lua_State* get();
   };

Stores a ``lua_State*`` (passed either via constructor 1 or created via
``luaL_newstate()`` by constructor 2) and closes it in its destructor.
``closing_lstate`` objects are moveable but not copyable. ``get()`` and the
implcit conversion operator both return the stored ``lua_State*``.

References to the Lua registry
------------------------------

Header::

   #include <apollo/reference.hpp>

Synopsis::

   enum class ref_mode { move, copy };

   class registry_reference {
   public:
       registry_reference(); // 1
       explicit registry_reference( // 2
           lua_State* L_, int idx = -1, ref_mode mode = ref_mode::move);
       ~registry_reference();

       registry_reference(registry_reference const& rhs);
       registry_reference& operator=(registry_reference const& rhs);

       registry_reference(registry_reference&& rhs);
       registry_reference& operator= (registry_reference&& rhs);

       void reset(int idx, ref_mode mode = ref_mode::move); // 1
       void reset( // 2
           lua_State* L_ = nullptr, int idx = 0,
           ref_mode mode = ref_mode::move);

       void push() const;

       bool empty() const;
       lua_State* L() const;
       int get() const;
   };

Manages a reference to the Lua registry using ``luaL_ref()`` / ``luaL_unref``.

The first constructor is equivalent to the second with ``L_ = nullptr`` and
``idx = 0``.

The second constructor overload and the second ``reset()`` overload make the
reference point to the value at ``idx`` on ``L``. If ``ref_mode::move`` was
specified, the value will be removed from the stack, if ``ref_mode::copy`` was
specified, it will be left there. The first ``reset()`` overload behaves like
the second called with the ``lua_State*`` that the reference currently refers to
(it thus may only be called if the reference refers to a ``lua_State*``). Both
``reset()`` overloads will ``luaL_unref`` the current reference, if any.

If ``0`` is passed as ``idx`` to ``reset()`` or the second constructor overload,
the reference is initialized with ``LUA_NOREF``. ``0`` is the only acceptable
``idx`` if ``L`` is ``nullptr``.

``push()`` pushes the currently referenced value on top of the currently
referenced states stack. Precondition: ``!empty()``.

A ``registry_reference`` is considered ``empty()`` if it references
``LUA_NOREF``. ``L`` returns the currently referenced ``lua_State*`` (or
``nullptr`` if there is none) and ``get()`` returns the registry index of the
value referenced there (or ``LUA_NOREF`` if there is none).

This class is usable with :ref:`f-push` and :ref:`f-to`: Pushing just
calls the ``push`` method or pushes nil if the reference is empty. If an attempt
is made to push a reference that references another state as that on which it
should be pushed, the behaviour is undefined.  ``to(L, idx)`` will
return a ``registry_reference`` constructed with these arguments and
``ref_mode::copy``. Thus, :ref:`f-is_convertible` is always true for
``stack_reference``.


References to the Lua stack
---------------------------

Header::

   #include <apollo/reference.hpp>


Synopsis::

   class stack_reference {
   public:
       explicit stack_reference(lua_State* L = nullptr, int idx = 0);
       void reset(lua_State* L = nullptr, int idx = 0);
       bool empty() const;
       bool valid(lua_State* L) const;
       int get() const;
   };

References the stack value at ``idx`` (as passed to ``reset`` or the
constructor). If ``idx`` is relative, it is converted to an absolute index using
``lua_absindex``.

A ``stack_reference`` is considered ``empty()`` if it has an ``idx`` of ``0``.

A ``stack_reference`` is considered ``valid(L)`` if it not empty and if ``idx <=
lua_gettop(L)``.

``get()`` returns the stored index.

The usefulness of ``stack_reference`` comes from the fact that it can be used
with :ref:`f-push` and :ref:`f-to`. Thus it can be used e.g. with
:ref:`f-rawset_table` or :ref:`f-new_table`.

``push()`` will (re)push the referenced value on top of the stack (using
``lua_pushvalue()``) or push nil if the reference is empty. ``to(L,
idx)`` will just construct a ``stack_reference`` with these arguments. Thus,
:ref:`f-is_convertible` is always true for ``stack_reference``.


``stack_balance``
-----------------

Header::

   #include <apollo/stack_balance.hpp>

Synopsis::

   class stack_balance {
   public:
       enum action { pop = 1, push_nil = 2, adjust = pop | push_nil, debug = 4 };
       explicit stack_balance(
           lua_State* L,
           int diff = 0,
           int act = pop|debug);
       ~stack_balance();
   };

Saves the current value of ``lua_gettop()`` in its constructor and in its
destructor, if the new stack top is not equal to the old stack top + ``diff``
acts according to the ``act`` parameter givent to the constructor:

- If the stack contains too many values and ``act`` has the ``pop``-bit set,
  pops surplus values.
- If the stack contains too few values and ``push_nil`` was specified, pushes
  ``nil``, until the desired stack size is reached.
- If, after the above, the stack size does not match the desired and ``debug``
  was specified, a ``BOOST_ASSERT`` will fail with an error message.

Type information
----------------

Header::

   #include <apollo/typeid.hpp>


.. _f-lbuiltin_typeid:

``lbuiltin_typeid()``
^^^^^^^^^^^^^^^^^^^^^

::

   boost::typeindex::type_info const& lbuiltin_typeid(int id);

Returns what :ref:`f-ltypeid` returns for the builtin Lua type ``id`` (e.g.
``LUA_TSTRING``) when no more specific type information is available.


.. _f-ltypeid:

``ltypeid()``
^^^^^^^^^^^^^

::

   boost::typeindex::type_info const& ltypeid(lua_State* L, int idx);

For userdata values pushed with apollo's object converter, returns the type of
the object. Otherwise, returns ``lbuiltin_typeid(lua_type(L, idx))``.
