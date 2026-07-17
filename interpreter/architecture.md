# Interpreter architecture

The interpreter is a single C++ program that runs a `.bp` file in one pass: each statement is read up to its `;`, parsed, and executed immediately before the next one is read. Statements before a bad statement have therefore already run by the time the error prints — unlike the bPlus predecessor, which parsed the whole file before executing anything. That parse-first structure becomes worth having again once the language grows loops; this version trades it for a shorter, flatter program.

For per-file, per-function documentation see `codebase.md`.

## Directory layout

```
.. (root)
│
main.c            Interpreter core: reading, parsing, execution, CLI handling.
├─ include/       Small shared helpers (has_extension.h).
└─ src/           Implementations of the headers in include/.
```

`main.c` owns everything about the language: statement splitting, parsing, variables, and the built-in instructions (`shout`, `break`) as branches inside the statement loop.

## Pipeline

1. **CLI handling** (`main`)
   Validates arguments, the `-v`/`-verbose` flag, the `.bp` extension, and that the file opens.

2. **Read** (main's statement loop)
   Streams the file character by character, accumulating text until a `;` outside a string literal ends the statement. Inside a `"..."` literal, `;` `{` `}` `,` are ordinary characters. Newlines are just whitespace, so a statement may span lines. `{` and `}` clear the current chunk without producing a statement. Leftover non-whitespace text at end of file is an unterminated-statement error.

3. **Parse and execute** (same loop, immediately)
   The finished statement text is classified: an `=` appearing before any `(` means assignment; everything else must be an instruction call of the form `name(arg, ...)`. `split_args` splits the argument list on commas outside quotes. The statement then runs right away — assignments write the `variables` map, `shout` prints its resolved arguments, `break` prints newlines. With `-v`, each statement text is printed as it is read.

## Data structures

- `Variable` — current value, its type (`"string"` or `"int"`, inferred from the last assigned value), and whether it is static. Variables live in a `map<string, Variable>` owned by main's loop.

## Language rules the code enforces

- **Variables**: `name = value;` declares dynamically (type may change on reassignment). A data type in front (`string name = ...;` / `int name = ...;`) declares statically; a value of the wrong type errors at declaration and on every reassignment. Values must be string or integer literals.
- **shout**: any number of comma-separated string literals and variables, printed joined together.
- **break**: no argument or one non-negative integer literal.

## Conventions

- Helpers stay small and pure (`trim`, the literal checks, `split_args`); decisions and effects happen inline in main's loop, readable top to bottom.
- **Errors** go to stderr as one line prefixed `Error: `, and the interpreter exits with status 1 at the first failure.
- **Adding a built-in instruction**: add an `else if (instruction == "...")` branch to the statement loop, validate the arguments there, and perform the effect. Update `bP_user_guide.md` in the repo root.

## Compiling

See `compile.md`; there is no build system, `g++ -Wall main.c src/*.c -o interpreter` builds everything.
