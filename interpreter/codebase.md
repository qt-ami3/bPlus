# Codebase reference

Per-file, per-function documentation of the interpreter. For the big picture (pipeline, conventions, how to add a built-in) see `architecture.md`.

## main.cpp

Interpreter core. Everything below lives here, ordered as it appears in the file.

- `void print_usage(const string& program_name)`
  Prints CLI usage to stderr.

- `int main(int argc, char* argv[])`
  1. **CLI checks** — argument count, the `-v`/`-verbose` flag, that the file opens, the `.bp` extension.
  2. **Count pass** — streams the file once (respecting `"..."` literals) to count semicolons into `semicolon_count` / `statement_count` (always equal).
  3. **Statement pass** — for `i` in `0..semicolon_count`, calls `read_until(filename, ';', 1, i)` to fetch statement `i` (reopens and re-reads the file each time), then runs independent `string_contains()` checks for `=` (→ `assign_variable`), `shout` (→ prints a quoted literal or a variable's value), and `break` (→ prints newlines). See `architecture.md` for why these checks can overlap on one statement.
  4. With `-v`, echoes each statement's `split_multi` tokens after it runs, then dumps every statement text again and both counts at the end.

## include/ and src/

One header in `include/` and one implementation in `src/` per module, all pulled in by `main.cpp`.

- `bool has_extension(const std::string& filename, const std::string& ext)` — has_extension.h / has_extension.cpp
  True when `filename` ends with `ext`. Used by main to require `.bp` files.

- `std::string trim(const std::string& str)` — trim.h / trim.cpp
  Copy without leading/trailing spaces, tabs, `\n`, `\r`.

- `bool string_contains(const std::string& str, const std::string& needle)` — string_contains.h / string_contains.cpp
  Thin wrapper over `str.find(needle) != npos`. Used throughout main.cpp for statement classification.

- `std::string between(const std::string& str, char open, char close)` — between.h / between.cpp
  Substring strictly between the first `open` and the following `close`, excluding both; `""` if either isn't found. Used to pull `"..."`/`(...)` contents and, in `assign_variable`, the right-hand side between `=` and `;`.

- `std::vector<std::string> split(const std::string& str, char delimiter)` — split.h / split.cpp
  Splits on one character, keeping empty tokens between consecutive delimiters.

- `std::vector<std::string> split_multi(const std::string& str, const std::string& delimiters)` — split.h / split.cpp
  Splits on any character in `delimiters`, dropping empty tokens. Used for verbose token echo, for `assign_variable`'s `[type] name` split, and by main for the `();` delimiter set.

- `std::string read_until(const std::string& filename, char stop_char, int count, int skip)` — read_until.h / read_until.cpp
  Reopens `filename` from the start, discards content through the first `skip` occurrences of `stop_char`, then returns everything through the next `count` occurrences (stop character included). Called once per statement by main's statement pass — the source of the two-pass, file-reopening read described in `architecture.md`.

- `variable.h` / `variable.cpp` — runtime variable representation.
  - `enum class VarType { Int, Long, Float, String, Bool }`
  - `using VarValue = std::variant<int, long long, double, std::string, bool>`
  - `struct Variable { VarType type; bool is_static; VarValue value; }`
  - `std::string var_type_name(VarType type)` — type name for error messages (`"int"`, `"long"`, `"float"`, `"string"`, `"bool"`).
  - `std::optional<VarType> type_from_keyword(const std::string& word)` — maps a type keyword to `VarType`; accepts both `float` and `double` as `VarType::Float`; `std::nullopt` if not a type keyword.
  - `void print_variable(const Variable& variable)` — prints the held value via `std::visit`; `bool` prints as `"true"`/`"false"`.
  - `std::optional<long long> as_integer(const Variable& variable)` — value as `long long` for `int`/`long`/`float`/`bool` (bool → 1/0); `std::nullopt` for `string`. Used by `break` to resolve a variable argument.

- `parse_literal.h` / `parse_literal.cpp` — literal text → typed value.
  - `std::optional<VarValue> parse_literal_as(const std::string& literal, VarType type)` — parses `literal` strictly as `type` (whole-string match required); `std::nullopt` on any mismatch or parse failure.
  - `std::optional<VarValue> infer_literal(const std::string& literal, VarType& out_type)` — infers a type from the literal's shape and parses it: quoted → String, `true`/`false` → Bool, contains `.` → Float, else tries Int then Long. Writes the inferred type to `out_type` on success.

- `void assign_variable(std::map<std::string, Variable>& variables, const std::string& statement)` — assign.h / assign.cpp
  Parses `name = value;` or `type name = value;` and applies it to `variables`. A type prefix always (re)declares the variable as static with that type (type error on mismatch). A bare name reassigns an existing static variable (type-checked against its stored type) or, for a new/dynamic name, infers the type from the value. Errors (`Unknown type: ...`, `Invalid variable declaration: ...`, `Type error: ...`, `Could not infer type for: ...`) go to stderr; the function returns without modifying `variables` in that case.
