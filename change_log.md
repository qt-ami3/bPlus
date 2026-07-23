# Start of history: tail change_log.md for 
For petitioning to change the change log style, please raise a Github issue.

## Outlining change log guidelines (0.0.0)
Versioning:

- Major.Minor.Patch *Version will not be adjusted until hello world works where it will then be (0.1.0)*
    
    A major release should add something new to the language or standard library, a minor release; another way to utilize an already existing feature. When a new release is about to be commited, the author should apend they're changes using the same style as the rest of the document and move the "Latest:" forward for parsing.

- Git commits

    A git commit should describe the goal or feature added in git commit in no more than one line, if the full commit command can fit in no more than two lines of a 760*540p(1080p/2) terminal window then you are doing too much, the place for detailed changes is this file.

## Hello World (0.1.0)
Added:

- shout

    Short for shell out; it is a the bP equivalent of C++'s cout, it takes string literal and whatever data types you would normally expect a basic shell out function to accept, as such it uses cout in the interpreter code.

    ./interpreter/src/shout.cpp

```
bool shout(const std::string& line) {
  std::cout << line;
  return true;
}
```

- break

    break, but with an argument for how many breaks you would like.

```
break()  //  One break
break(2) //  Two breaks
```

## Added LICENSE

Added GNU General Public License v3.0

## Data types (0.2.1)

Added:

- Two new data types
    Currently, the two data types are int and string. Variables are dynamically typed by default, however if you add a data type before the name, the variable is static.

```
name = "carl";
string carlIsAwesome = " is awesome";

shout(name, carlIsAwesome);
```

- ./interpreter/codebase.md

- ./examples directory
    Contains helloWorld program and newly added dataTypes program.

Changes:

- ./interpreter/main.cpp
    Added logic for Variables.
    Added compatibility for multiple arguments for shout.

- README.md
    Removed Hello World section. Seemed redundant with the addition of an examples directory.

- User Manual
    Updated to include new features.

Fixes:

- shout
    Shout would still run without closing parenthesis if a semicolon was placed causing it to print the rest of the source file until it ran into another parenthesis.

## C refactor (0.2.2)

Changes:

- ./interpreter/main.c
    Rewrote the interpreter as a single pass; each statement is parsed and executed as soon as its ';' is read, instead of parsing the whole file first. Language features are unchanged from 0.2.1 (shout, break, dynamic and static variables, same error messages). shout and break now live as branches inside the statement loop; only has_extension remains as a separate file in ./interpreter/src/.

## Latest: C++ modular rewrite, new data types (0.3.0)

"Another refactor?"

Yes, and the reason why is because this is still a learning experience for me, there is, on my pc, a local git repo with unstaged changes where I rewrite the current release in order to understand my code better and see how it can be improved. With this rewrite (hopefully the final one for now) I aim to abstract the lower level C++ code behind a more still but slightly less functional, Go-esque, include directory.

"That seems like alot of effort."

Yes however; I think I have landed on a codebase methodology that I can actually fully understand the logic behind so that I can be accountable for the changes I make. Variables are also now parsed in the interpreter as actual C++ variables so I can easily add features without spending forever on ultra-low level C++ black magic.

Added:

- Three new data types
    long, float (the keyword `double` is also accepted and stored the same way), and bool (`true`/`false`). Together with the existing int and string that's five types. Type inference order for an unlabelled assignment: quoted text is string, `true`/`false` is bool, a literal containing `.` is float, otherwise int then long.

Changes:

- ./interpreter/main.cpp, ./interpreter/src/*.cpp, ./interpreter/include/*.h
    Rewrote the interpreter in C++17, split into main.cpp plus 9 helper modules (has_extension, trim, string_contains, between, split, read_until, variable, parse_literal, assign). Variable state and literal parsing moved out of main into variable.h/parse_literal.h; assignment moved into assign.h. Compiles with `g++ -Wall -std=c++17 main.cpp src/*.cpp -o bp`; ./interpreter/main.c is no longer part of the build.
- Errors
    No longer prefixed `Error: ` and no longer fatal. Each failure (`Type error: ...`, `Unknown type: ...`, `Invalid variable declaration: ...`, `Could not infer type for: ...`) prints its own message to stderr and execution continues with the next statement; the interpreter now always exits 0.
- Statement reading
    Reworked into two passes: main first streams the file once to count statements, then re-reads the whole file from the start once per statement via read_until. This replaces 0.2.2's single streamed pass.
- ./interpreter/architecture.md, ./interpreter/codebase.md, ./interpreter/compile.md, ./bP_user_guide.md
    Rewritten to document the C++ rewrite, the new types, and the regressions below.

Known issues / Regressions:

- shout is single-argument
    shout no longer joins comma-separated arguments; the comma-splitting from 0.2.1 (`split_args`) was not carried over. `shout("hi ", "there");` prints only `hi `; `shout(name, other);` prints nothing. This breaks ./examples/dataTypes.bp and the multi-argument shout example previously shown in bP_user_guide.md as written.
- break is off by one
    `break()` prints 2 newlines, but `break(N)` prints `N + 1` newlines (`break(1)` → 2, `break(2)` → 3), and `break(0)` prints only 1 — the no-argument case is a separate hardcoded branch, not `N = 0` through the same loop.
- Unknown instructions are silently ignored
    A call that isn't `shout`/`break`/an assignment (e.g. `frobnicate("x");`) does nothing and produces no error, where 0.2.2 raised an unknown-instruction error.
- Statement classification is substring-based, not structural
    main.cpp decides what a statement does by running string_contains(statement, "=" / "shout" / "break") on the raw statement text as independent ifs, not by parsing an instruction name. A statement can trigger more than one branch: `shout("a = b");` also matches the `=` check and logs `Could not infer type for: b")` to stderr before printing `a = b`.
- `{` / `}` block-clearing is gone
    0.2.2 treated `{`/`}` as chunk-clearing characters; the new statement reader has no equivalent, and there is no longer an unterminated-statement-at-EOF error.
