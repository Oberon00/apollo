Lower level utilities
=====================

- lapi.hpp


.. _f-pcall:

``pcall()``
^^^^^^^^^^^

::

   void pcall(lua_State* L, int nargs, int nresults, int msgh); // 1
   void pcall(lua_State* L, int nargs, int nresults); // 2



- gc.hpp
- closing_lstate
- reference.hpp


.. _f-exceptions_to_lua_errors:

``exceptions_to_lua_errors()``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

   template <typename F, typename... Args>
   auto exceptions_to_lua_errors(lua_State* L, F&& f, Args&&... args) noexcept
       -> decltype(f(std::forward<Args>(args)...));

- see functions for wrapper functions
- stack_balance
- typeid
