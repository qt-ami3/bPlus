# Codebase reference

Per-file, per-function documentation of the interpreter. For the big picture (pipeline, conventions, how to add a built-in) see `architecture.md`.

## main.cpp

Interpreter core. Everything below lives here, ordered as it appears in the file.

- `void print_usage(const string& program_name)`
  Prints CLI usage to stderr.

- `int main(int argc, char* argv[])`
  1. **CLI checks** — argument count, the `-v`/`-verbose` flag, that the file opens, the `.bp` extension. Any failure prints usage or a one-line reason and exits `1`.
  2. **Validation and count pass** — reads the file line by line (respecting `"..."` literals), recording every `;` position to count `semicolon_count` / `statement_count` (equal on a valid file) and to collect missing/extra semicolon errors. If any were collected they are all printed and main exits `1` before execution.
  3. **Statement pass** — for `i` in `0..semicolon_count`, calls `read_until(filename, ';', 1, i)` to fetch statement `i` (reopens and re-reads the file each time) and trims it. A statement containing `=` goes to `assign_variable`; otherwise the text before the first `(` is the instruction name and `between(statement, '(', ')')` is the argument, dispatched by name to `shout`, to `break`, or to `Unknown function: <name>`. A statement with no `(` reports `Not a statement: <statement>`. See `architecture.md` for the full branch behavior.
  4. With `-v`, echoes each statement's `split_multi` tokens after it runs, then dumps every statement text again and both counts at the end.

## include/ and src/

One header in `include/` and one implementation in `src/` per module, all pulled in by `main.cpp`.

- `bool has_extension(const std::string& filename, const std::string& ext)` — has_extension.h / has_extension.cpp
  True when `filename` ends with `ext`. Used by main to require `.bp` files.

- `std::string trim(const std::string& str)` — trim.h / trim.cpp
  Copy without leading/trailing spaces, tabs, `\n`, `\r`.

- `bool string_contains(const std::string& str, const std::string& needle)` — string_contains.h / string_contains.cpp
  Thin wrapper over `str.find(needle) != npos`. In main.cpp it now only decides assignment-vs-call (`=`) and whether a `shout` argument is a quoted literal; instruction names are matched by equality, not by substring search.

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

- `std::optional<VarValue> eval_expr(const std::string& expr, const std::map<std::string, Variable>& variables)` — eval_expr.h / eval_expr.cpp
  Evaluates a `+ - * /` expression with standard precedence, parentheses and unary minus. Operands are numeric literals or variable names resolved from `variables` (bool → 1/0). Returns `nullopt` in three distinct situations, which callers treat alike: the text contains no operator (a bare literal or lone variable, left for the caller), the text is not an expression at all (an illegal character such as `"` aborts tokenizing), or evaluation failed for a semantic reason — `Unknown variable in expression: ...`, `Cannot use string in expression`, `Division by zero`, each reported to stderr here. Result type: Float if any operand was Float or the result is fractional, else Int, else Long beyond `int` range. Internals (`Token`, `Value`, `Parser`) are file-local to eval_expr.cpp.

- `void assign_variable(std::map<std::string, Variable>& variables, const std::string& statement)` — assign.h / assign.cpp
  Parses `name = value;` or `type name = value;` and applies it to `variables`. A type prefix always (re)declares the variable as static with that type (type error on mismatch). A bare name reassigns an existing static variable (type-checked against its stored type) or, for a new/dynamic name, tries `eval_expr` on the right-hand side first and falls back to `infer_literal`. **`eval_expr` is only reached on the dynamic path**: both the declared-type branch and the existing-static branch go straight to `parse_literal_as`, which requires a whole-string literal match, so `int n = 1 + 2;` and a later `n = 2 + 3;` are both type errors. Expressions in assignments therefore only work on dynamically typed variables. Errors (`Unknown type: ...`, `Invalid variable declaration: ...`, `Type error: ...`, `Could not infer type for: ...`) go to stderr; the function returns without modifying `variables` in that case.

## unit_tests/

Run with `cd unit_tests && python3 main.py`.

- `bp` — a **copy** of the built `../bp`, not a symlink. Re-copy after rebuilding or the suite tests a stale interpreter.
- `main.py` — the harness.
  - `BP` — path to the interpreter under test.
  - `USAGE`, `VERBOSE_HELLO` — expected multi-line outputs, kept as constants because several cases share them.
  - `run_command_and_compare(command, expected_output, expected_error, expected_status)` — runs `command`, compares stripped stdout, stripped stderr and the exit code, prints each mismatch as expected-vs-got, and returns `0` on pass or `1` on fail. Deliberately does **not** pass `check=True` to `subprocess.run`, since a nonzero exit is itself an expected result in several cases.
  - `case(arguments, expected_output, expected_error = "", expected_status = 0)` — appends one case to `tests`. Arguments are appended to `BP`, so `case([], ...)` tests the no-argument usage path.
  - The bottom loop runs every case, prints `<label> passed ✓` or `Unexpected output from <label>`, and ends with the pass count.
- `*.bp` — one program per case, grouped in `main.py` as language features, non-fatal errors, semicolon checking, command line handling, and known limitations.

Expectations record current behavior, including the wrong behavior in `parens.bp`, `unknown_variable.bp` and `equals_in_string.bp`. A fix to any of those is expected to fail its test until the expectation is updated alongside it.
