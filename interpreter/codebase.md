# Codebase reference

Per-file, per-function documentation of the interpreter. For the big picture (pipeline, conventions, how to add a built-in) see `architecture.md`.

## main.c

Interpreter core. Everything below lives here, ordered as it appears in the file.

### Types

- `Variable`
  Runtime state of one variable: `value`, its current `type` (`"string"` or `"int"`, inferred from the last assigned value), and `is_static` (declared with a type in front; reassignments must keep that type).

### Helpers

All pure; no bP knowledge beyond literal syntax.

- `void print_usage(const string& program_name)`
  Prints CLI usage to stderr.

- `string trim(const string& text)`
  Returns a copy without leading/trailing spaces, tabs, and line endings.

- `bool is_string_literal(const string& text)`
  True when the text is a `"..."` literal.

- `bool is_int_literal(const string& text)`
  True when the whole text is an optional `-` followed by digits. Used to type assignment values and validate break's argument.

- `string literal_value(const string& text)`
  A string literal without its surrounding quotes.

- `vector<string> split_args(const string& text)`
  Splits `a, b, c` on commas outside quotes into trimmed segments. Empty input means zero arguments; an empty segment (`"a",` or `,,`) comes through as an empty string and fails validation at the use site.

### main

1. **CLI checks** — argument count, the `-v`/`-verbose` flag, the file opens, the `.bp` extension.
2. **Statement loop** — accumulates characters into `current` until a `;` outside a string literal ends the statement (`{`/`}` clear the chunk; quoted text passes through untouched). Each finished statement is trimmed and handled immediately:
   - **Assignment** (an `=` before any `(`): splits `[type] name = value`; validates the optional static type (`string`/`int` only), the variable name (no whitespace, quotes, or parentheses), and the value's literal type; enforces the declared type at declaration and the remembered type on every static reassignment; writes the `variables` map.
   - **shout** — every argument must be a string literal (concatenated verbatim) or a declared variable (its value); at least one argument required.
   - **break** — no argument or one non-negative integer literal; prints that many newlines.
   - Anything else is an unknown-instruction error.
3. **After the loop** — leftover non-whitespace text is an unterminated-statement error. With `-v`, prints the statements-read and statements-executed counts.

Errors print one `Error: ...` line to stderr and exit 1 immediately; statements before the failing one have already executed.

## include/ and src/

Shared helpers with one header in `include/` and one implementation in `src/`. The built-in instructions themselves live in main.c's statement loop.

- `bool has_extension(const std::string& filename, const std::string& ext)` — has_extension.h / has_extension.c
  True when `filename` ends with `ext`. Used by main to require `.bp` files.
