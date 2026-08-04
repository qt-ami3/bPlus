# Codebase reference

Per-file, per-function documentation of the interpreter. For the big picture (pipeline, conventions, how to add a built-in) see `architecture.md`.

## main.cpp

Interpreter core. Everything below lives here, ordered as it appears in the file.

- `void print_usage(const string& program_name)`
  Prints CLI usage to stderr.

- `int main(int argc, char* argv[])`
  1. **CLI checks** — argument count, the `-v`/`-verbose` flag, that the file opens, the `.bp` extension. Any failure prints usage or a one-line reason and exits `1`. These are the only paths that exit non-zero.
  2. **Pass loop** — `while (!instruction_loop_break)`, repeating until an `end` instruction sets the flag. Each turn calls `validate_and_count` for a fresh statement list, then builds a new `map<string, Variable>` and `set<string> active` and calls `instruction_loop`. Re-reading each pass is what makes editing a running program take effect; rebuilding the map is what makes a pass behave like a first run.
  3. **Validation failure** — errors are printed once and the loop `continue`s, holding and re-reading until the file is valid. A `reported` copy suppresses reprinting an unchanged error set, which would otherwise flood stderr thousands of times a second.
  4. With `-v`, prints a "Statements read;" dump from the statement list plus the final pass's counts, after the loop ends.

## include/ and src/

One header in `include/` and one implementation in `src/` per module, all pulled in by `main.cpp`.

- `bool has_extension(const std::string& filename, const std::string& ext)` — has_extension.h / has_extension.cpp
  True when `filename` ends with `ext`. Used by main to require `.bp` files.

- `std::string trim(const std::string& str)` — trim.h / trim.cpp
  Copy without leading/trailing spaces, tabs, `\n`, `\r`.

- `bool string_contains(const std::string& str, const std::string& needle)` — string_contains.h / string_contains.cpp
  Thin wrapper over `str.find(needle) != npos`. In main.cpp it now only decides assignment-vs-call (`=`) and whether a `shout` argument is a quoted literal; instruction names are matched by equality, not by substring search.

- `std::string between(const std::string& str, char open, char close)` — between.h / between.cpp
  Substring strictly between the first `open` and the following `close`, excluding both; `""` if either isn't found. Used to pull `"..."` contents and, in `assign_variable`, the right-hand side between `=` and `;`.

- `std::string between_matching(const std::string& str, char open, char close)` — between.h / between.cpp
  Same, but pairs the first `open` with the `close` that actually matches it, counting nesting and ignoring both characters inside `"..."` literals. `""` when there is no `open` or nothing closes it. Argument and condition extraction use this, which is what lets an argument carry brackets of its own: `shout((2 + 3) * 4);` and `if ((a + b) > 5)`. Using plain `between` there truncated the argument at the first `)`.

- `std::vector<std::string> split(const std::string& str, char delimiter)` — split.h / split.cpp
  Splits on one character, keeping empty tokens between consecutive delimiters.

- `std::vector<std::string> split_multi(const std::string& str, const std::string& delimiters)` — split.h / split.cpp
  Splits on any character in `delimiters`, dropping empty tokens. Used for verbose token echo, for `assign_variable`'s `[type] name` split, and by main for the `();` delimiter set.

- `std::string read_until(const std::string& filename, char stop_char, int count, int skip)` — read_until.h / read_until.cpp
  Reopens `filename` from the start, discards content through the first `skip` occurrences of `stop_char`, then returns everything through the next `count` occurrences (stop character included). **No longer called by anything.** Statements have been read once per pass into a list since 0.6.0; this was the per-statement re-read that made execution quadratic in file length. Left in place, not deleted.

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
  Evaluates a `+ - * /` expression with standard precedence, parentheses and unary minus, and resolves array elements. Operands are numeric literals, variable names, or `arr[index]` where the index is itself an expression — all resolved from `variables` (bool → 1/0). Returns `nullopt` in three distinct situations, which callers treat alike: the text contains neither an operator nor a bracket (a bare literal or lone variable, left for the caller), the text is not an expression at all (an illegal character such as `"` aborts tokenizing), or evaluation failed for a semantic reason — `Unknown variable in expression: ...`, `Index out of range: ...`, `Cannot use string in expression`, `Division by zero`, each reported to stderr here. Result type: Float if any operand was Float or the result is fractional, else Int, else Long beyond `int` range.

  Two things exist for arrays. A bracket makes the evaluator engage even with no operator, because `arr[i]` cannot be looked up by name until `i` is known. And a **lone element** — the whole expression being one `arr[...]` — is answered straight from the map before the arithmetic starts, returning the stored value unchanged; without that a string element would be rejected as not a number, and a whole number would round trip through `double`. Internals (`Token`, `Value`, `Parser`) are file-local to eval_expr.cpp.

- `void assign_variable(std::map<std::string, Variable>& variables, const std::string& statement)` — assign.h / assign.cpp
  Parses `name = value;` or `type name = value;` and applies it to `variables`. A type prefix always (re)declares the variable as static with that type (type error on mismatch). A bare name reassigns an existing static variable (type-checked against its stored type) or, for a new/dynamic name, tries `eval_expr` on the right-hand side first and falls back to `infer_literal`. **`eval_expr` is only reached on the dynamic path**: both the declared-type branch and the existing-static branch go straight to `parse_literal_as`, which requires a whole-string literal match, so `int n = 1 + 2;` and a later `n = 2 + 3;` are both type errors. Expressions in assignments therefore only work on dynamically typed variables. Errors (`Unknown type: ...`, `Invalid variable declaration: ...`, `Type error: ...`, `Could not infer type for: ...`) go to stderr; the function returns without modifying `variables` in that case.

- `bool validate_and_count(const std::string& filename, std::vector<std::string>& statements, int& statement_count, int& semicolon_count, std::vector<std::string>& errors)` — validate.h / validate.cpp
  Reads `filename` into `statements`, one entry per non-empty line: a `;`-terminated statement, a block header ending in `{`, a lone `}`, or a `use "library"` declaration. Counts semicolons and braces outside `"..."` literals and tracks brace depth. Fills `errors` and returns false for `Missing ';'`, `Extra ';'`, `Braces belong on a line of their own`, `Unmatched '}'` and `Unclosed '{'`. Braces on a line **with** a `;` are data, not a block, which is what makes `arr[3] = {1,2,3};` legal; a block keyword whose braces share its line is still refused so it says why. `in_string` is declared outside the line loop, so an unterminated `"` carries into following lines. Called once per pass by main, and again per file `pass` pulls in.

- `void instruction_loop(bool& flag, const std::string& filename, const std::vector<std::string>& statements, std::map<std::string, Variable>& variables, bool verbose, std::set<std::string>& active)` — instruction_loop.h / instruction_loop.cpp
  Walks the statement list, executing as it goes. Holds every built-in and all statement classification. Recursive: the `pass` arm calls it again for another file with the same `variables`. File-local helpers:
  - `canonical_path` — `filesystem::weakly_canonical`, so `"x.bp"` and `"./x.bp"` compare equal in `active` and a cycle cannot slip through.
  - `has_assignment` — a lone `=` outside quotes, skipping `==`, `!=`, `<=`, `>=`. Without it every condition would look like an assignment.
  - `split_top_level` — splits on a separator only outside quotes and outside any bracket, so `shout("a, b", f(1, 2))` is two arguments. One helper serves comma-separated arguments, `&&` and `||`.
  - `store_result`, `numeric_arguments` — support for library instructions shaped `name(variable, a, b)`. The first writes a produced value into a variable, creating it if new and refusing to change a static one's declared type; the second resolves every argument after the first as a number.
  - `resolve_operand` — quoted string, then `eval_expr`, then variable lookup, then `infer_literal`.
  - `as_number`, `find_comparison`, `evaluate_condition` — condition support; numbers compare as `double`, strings as text, a mismatch sets `ok` false and the caller reports `Bad condition:`. `evaluate_condition` splits on `||` then `&&` before anything else and recurses, which is what gives `||` the loosest binding; a term wrapped entirely in its own brackets is unwrapped and recursed into. There is no `!` and no `and`/`or` word forms.
  - `matching_close` — index of the `}` closing a block, by depth counting, so skipping a false block skips nested blocks whole.
  - `libraries`, `providing_library` — the registry of C++ libraries and their instructions; `use "name"` fills a local `enabled` set and an instruction is refused unless its library is in it.
  - `scan_libraries` — pre-pass writing a bool per `pass` target, named after the file's stem, recording whether it exists.

- `array.h` / `array.cpp` — arrays.
  - `std::string array_element(const std::string& name, long long index)` — the mangled name of a slot, `arr[0]`.
  - `std::string array_header(const std::string& name)` — the mangled name of the declaration, `arr[]`.
  - `bool array_statement(std::map<std::string, Variable>& variables, const std::string& statement)` — handles `arr[3];`, `int arr[3];`, `arr[3] = {1,2,3};` and `arr[0] = value;`, returning false when the statement is not an array statement so the caller carries on. A `[` **left of the `=`** is what marks one, and a `(` in the same span rules it out, so `shout(arr[0]);` is left alone. Declaring writes a zero of the element type into every slot. Static arrays type-check through `parse_literal_as`, so they reject expressions exactly as static scalars do. Errors: `Index out of range:`, `Unknown array:`, `Bad array length:`, `Unknown type:`, `Invalid array declaration:`, `Too many values for ...`.

  Everything lives in the ordinary variable map, so `shout`, conditions and assignment resolve an element without knowing arrays exist. `arr[]` holds the element type in `type`, whether a type was declared in `is_static`, and the length in `value`.

## Outside the core

- `long get_process_ram_kb()` / `void print_system_info(bool enabled)` — process_ram_kb.h / process_ram_kb.cpp
  Reads `VmRSS` from `/proc/self/status`. `print_system_info` prints `RAM: N KB` and returns immediately when `enabled` is false. Called by main at each end of a pass. Linux specific.

- `void set_buffered_input(bool enable)` / `void clear_screen()` — libraries/shell_utilities.h / libraries/shell_utilities.cpp
  Terminal helpers, compiled from `src/libraries/`. `clear_screen` writes `\033[2J\033[H` and backs the `clear` instruction. `set_buffered_input` turns off canonical mode and echo so a keypress arrives without Enter; it is **not** exposed to bP yet, and note it restores only on an explicit `set_buffered_input(true)` — an interpreter that exits while unbuffered leaves the user's shell with echo off. The library's other instruction, `exec`, is not a library function at all: it lives in the dispatch arm and calls `system()` through a local `run_shell` helper, which reports `exec failed:` on a non-zero status. `system` is `warn_unused_result`, so the status cannot simply be discarded.

- `random.h` / `random.cpp` — libraries/random.
  - `void randomint(int& result, int from, int up_to)` — whole number, both ends included.
  - `void randomdouble(double& result, double from, double up_to)` — real number in `[from, up_to)`.
  - `void randombool(bool& result)` — even coin flip.
  - `void doublebellcurve(double& result, double mean, double deviation)` — normally distributed.

  Each writes through its first parameter rather than returning, because a bP instruction has no way to hand a value back to an expression. All four share one `mt19937` from a function-local static: building a generator from `random_device` per call is slow, and where `random_device` is not a real entropy source it reseeds identically every time, so the "random" number never changes. Reversed bounds are swapped rather than left undefined, and a zero deviation returns the mean, since `normal_distribution` requires a positive spread.

## unit_tests/

Run with `cd unit_tests && python3 main.py`.

**The suite does not run as of 0.5.0.** No test program contains `end();`, so each one loops forever and the harness blocks on the first case rather than failing it — `subprocess.run` is called without a timeout. Three expectations are stale on top of that: `equals_in_string.bp` records the `shout("a = b");` bug fixed in 0.5.0, and both `break` cases record the old off-by-one counts.

- `bp` — a **copy** of the built `../bp`, not a symlink. Re-copy after rebuilding or the suite tests a stale interpreter.
- `main.py` — the harness.
  - `BP` — path to the interpreter under test.
  - `USAGE`, `VERBOSE_HELLO` — expected multi-line outputs, kept as constants because several cases share them.
  - `run_command_and_compare(command, expected_output, expected_error, expected_status)` — runs `command`, compares stripped stdout, stripped stderr and the exit code, prints each mismatch as expected-vs-got, and returns `0` on pass or `1` on fail. Deliberately does **not** pass `check=True` to `subprocess.run`, since a nonzero exit is itself an expected result in several cases.
  - `case(arguments, expected_output, expected_error = "", expected_status = 0)` — appends one case to `tests`. Arguments are appended to `BP`, so `case([], ...)` tests the no-argument usage path.
  - The bottom loop runs every case, prints `<label> passed ✓` or `Unexpected output from <label>`, and ends with the pass count.
- `*.bp` — one program per case, grouped in `main.py` as language features, non-fatal errors, semicolon checking, command line handling, and known limitations.

Expectations record current behavior, including the wrong behavior in `parens.bp`, `unknown_variable.bp` and `equals_in_string.bp`. A fix to any of those is expected to fail its test until the expectation is updated alongside it.
