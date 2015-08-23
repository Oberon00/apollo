# Apollo C++ styleguide

This style guide defines the style used for apollo. It is most similar to the
[Boost][] / standard library style.

[Boost]: http://boost.org

If not explicitly stated otherwise, everything said about variables also applies
to function parameters (which are variables too).

## Definitions

<dl>
  <dt>private context</dt>
  <dd>
    Inside a <code>detail</code> subnamespace, (inside) an entity in a cpp-file
    withouth external linkage or in an anonymous namespace, in the
    <code>private</code> section of a class.
  </dd>
</dl>

## General

The following example shows many aspects of apollo's formatting style. The
details are described in the remainder of this guide.

```c++
namespace somenamespace { namespace subnamespace {

class some_type {
public:
    void do_something()
    {
        if (some_condition) {
            do_a2(); // We can't use a1 here because it frobinates the zquift.
        } else if (some_check()) {
            // do_x() can throw if the moon is in the third house and the caller
            // acted naughty last thursday. We need to handle this.
            try {
                do_x(&create_bar, foo + bar);
            } catch (mischief_error const& e) {
              call_cops(e.reason());
              throw;
            }
            do_a();
        }
    }

    // ...

private:
    int m_some_member;
};

} } // namespace somenamespace::subnamespace

```

- Only ASCII characters may be used in source files and for file names.
  Documentation files may use UTF-8 instead.
- Filenames must be valid C++ identifiers (e.g no spaces, no special
  characters), except that non-source files may additionally use hyphens to
  separate words.
- Unix linebreaks (LF) must be used.
- You must not leave trailing whitespace (including whitespace on otherwise
  empty lines that continues the indentation from the line above). This
  overrules all other rules about the placement of spaces.
- Lines must not be longer than 80 characters. This is so that at least two
  files can be viewed side-by-side. Additionally, reading long lines is tiresome
  for our human eyes (count how many characters there are in books!).
- Do not put multiple statements on the same line.
- Use assertions using [`BOOST_ASSERT` or `BOOST_ASSERT_MSG`][boost-assert]
  freely. Also make yourself familiar with the great [`BOOST_VERIFY` and
  `BOOST_VERIFY_MSG`][boost-assert] macros and use them freely too.

[boost-assert]: http://www.boost.org/doc/libs/release/libs/assert/assert.html

### Spaces

- Put a space between keywords and the following token, e.g. `if (foo)` instead
  of ~~`if(foo)`~~.
- Do not put spaces on either side of parenthesis, square or angle brackets
  otherwise.
- Definitions of overloaded operators are exempt from these rules: use the
  format `return_type operator+ (lhs_type lhs, rhs_type rhs)` for them instead.
- Put a space after commas.
- Put a space on each side of binary operators except the `.`, `->`, `::` and
  `,` operators (for the latter, see the above rule).
- Put a space on each side of the trailing return type introduction token `->`.

### Indentation

- Indentation is four spaces, no tabs allowed.
- Do not align the beginning of lines (e.g. for function parameters or arguments).

### Capitalization styles

<dl>
  <dt><code>lowercase</code></dt>
  <dd>
    All lowercase withouth underscores (e.g. <code>touserdata</code>). Used for
    namespace names.
  </dd>

  <dt><code>snake_case</code></dt>
  <dd>
    All lowercase with underscores seperating multiword identifiers. Used for
    basically everything.
  </dd>

  <dt><code>PascalCase</code></dt>
  <dd>
    Camel case with initial uppercase letter. Used for template parameter
    names.
  </dd>

  <dt><code>ALL_CAPS</code></dt>
  <dd>
    All caps with underscores. Used for <code>#define</code>d names
    (“macros”).
  </dd>
</dl>

### Braces

- The “Stroustrup style” is used, i.e. braces are attached to everything
  (including namespaces, classes and lambdas) except functions. “Attached” means
  on the same line as the corresponding keyword. Also, `else`, `else if` and
  `catch` are placed on the same line as the closing brace of the previous
  block.

- Braces should be elided for “blocks” spanning only a single line (≠ single
  statement) that are not in the same, e.g. use:

  ```c++
  if (a)
      f();
  else
      d();
  ```

  + Do not, however, elide braces for multi-line statements, even if it is
    syntactically legal, e.g. use:

    ```c++
    if (a) {
        while (b)
            f();
    }

    // and:

    if (a) {
        a_single_statement(
            that_is + not_even(composed, but | still | too | longish, that, it),
            does_not, fit<on_a>::single_line());
    }
    ```

    But do not use:

    ```c++

    if (a)
        while(b)
            f();

    // nor:

    if (a)
        a_single_statement(
            that_is + not_even, composed, but | still | too | long, that, it,
            does_not, fit<on_a>::single_line());
    ```

  + Also do not elide braces for `else` or `else if` if an associated
    ``if``/``else``/``else if`` uses braces:

    ```c++
    if (a) {
        do_a();
        do_b();
    } else {
        f(); // Must use braces around this.
    }
    ```

## Functions

- Function names must use `snake_case`.
- Functions should be named after verbs or, if they are side-effect free, should
  be named after the value they return (noun). In the latter case, the rules for
  variable names apply.
- For functions converting something to something other, you should usually use
  names in the `to_b`, `a::to_b` or `a_to_b` style. Never abbreviate `to` with
  `2`. You should usually avoid the `b_from_a` style. You may use `b::from_a` if
  the conversion function needs to access private members of either ``a`` or
  ``b`` and ``a`` does not need to know ``b`` otherwise than for a ``a::to_b``
  function.
- Do not use a `get_` prefix for getters, but use a `set_` prefix for setters,
  e.g. `someobj.value()` and `someobj.set_value()` as accessors for the property
  `value` (maybe implemented using a private member variable `m_value`).
- Prefer the name (part) `create` to `make` or `new`.
- Use the `_impl` suffix for a function in a private context that is only
  necessary for technical reasons (e.g. supplementing a tuple with a
  corresponding index sequence) to implement another function of the same name
  but without the suffix.
- Prefer return values to output parameters.
  + If you need multiple return values, consider if it makes sense to introduce a
    type to hold them.
  + Avoid `std::pair` and `std::tuple` as a replacement for output parameters of
    distinct meaning, unless it follows an established convention like using
    `std::pair::first` as key and `std::pair::second` as value.
- Annotate functions that never return (e.g. because they always call
  `lua_error`, `abort`, `terminate`, etc. or always throw exceptions) with
  [`BOOST_NORETURN`][boost-noreturn].

## Types

- Type names must use `snake_case`.
- Types should be named after nouns.

#### Type aliases
- Do not use `typedef`, use `using` aliases instead.
- Do not use C++03 style templated aliases (`struct` templates with a member
  alias `type` as the only member). Use templated `using` aliases instead.
- The above two rules may be ignored if it is necessary for compiler
  compatibility (MSVC).
- Consider using a `_t` suffix for `using`-aliases if they describe some traits
  of a class that are derived from a template parameter (but not of the template
  parameter!).
- Use the `_t` alias for a `using`-alias template if an equivalent C++03 style
  nested typedef withouth the `_t` suffix exists.
- Do not use the `_t` suffix for anything else.

### `class` and `struct` definitions
- Do not indent accessibility keywords (`public:`, `private:`, `protected:`).
- Put `public` members first, then `protected` and at the end `private`.
- Do not use default accessibility for `class` and avoid non-default visibility
  for `struct`, except for implementation details for metafunctions.

### Declarators
- All declarators (`&`, `&&`, `*`) are considered part of the type, not the
  variable name. Thus use `auto& foo = bar;`, etc. instead of
  ~~`auto &foo = bar;`~~.
- Place `const` after the type name, e.g. `auto const foo = bar` instead of
  ~~`const auto foo = bar`~~ or `sometype const* p = &v` instead of
  ~~`const sometype* p = &v`~~.

## Variables

- Variable names must use `snake_case`.
- Variables should be named after nouns.
- Variables of type `lua_State*` should, by convention, be named just `L`. This
  is an exception to the above two rules.
- For variables that hold the count of something use names in the `n_things`
  style, not ~~`num_things`~~ or ~~`thing_count`~~.
- Private member variables have the prefix `m_`.
- Consider naming the variable that holds the return value of a function
  `result`, especially when the function is named after the value it returns.
- If a variable is only used once, consider inlining the initialization
  expression, thus avoiding the variable.
- Use `const` freely, but don't use `const` when it prohibits moving from a
  variable.

## Template parameters

- Template parameter names must use `PascalCase`, even if they are non-type ones.
- If a single template parameter is used and the meaning is clear from the
  context (both at template definition and usage side) it should be named just
  `T`, except if that has already been used in a template parameter of an
  enclosing entity, in which case `U` can be used. `V` and on must be avoided:
  Consider breaking up the nesting or use descriptive names.
- Template parameters should not have a `T` prefix or suffix (except being named
  just `T` per the above rule).
- Name template parameters whose only purpose is to enable SFINAE and
  which are otherwise unused `Enable`.
- Use `typename` instead of `class` whenever possible.
- Put a space between the `template` and the opening angle bracket.

## Macros

- Avoid macros! Whenever possible use function, function templates, `const` or
  `enum` instead.
- Macro names must be in `ALL_CAPS`.
- Macro names must be prefixed with the all caps translation of what would
  otherwise be the namespace (but without additional underscores), e.g.:

  ```c++
  namespace somenamespace { namespace detail {

  int some_function();

  } } // namespace somenamespace::detail
  ```

  This has the macro equivalent:

  ```c++
  #define SOMENAMESPACE_DETAIL_SOME_MACRO(x) write_log(#x, (x))
  ```

## Namespaces

- Use namespaces instead of name prefixes (e.g. `apollo::to` instead of
  ~~`apollo_to`~~). In particular, do not repeat namespace names in the
  namespace's members e.g. `apollo::error` instead of
  ~~`apollo::apollo_error`~~.
- Namespace names must be in all-lowercase without underscores.
- Resist the urge to make namespace names overly abbreviated; there are
  namespace aliases for the user to solve this, e.g.
  `namespace fs = boost::filesystem;`.
- Use a `detail` subnamespace to hide implementation details that must for
  technical reasons be exposed in public headers.
- Do not indent the contents of namespaces. Place a comment of the form 
  `// namespace namespacename` (for anonymous namespaces:
  `// anonymous namespace`) after the closing brace instead (see example above.)
- When defining a nested namespace and only the innermost namespace has members
  added, put the opening `namespace` statements including braces on one line and
  put the closing braces on one line. Use a closing comment of the form
  `// namespace somenamespace::somesubnamespace`.

## Jump statements (`return`, `break`, `continue`, `goto`), loops and recursion

- Never use `goto`! If you are tempted to use it for cleanup, use RAII instead. For
  exiting from nested loops, break loops into functions in such a way that you
  can use `return` instead.
- Avoid boolean control variables if they can be replaced with jump statements
  (other than `goto`).
- If an `if`'s or `else if`'s contents end with `return`, do not use `else`
  after it.
- Prefer early exits with `return`, `break`, or `continue` to nested statements.
- For `break`-controlled loops use `for (;;)` instead of e.g.
  ~~`while (true)`~~.
- Whenever it does not matter, use `while` instead of `do while`. But if it does
  save code (duplication), do not hesitate to use `do while`!
- Strongly prefer loops to recursion. You may use recursion only if an iterative
  implementation would need to use an explicit stack and the recursion depth is
  safely bounded so as to not cause a stack overflow for any inputs. Do not
  depend on the compiler transforming the recursion into iteration here: debug
  builds should not crash either.
- You may of course also use recursion if an iterative solution is not possible,
  i.e. for processing template parameter packs in-order on compilers that do not
  support ordered evaluation of braced initializer lists (cf. the `pass`
  function template at
  <https://en.wikipedia.org/wiki/Variadic_template#C.2B.2B> and
  `apollo::detail::variadic_pass`).

## The `switch` statement

- Do indent `case`s of a switch and indent the content one level more.
  Indent a terminating `break` (but not a return) the same level as the
  corresponding `case`:

  ```c++
  switch (foo) {
      case bar:
          do_a();
      break;

      case baz:
          do_b();
          return calculate_answer();

      default:
          do_c();
      break;
  }
  ```
- Do not use braces for the `case`s of a `switch`, unless required (for
  branch-local variables). In that case, place the `break` *after* the closing
  brace, on the same line (this does not apply to `case`s terminated with
  `return`):

  ```c++
  switch (foo) {
      case bar: {
          auto a = do_a();
          kill_a();
          return a;
      }

      case baz: {
          auto some_local = create_something();
          do_something_with(some_local, some_local.some_property());
      } break;

      default:
          do_c();
      break;
  ```
- If a `case` of a `switch` consists only of a `return` statement, it may be
  placed on the same line as the `case` keyword, but only if the whole `return`
  statement together with the `case` fits in single line.
- Use the [`BOOST_FALLTHROUGH`][boost-fallthrough] macro to mark fall-through.

[boost-fallthrough]: http://www.boost.org/doc/libs/release/libs/config/doc/html/boost_config/boost_macro_reference.html#boost_config.boost_macro_reference.boost_helper_macros

## Exceptions

- Use exceptions for error handling only! Do not use them as a replacement for
  other RTTI mechanisms or normal control flow statements.
- Prefer exceptions to error codes.
- Consider suffixing exception class names with `_error`.
- Name the exception caught in `catch`-handler just `e`.
- Don't use exceptions if the error is so frequent that they hurt performance.
- Use [`BOOST_THROW_EXCEPTION`][boost-throw] instead of `throw`.
- Use virtual inheritance for exception hierarchies
  [(rationale)][boost-virtual-exc].
- When rethrowing exceptions use `throw;` do not rethrow by throwing the caught
  exception again ~~`catch (exception const& e) { /* ... */ throw e; }`~~.
- Catch by reference. Use mutable references only if you need to modify the
  exception.
- Annotate functions that may not throw with [`BOOST_NOEXCEPT`][boost-noexcept].
- Annotate functions that always throw with
  [`BOOST_NORETURN`][`boost-noreturn`].

[boost-noexpect]: http://www.boost.org/doc/libs/release/libs/config/doc/html/boost_config/boost_macro_reference.html#boost_config.boost_macro_reference.macros_that_allow_use_of_c__11_features_with_c__03_compilers
[boost-noreturn]: http://www.boost.org/doc/libs/release/libs/config/doc/html/boost_config/boost_macro_reference.html#boost_config.boost_macro_reference.boost_helper_macros
[boost-throw]: http://www.boost.org/doc/libs/release/libs/exception/doc/BOOST_THROW_EXCEPTION.html
[boost-virtual-exc]: http://www.boost.org/doc/libs/release/libs/exception/doc/using_virtual_inheritance_in_exception_types.html

## Comments

- Prefer self-explaining code to comments.
- Whenever it is not clear why you had to do something in a certain way, use a
  comment to explain it, especially if another way exists that *seems* to be
  better -- without a comment, this is a ticking time-bomb..
- Only use C++-style line-comments (`//`, not ~~`/* */`~~).
- Put a space between the `//` and the comments content.
- Avoid commented-out code -- we have git! But if you do comment out code, do
  not use a space between the `//` and the commented-out code.
- If a comment fits on the same single line as the thing it comments, place it
  there.
- If a comment does not fit on the same line as the thing it comments or the
  thing it comments spans multiple lines, place the comment on lines of its own
  before the thing.
- Consider using `// Some region //` (slashes before and after comment) to mark
  regions of code.
