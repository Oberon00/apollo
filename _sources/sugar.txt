Helpers for table creation/modification
=======================================

These helpers offer a convenient syntax for :ref:`f-push` and `lua_rawset`.

.. seealso::

   :ref:`sec-basic-sugar`
      A short tutorial that demonstrates the most important features.

Header::

   #include <apollo/create_table.hpp>

Free functions
--------------

.. _f-rawset_table:

``rawset_table()``
^^^^^^^^^^^^^^^^^^

::

   table-setter rawset_table(lua_State* L, int table_idx);

Returns a table setter that will modify the table at index ``table_idx`` on the
stack of ``L``.


.. _f-new_table:

``new_table()``
^^^^^^^^^^^^^^^

::

   table-setter new_table(lua_State* L);

Pushes a new table onto ``L``'s stack and returns a table setter that will
modify it.


Table setter objects
--------------------

Table setter objects modify their reference tables using ``lua_rawset`` with the
methods below. All methods return a rvalue reference to the table setter they
were called on so that they are chainable.

You should not store a table setter! Just use it in the expression it was
created in.

.. warning:: Due to C++'s undefined order of argument evaluation, it is
   important that any methods you call in this expression leave the Lua stack
   balanced.

.. _f-ts-operator-call:

``operator()``
^^^^^^^^^^^^^^

::

    template <typename K, typename V>
    table-setter&& operator() (K&& key, V&& value);

Sets ``value`` as the value of the current table at ``key``.


.. _f-ts-subtable:

``subtable()``
^^^^^^^^^^^^^^

::

   template <typename K>
   table-setter&& subtable(K&& key);

Creates a new table, sets it as the value of the current table at ``key`` and
makes the table setter modify that new table until :ref:`f-ts-end_subtable` is
called. Note that every call to ``subtable()`` must be matched by a corresponding
call to ``end_subtable()``, even if you do not intend to modify the previous
table.

Subtables may be nested.


.. _f-ts-end_subtable:

``end_subtable()``
^^^^^^^^^^^^^^^^^^

::

   table-setter&& end_subtable();

Ends a subtable began by :ref:`f-ts-subtable`. See that method for more
information.


.. _f-ts-thistable_as:

``thistable_as()``
^^^^^^^^^^^^^^^^^^

::

   template <typename K>
   table-setter&& thistable_as(K&& key);

Sets the current table as the value of itself at ``key``.


.. _f-ts-thistable_index:

``thistable_index()``
^^^^^^^^^^^^^^^^^^^^^

::

   table-setter&& thistable_index();

Equivalent to calling :ref:`f-ts-thistable_as` with ``"__index"`` as key.


.. _f-ts-metatable:

``metatable()``
^^^^^^^^^^^^^^^

::

   table-setter&& metatable();

If the current table does not have a metatable, creates a new table and sets it
as metatable. Then makes the table setter modify the current table's metatable
until :ref:`f-ts-end_metatable` is called. Note that every call to
``metatable()`` must be matched by a corresponding call to ``end_metatable()``,
even if you do not intend to modify the previous table.

``metatable()`` calls may be nested, they will modify the metatable's metatable
and so on.


.. _f-ts-end_metatable:

``end_metatable()``
^^^^^^^^^^^^^^^^^^^

::

   table-setter&& end_metatable();

Ends a metatable began by :ref:`f-ts-metatable`. See that method for more
information.
