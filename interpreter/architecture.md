# Interpreter architecture

The interpreter is a C++17 program: `main.cpp` plus small helper modules under `include/`/`src/`. Statement execution is now two-pass: main first streams the whole file once just to count semicolons, then loops over that count and calls `read_until()` once per statement — which reopens the file and re-reads it from byte 0 every time. This replaces the single streamed pass the 0.2.2 C rewrite used; see "Pipeline" below for why that matters.

For per-file, per-function documentation see `codebase.md`.

## Directory layout

```
.. (root)
│
main.cpp          Interpreter core: reading, parsing, execution, CLI handling.
├─ include/       Headers, one per helper module.
└─ src/           Implementations of the headers in include/.
```

Modules (header + impl pair each): `has_extension`, `trim`, `string_contains`, `between`, `split`, `read_until`, `variable`, `parse_literal`, `assign`. `main.cpp` still owns the statement loop and the `shout`/`break` built-ins as inline branches; variable state and literal parsing were pulled out into `variable`/`parse_literal`/`assign`.

## Pipeline

1. **CLI handling** (`main`) — argument count, the `-v`/`-verbose` flag, the `.bp` extension, and that the file opens. Unchanged from 0.2.2.

2. **Count pass** — streams the file character by character, tracking `in_string` so a `;` inside a `"..."` literal isn't counted, purely to count semicolons. Nothing is parsed or executed here. `statement_count` and `semicolon_count` come out equal — a dead duplicate, both printed in verbose mode.

3. **Statement pass** — loops `i` from `0` to `semicolon_count`, calling `read_until(filename, ';', 1, i)` each iteration. `read_until` **reopens the file and scans from the start every call**, skipping `i` semicolons and then reading through the next one. So the file is re-read from byte 0 once per statement — O(n²) in file size × statement count, not the single streamed pass 0.2.2's architecture.md previously described.

4. **Classify and execute** (per statement) — main.cpp does not parse a statement into an instruction name plus an argument list. It runs independent `string_contains()` checks against the raw statement text, each an unguarded `if` (not `else if`), so more than one can fire on the same statement:
   - contains `"="` → `assign_variable()` runs.
   - contains `"shout"` and `"\""` → prints the text between the first two `"` characters in the statement (i.e. only the first quoted literal — see Known issues in `../change_log.md`).
   - contains `"shout"` without a `"` → resolves `between(statement, '(', ')')` as a variable name and prints it if found.
   - contains `"break"` → prints newlines (see `../change_log.md` for the exact, off-by-one count).

   Because the checks are substring-based and independent, `shout("a = b");` also contains `"="` and so also runs `assign_variable`, which fails to parse `b")` as a value and logs `Could not infer type for: b")` to stderr before the shout branch prints `a = b`.

5. With `-v`, each statement's `split_multi` tokens are echoed to stdout **after** the statement has already produced its own output — interleaved, not printed up front. After the whole loop, verbose mode re-reads every statement a second time via `read_until` to print a "Statements read;" dump, then both (equal) counts.

## Data structures

- `Variable` (`include/variable.h`) — `{ VarType type; bool is_static; VarValue value; }`.
  `VarType` is `Int | Long | Float | String | Bool`. `VarValue` is `std::variant<int, long long, double, std::string, bool>`. Variables live in a `map<string, Variable>` owned by main's loop.

## Language rules the code enforces

- **Variables**: `name = value;` infers a type from the literal (quoted → String, `true`/`false` → Bool, contains `.` → Float, else Int then Long) and declares dynamically. `type name = value;` (`int`, `long`, `float`/`double`, `string`, `bool`) declares statically; a value of the wrong type errors, at declaration and on every reassignment.
- **shout**: effectively single-argument in the current code — see `../bP_user_guide.md` and the "Regressions" entry in `../change_log.md` (0.3.0). Only the first quoted literal in the call is printed, or one variable resolved by name; comma-separated multi-argument calls do not work.
- **break**: `break()` prints 2 newlines; `break(N)` (literal or variable) prints `N + 1` newlines — `break(1)` → 2, `break(2)` → 3. `break(0)` is the odd one out at 1 newline, since the no-argument case is hardcoded separately from the counted loop.

## Conventions

- Helper modules stay small and pure (`has_extension`, `trim`, `string_contains`, `between`, `split`/`split_multi`, `read_until`, `variable`, `parse_literal`); `main.cpp` still holds the `shout`/`break` effects and all statement classification, inline in the statement loop.
- **Errors** go to stderr as an un-prefixed message (e.g. `Type error: ...`, `Unknown type: ...`, `Could not infer type for: ...`) and execution **continues** — unlike 0.2.2, there is no exit-on-first-error. The interpreter always exits 0, even after printing errors.
- Unknown instructions are silently ignored: anything that doesn't match the `=`/`shout`/`break` substring checks produces no output and no error.
- **Adding a built-in instruction**: add a `string_contains(statement, "...")` branch inside the statement loop in `main.cpp`. Remember the checks are independent `if`s, not `else if` — a new keyword that appears as a substring inside another branch's quoted text (or vice versa) will fire spuriously. Update `../bP_user_guide.md`.

## Compiling

See `compile.md`; there is no build system. `g++ -Wall -std=c++17 main.cpp src/*.cpp -o bp` builds everything. C++17 is required (`std::variant`, `std::optional`, `if constexpr`).

The old `main.c` (0.2.2) is not part of this build and no longer compiles on its own — it has an uncommitted, broken `pi` instruction stub. It is left in place but superseded by `main.cpp`.
