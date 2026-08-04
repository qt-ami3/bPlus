# Interpreter architecture

The interpreter is a C++17 program: `main.cpp` plus small helper modules under `include/`/`src/`. A run is a loop, not a single execution: main reads the whole file into a list of statements, hands that list to `instruction_loop()`, and then does it all again, until an `end` instruction sets the flag it watches. Re-reading each pass is what makes editing a running program take effect. Since 0.5.0 the file is read once per pass rather than once per statement, so `read_until()` is no longer on the execution path.

For per-file, per-function documentation see `codebase.md`.

## Directory layout

```
.. (root)
│
main.cpp             CLI handling and the pass loop, nothing else.
├─ Makefile          make / make clean.
├─ include/          Core headers, one per module.
│  └─ libraries/     Headers for optional libraries.
├─ src/              Core implementations.
│  └─ libraries/     Optional, platform specific, outside the core.
└─ unit_tests/       Python harness plus one .bp program per test case.
```

Core modules (header + impl pair each): `has_extension`, `trim`, `string_contains`, `between`, `split`, `read_until`, `variable`, `parse_literal`, `assign`, `eval_expr`, `validate`, `array`, `instruction_loop`. Outside the core: `process_ram_kb` (memory readout for `-v`), `libraries/shell_utilities` (`clear_screen`, `set_buffered_input`) and `libraries/random` (four generators sharing one seeded `mt19937`). `main.cpp` is down to CLI handling and the re-read loop; the statement loop and every built-in live in `instruction_loop`, file reading and checking in `validate`, and array declaration and element assignment in `array`.

`read_until` is no longer called by anything. It is left in place, not deleted.

## Pipeline

1. **CLI handling** (`main`) — argument count, the `-v`/`-verbose` flag, the `.bp` extension, and that the file opens. Unchanged from 0.2.2.

2. **The pass loop** (`main`) — `while (!instruction_loop_break)`, repeating steps 3 and 4 until an `end` instruction sets the flag. Each pass builds a fresh `map<string, Variable>` and a fresh `set<string>` of active files, so a pass is indistinguishable from a first run. There is no sleep: passes run back to back as fast as the machine allows.

3. **Read and validate** (`validate_and_count`, `src/validate.cpp`) — reads the file line by line and returns it as a `vector<string>`, one entry per non-empty line: a `;`-terminated statement, a block header ending in `{`, or a lone `}`. Semicolons and braces are counted outside `"..."` literals. Errors raised here:
   - `Missing ';' for statement line N` — a statement line with no `;`.
   - `Extra ';' for statement line N` — one per surplus semicolon.
   - `Braces belong on a line of their own, line N` — braces on a line with no `;`, or a block keyword whose braces share its line. Braces on a statement line are **data**, which is what makes `arr[3] = {1,2,3};` legal; the block-keyword check exists so `if (x) { shout("a"); }` still says why it is refused instead of failing later as an unknown instruction.
   - `Unmatched '}' on line N` and `Unclosed '{'` — from brace-depth tracking.

   Failure is **not fatal and not an exit**: main prints the errors once, holds, and keeps re-reading until the file validates. The message is reprinted only if the set of errors changes, otherwise a broken file would flood stderr thousands of times a second.

   `in_string` is declared outside the line loop, so an unterminated `"` carries its state into the following lines.

4. **Execute** (`instruction_loop`, `src/instruction_loop.cpp`) — walks the statement list. Per entry:

   - `}` — block end, nothing to do.
   - ends with `{` — a block header. The text before the first `(` is the keyword. `if` evaluates its condition; if false (or unresolvable) `i` jumps to the index of the matching `}` found by `matching_close()`, so nested blocks inside a skipped block are skipped whole. Any other keyword reports `Unknown block:` and skips likewise.
   - `has_assignment()` true — a lone `=` outside quotes, ignoring `==`, `!=`, `<=`, `>=`. Goes to `assign_variable()`. This distinction is what lets a condition contain `==` without being mistaken for an assignment.
   - otherwise a call — text before the first `(` is the **instruction name**, `between_matching(statement, '(', ')')` the argument, matched in one `if`/`else if` chain. A statement with no `(` reports `Not a statement:`. Matching rather than first-`)` extraction is what lets an argument hold brackets of its own; instructions that take several arguments split what comes back with `split_top_level(arg, ",")`.

   The built-ins:
   - `shout` — argument containing `"` prints the text between the first two quotes; otherwise `eval_expr()`, and on `nullopt` a variable lookup. A bare literal resolves to neither, so it prints nothing.
   - `break` — empty argument prints one newline, otherwise N newlines from `stoi`, falling back to a variable through `as_integer()`.
   - `end` — sets `flag` and breaks the statement loop, so it stops the file it appears in. At the top level that flag is main's, ending the pass loop.
   - `pass` — see "Files that run files" below.
   - anything in the library registry — see "Libraries written in C++" below.

   Two statement shapes never reach the call dispatch. `use "library"` is caught earlier by prefix, since it has no parentheses. An array statement is caught by `array_statement()` ahead of the assignment check, since `arr[0] = 5;` would otherwise look like an ordinary assignment.

5. Conditions (`evaluate_condition`) — `find_comparison()` locates `==`, `!=`, `<`, `>`, `<=` or `>=` outside string literals. With no operator the condition is the truthiness of a single operand. Both sides go through `resolve_operand()`, which tries, in order: quoted string literal, `eval_expr()`, variable lookup, `infer_literal()`. Numbers compare as `double` so floats are not truncated; two strings compare as text; a string against a number sets `ok` false. An unresolvable condition reports `Bad condition:` and the block is skipped.

6. With `-v`, each statement is traced **before** it runs, as `[n] ` plus its `split_multi` tokens (delimiter set `();`), each token trimmed. The line is preceded by a newline so it starts at column 0 whatever the statement before it left dangling — `shout` writes no trailing newline, and without this the trace read `17993shout x`. Block headers and `}` are traced too, so a skipped block shows as a gap in the numbering. Once the pass loop finishes, verbose mode prints a "Statements read;" dump straight from the statement list, then the statement and semicolon counts of the final pass. `print_system_info` reports the interpreter's own memory at each end of a pass.

## Files that run files

`pass("other.bp")` does not include a file, it **repeats** one. (It was called `use` until 0.6.0, which now means the library declaration below.) The arm resolves the target, refuses it if it is already running, then loops:

```
while (!target_flag) {
  validate_and_count(target, ...)      //  re-read every time round
  instruction_loop(target_flag, target, ..., variables, ..., active)
}
```

Three things fall out of that shape:

- **`variables` is passed straight through**, so the used file reads and writes the caller's variables. There is no scoping and no shadowing: same name, same variable.
- **`target_flag` is local**, so the used file's `end` ends its own repeat and cannot touch the caller's flag. A used file with no reachable `end` never returns.
- **The target is re-read on every iteration.** This is not just for live editing: editors and shell redirection truncate a file before writing it, and a pass that read the file in that instant would see zero statements, never reach an `end`, and spin silently forever. Re-reading is what lets it recover.

`active` is a `set<string>` of the files currently running, threaded through every call. Each call inserts its own path on entry and erases it on exit, and paths go through `filesystem::weakly_canonical` first so `"x.bp"` and `"./x.bp"` compare equal. Without that a cycle recurses until the stack runs out. A refused target reports `Cyclic use:` and execution continues.

A target that fails validation reports its errors once and the repeat holds, re-reading, exactly as main does. That includes a target that does not exist, so a typo waits forever rather than carrying on.

`scan_libraries()` runs before the statement loop, once per call. It finds every `pass("name.bp")` in the statement list, checks whether the file exists, and writes a bool named after the file's stem — `pass("libs/maths.bp")` sets `maths`. That is what lets a program guard a `pass` it is not sure about. It re-runs on every call, so those variables cannot be durably overwritten from bP.

## Libraries written in C++

`src/libraries/` holds optional code compiled into the interpreter. A registry in `instruction_loop.cpp` maps a library name to the instructions it provides:

```
static const map<string, set<string>> libraries = {
  {"shell_utilities", {"clear", "exec"}},
  {"random", {"randomint", "randomdouble", "randombool", "doublebellcurve"}},
};
```

`use "library"` — no parentheses, no semicolon — inserts a name into a `set<string> enabled` local to `instruction_loop`. `providing_library(name)` finds the owner of an instruction, and the dispatch arm refuses it unless the file declared it, reporting `clear needs: use "shell_utilities"`. Being local to the call means the declaration does **not** cross a `pass`; each file declares for itself.

Nothing is loaded at runtime — the C++ is already linked in. `use` decides what a program is allowed to call. Adding an instruction is a name in the registry set plus an arm in the dispatch; the refusal message comes free.

An instruction cannot return a value, because the dispatch executes statements and hands nothing back to an expression. A library function that computes something therefore takes the **variable to write into as its first argument** — `randomint(a, 1, 6);` sets `a`. Two helpers support that shape: `store_result()` writes a produced value into a name, creating it if new and refusing to change a static variable's type, and `numeric_arguments()` resolves everything after the first argument as numbers. Anything shaped like a function that belongs in an expression would need `eval_expr` to resolve a call, the same way it now resolves an index.

## Data structures

- `Variable` (`include/variable.h`) — `{ VarType type; bool is_static; VarValue value; }`.
  `VarType` is `Int | Long | Float | String | Bool`. `VarValue` is `std::variant<int, long long, double, std::string, bool>`. Variables live in a `map<string, Variable>` built fresh inside main's pass loop and passed by reference to `instruction_loop` and on to every file `use` pulls in. There is no global state anywhere in the interpreter, which is what makes `instruction_loop` safe to call recursively.

- `Token` / `Value` / `Parser` (`src/eval_expr.cpp`, file-local) — the expression evaluator's internals, deliberately not exported. `Token` is `Number | Name | Op | LParen | RParen | LBracket | RBracket`; `Value` is a running `double` plus an `is_float` flag that decides whether the result comes back as Float or as Int/Long. `Parser` is recursive descent (`parse_sum` → `parse_product` → `parse_atom`), which is what gives `* /` precedence over `+ -`; parentheses and unary minus are handled in `parse_atom`.

## Language rules the code enforces

- **Variables**: `name = value;` infers a type from the literal (quoted → String, `true`/`false` → Bool, contains `.` → Float, else Int then Long) and declares dynamically. `type name = value;` (`int`, `long`, `float`/`double`, `string`, `bool`) declares statically; a value of the wrong type errors, at declaration and on every reassignment.
- **Expressions**: `+ - * /` with standard precedence, parentheses and unary minus, over numeric literals and variable names. Available on the right of an assignment and as a `shout` argument. Division is real division: the result is Float when any operand was Float or the result is fractional, otherwise Int, or Long when it exceeds `int` range. Bools resolve to 1/0; a string in an expression is an error. `eval_expr` returns `nullopt` when the text holds neither an operator nor a bracket, which is how a bare literal or lone variable name falls through to the caller's own handling. A bracket counts because `arr[i]` has no operator but still cannot be looked up by name until `i` is known. A lone element is answered directly from the map before arithmetic starts, so a string element is returned whole rather than rejected as not a number, and a whole number avoids a round trip through `double`. Note that assignment only evaluates expressions for **dynamically typed** variables — a static declaration or a static reassignment parses its right-hand side as a whole-string literal, so `int n = 1 + 2;` is a type error.
- **shout**: takes any number of comma-separated arguments, printed in turn with nothing between them, so spacing lives inside the literals. Each argument is a quoted literal, an expression, an array slot or a variable name, resolved independently. A bare number is still the one gap — `shout(5);` prints nothing, because the evaluator only engages on an operator or a bracket and there is no fallback to plain literal parsing. Arguments split on commas at bracket depth zero and outside quotes, so `shout("a, b")` is one argument.
- **break**: prints exactly what it is asked for. `break()` and `break(1)` print one newline, `break(2)` two, `break(0)` none. The 0.3.0–0.4.3 off-by-one is gone.
- **Blocks**: one statement per line still holds, and braces extend it rather than replacing it — an opening brace ends its line, a closing brace has a line to itself, and neither carries a `;`. `if (x) { shout("a"); }` is rejected rather than parsed. This is what keeps the reader line-based.
- **Conditions**: `&&` and `||`, then `==`, `!=`, `<`, `>`, `<=`, `>=`, or a lone operand tested for truthiness (non-zero, not `false`, not empty text). `||` binds loosest, `&&` next, comparisons tightest, and brackets group — `if ((a > 1 || b > 5) && name == "carl")`. Both operators stop as soon as the answer is settled, so an unresolvable term after a decided one is never reported. Operands may be expressions, variables, literals, array slots or quoted strings. There is no `!`, and no `and`/`or` word forms.
- **end / pass**: `end` stops the file it is written in. `pass` repeats a file until that file's own `end`, sharing all variables. See "Files that run files".
- **Arrays**: `arr[3];` declares, `int arr[3];` fixes the element type, `arr[3] = {1,2,3};` fills. Elements are stored in the ordinary variable map under mangled names — `arr[0]`, `arr[1]` — with `arr[]` holding the declaration: element type in `type`, whether a type was given in `is_static`, length in `value`. Declaring writes a zero of the element type into every slot, so an unset slot reads as 0 rather than as a missing variable, and every existing lookup path resolves an element without knowing arrays exist. Indices may be expressions. Out of range reports `Index out of range:`, an undeclared name `Unknown array:`. There is no whole-array printing and no length instruction, though the length is stored.

## Conventions

- Helper modules stay small and pure (`has_extension`, `trim`, `string_contains`, `between`, `split`/`split_multi`, `variable`, `parse_literal`, `eval_expr`, `validate`, `array`); `instruction_loop` holds every side effect and all statement classification; `main.cpp` holds CLI handling and the pass loop and nothing else. `process_ram_kb` and everything under `libraries/` are outside that core.
- **Storing anything new** — the `map<string, Variable>` is the only state there is. Arrays live in it under mangled names rather than in a structure of their own, which is why printing, conditions and assignment needed no changes to support them. Prefer that trick to widening `VarValue`, which is a `std::variant` and cannot hold itself.
- **Errors** come in two kinds. File-level errors — the semicolon and brace checks in `validate` — stop execution but do **not** exit: they are printed once and the interpreter holds, re-reading, until the file is fixed. The message is reprinted only when the set of errors changes. Everything else (`Type error: ...`, `Unknown type: ...`, `Could not infer type for: ...`, `Unknown function: ...`, `Unknown block: ...`, `Bad condition: ...`, `Cyclic use: ...`, `Unknown variable in expression: ...`, `Division by zero`) goes to stderr un-prefixed and execution **continues** to the next statement. Only the CLI checks in `main` exit non-zero.
- Unresolved *names* are still silent where unresolved *instructions* are not: `shout(q);` on an undefined variable prints nothing and reports nothing, while `q(1);` reports `Unknown function: q`.
- **Adding a built-in instruction**: add an `else if (name == "...")` arm to the dispatch chain in `instruction_loop`, before the `Unknown function` fallback. The name is already parsed out, so arms cannot overlap. Past roughly five built-ins, replace the chain with a `map<string, ...>` lookup rather than letting it grow. Update `../bP_user_guide.md`, and add a case to `unit_tests/main.py`.
- **Adding a block keyword**: add an arm beside `if` in the block-header branch. `matching_close()` already finds the end of a block, so skipping one costs a line; a looping construct would set `i` back to the header index instead of forward.

## Compiling

`make` builds everything; `make clean` removes the binary. The Makefile wraps `g++ -Wall -std=c++17 main.cpp src/*.cpp src/libraries/*.cpp -o bp`, which still works typed by hand — the point of it is that neither `-std=c++17` nor the library directory can be left off by accident. C++17 is required, not incidental (`std::variant`, `std::optional`, `if constexpr`, and since 0.5.0 `std::filesystem` for the cycle check). See `compile.md`.

The Makefile splits the two so the distinction is visible in the build itself:

```
CORE = main.cpp $(wildcard src/*.cpp)
LIBS = $(wildcard src/libraries/*.cpp)
```

`src/libraries/` holds optional, platform-specific code that interpreting a `.bp` file does not depend on — currently `shell_utilities` (`clear_screen`, `set_buffered_input`), with headers under `include/libraries/`. Dropping a `.cpp` and a header into those two directories is the whole of adding one; the wildcards need no editing.

Both wildcards match `*.cpp` only, so the superseded `src/has_extension.c` stays out of the build.

The old `main.c` (0.2.2) is not part of this build and no longer compiles on its own — it has an uncommitted, broken `pi` instruction stub. It is left in place but superseded by `main.cpp`.

## Testing

`cd unit_tests && python3 main.py`. Each case runs the interpreter on one `.bp` program and compares stripped stdout, stripped stderr and the exit code against expected values; the run ends with a pass count. The binary under test is `unit_tests/bp`, a **copy** of `../bp` — re-copy it after rebuilding or the suite silently tests the old interpreter.

**The suite does not run as of 0.5.0.** None of the 31 programs contain `end();`, so each one loops forever and the harness blocks on the first case instead of failing it. `subprocess.run` is called without a timeout, so nothing breaks the wait. Two ways out: append `end();` to every test program — which does not save `empty.bp`, since a file with no statements can never reach an `end` — or add a flag that runs a single pass and point the harness at that. The flag is the smaller change and keeps each test asserting what it asserts today.

Three expectations are also stale beyond that: `equals_in_string.bp` records the `shout("a = b");` bug fixed in 0.5.0, and both `break` cases record the old off-by-one counts.

Expectations record what the interpreter *currently does*, including behavior known to be wrong (`parens.bp` and `unknown_variable.bp` are grouped and commented as such). Fixing one of those is meant to fail its test; update the expectation as part of the fix.
