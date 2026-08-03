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

## C++ modular rewrite, new data types (0.3.0)

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

## Proper semicolon error checking (0.3.1)

## Updated style.md (0.3.2)

I am very sickly right now so I dont have the energy to make big additions, so intead I am working on slimming down the verbosity of the codebase by updating the style guide.

Changes:

- All applicable if statements are now one-liners.

- All applicable for loops drop the braces.

## Expression evaluation! (0.4.2)

Today I added expression evaluation so you can actually do things with variables now other than print them. I plan on adding logic statements next so you can dynamic programs so, gah whatever.

As far as the future of this project and if it will aim to do anything unique or helpful; I had a dream the other night where b+ could be interpreted to almost-production ready c++ code which I think would be a good legitimate use case for the project so once I get the bones finished thats where I will take the interpreter next.

Added:

include/eval_expr.h
src/eval_expr.cpp

changes:

- main.cpp
    Added logic for expression evaluation.

- bP_user_guide.md
    Added instruction on mathamatical evaluation.

## Instruction name dispatch (0.4.3)

No new language features here. Before this, main.cpp asked "does this line contain the text `shout`?" and "does it contain the text `break`?" for every single statement, as separate ifs, which meant every builtin got checked on every line whether or not it was being called, and a variable whose name merely contained `break` would run break. Now the instruction name is parsed out once, up front, and only the matching builtin is looked at. Adding a builtin no longer adds a scan to every line of every program.

Added:
- ./interpreter/unit_tests
    a python script and a host of sample .bp folders for testing the parsing of the interpreter.

Changes:

- main.cpp
    Statement execution now parses `name(arg)` structurally. The statement is trimmed, split at the first `(` into an instruction name and an argument, and the name is matched against `shout` and `break` in a single if/else chain. Replaces the four independent string_contains scans that ran per statement. The quoted-string check for shout now looks at the argument instead of the whole statement.

Fixes:

- Unknown instructions are reported again
    A call that isn't `shout`, `break`, or an assignment now prints `Unknown function: <name>` to stderr, restoring the unknown-instruction error lost in 0.3.0. A statement with no `(` at all reports `Not a statement: <statement>`. Both are non-fatal, in keeping with 0.3.0's error handling.
- Statements no longer trigger more than one builtin
    `breaker = 3;` was matching the `break` check and printing two blank lines. `x = "shout(5)";` was matching the `shout` check and printing `shout(5)` on assignment. Both now take the assignment branch only.

Known issues:

- Assignment detection is still substring-based
    The choice between "assignment" and "call" is still `string_contains(statement, "=")` on the raw text, so a statement carrying an `=` inside a string literal is treated as an assignment. `shout("a = b");` still misparses. The instruction-name half of 0.3.0's substring-classification issue is fixed; this half is not.
- Verbose output
    Statements are trimmed before tokenising, so `-v` token lines no longer carry the leading newline that read_until returns. Cosmetic, but the output differs from 0.4.2.
- Statement reading is unchanged
    read_until still reopens the source file and re-reads it from the start once per statement, so execution is still quadratic in file length. The dispatch cost was never the bottleneck here; this is.

## Control flow, files that repeat, and a program that never stops reading itself (0.5.0)

This is the release where bP stops being a list of instructions and starts being able to decide things. `if` blocks are in, with real conditions. The interpreter also re-reads its source file continuously now, so editing a running program changes what it does without restarting it, and `use` pulls in another file while sharing every variable with it.

The one thing I want to write down for later me: `use` is not an include. It repeats the file until that file ends itself, which is why the counter example works. A helper file with no `end();` in it will loop forever. I picked that deliberately but it will catch me out at some point.

Added:

- if
    Braced blocks with a condition. Conditions take `==`, `!=`, `<`, `>`, `<=` and `>=`, or a lone value that is true when it isn't zero, isn't false, and isn't an empty string. Either side can be an expression or a variable, and two strings compare as text.

```
if (x > 5) {
  shout("big");
}
if (name == "carl") {
  shout("hi carl");
}
```

- end
    Stops the file it appears in. In the file you ran, that also stops the interpreter re-reading it, so `end();` is how a program finishes. In a file pulled in by `use`, it only ends that file's repeat.

- use
    Runs another file, repeatedly, until that file's own `end` fires, sharing all variable data both ways. A file cannot use itself, directly or through a chain; that reports `Cyclic use:` and carries on. The used file is re-read every time round the repeat, so editing it changes the loop while it runs.

```
main.bp                    second.bp
x = 0;                     x = x + 1;
use("second.bp");          shout(x);
end();                     break();
                           if (x == 5) {
                             end();
                           }
```

- Continuous re-reading
    The interpreter now loops: read the file, check it, run it, repeat, until something calls `end`. Editing a running program takes effect on the next pass, including statements you add or delete. Variables start empty on every pass, so a pass behaves exactly like a fresh run.

- include/validate.h, src/validate.cpp
- include/instruction_loop.h, src/instruction_loop.cpp

Changes:

- break counts what you tell it
    `break()` and `break(1)` both print one newline, `break(2)` prints two, `break(0)` prints none. It used to print one more than asked, with no argument meaning two. Any program relying on the old count now prints one fewer blank line.

- Statements are read once per pass, not once each
    The file is read into a list of statements up front instead of reopening it for every statement through read_until. Execution is no longer quadratic in file length. read_until is now unused by anything, and left in place.

- Assignment is detected properly
    Deciding whether a statement assigns is now a scan for a lone `=` outside quotes, skipping `==`, `!=`, `<=` and `>=`. Without it every condition would have been read as an assignment.

- main.cpp
    Down to argument handling and the re-read loop. The statement loop moved to instruction_loop, the file checking to validate.

Fixes:

- `shout("a = b");`
    Prints `a = b`. It used to also run as an assignment and log `Could not infer type for: b")` to stderr first. Listed as a known issue since 0.3.0.

- Statements no longer trigger the wrong builtin, and a cached file no longer traps the interpreter
    A `use`d file was checked once and then repeated from that copy, so edits to it never landed. Worse, editors and shell redirection empty a file before writing it; a pass that read it in that instant saw no statements, never reached an `end`, and span forever in silence with no way back. It is re-read every time round now.

Known issues:

- A file pulled in by `use` must be able to reach an `end`
    No `end`, no way out. This includes a file that cannot be read: `use("typo.bp")` reports `Could not open file:` once and then holds, waiting for the file to appear, rather than carrying on.

- Braces need a line of their own
    `if (x > 5) { shout("a"); }` is rejected with `Braces belong on a line of their own`. The opening brace ends its line, the closing brace has a line to itself.

- No else
    Write the opposite condition as a second `if`.

- Conditions cannot contain brackets
    `if ((a + b) > 5)` reads the condition as `(a + b` and reports `Bad condition:`, because the argument is still taken as the text between the first `(` and the first `)`. It fails loudly rather than quietly.

- `end` is also what stops the re-reading
    A program that ends cannot be edited while it runs, because it has already exited. Live editing and finishing on your own terms are the same switch.

- shout still prints nothing for a bare literal
    `shout(5);` and `shout(true);` print nothing; `shout(5 + 0);` prints 5. Expression evaluation only engages when there is an operator, and shout has no fallback to plain literal parsing after that.

## Arrays, and libraries written in C++ (0.6.0)

Arrays are in, which is the first time bP can hold more than one thing under one name. They work the way you would expect from C: `arr[3];` to make one, `arr[0]` to reach a slot, and the number in the brackets can be worked out rather than written down, so `nums[i + 1]` does what it looks like.

The other half of this release is that libraries can now be written in C++ and sit outside the core interpreter, in src/libraries. `use "shell_utilities"` declares one and unlocks its instructions. This is the first step towards the standard library being something I add to rather than something baked into the statement loop.

One rename to be aware of before anything else: the instruction that runs another bP file is now **pass**, not use. `use` means the C++ library declaration. Any program written against 0.5.0 needs `use("file.bp")` changed to `pass("file.bp")`.

Added:

- Arrays
    Declared with a length, dynamic or with a type in front, and optionally filled on the spot. Every slot starts at a zero of its type, so a new array can be read straight away. Slots count from 0.

```
arr[3];
int scores[3];
nums[3] = {1,2,3};
string words[2] = {"hi","there"};

arr[0] = 7;
shout(arr[0]);
```

    The index can be a variable or a sum, and a slot works anywhere a variable does, including in expressions and conditions.

```
i = 2;
shout(nums[i]);
shout(nums[i + 1]);
x = nums[0] + nums[1];
if (nums[3] > 35) {
  shout("high");
}
```

    Typing follows ordinary variables: `arr[3];` takes anything per slot, `int scores[3];` refuses anything that is not an int. Reaching past the end reports `Index out of range:` and carries on. A slot of an array that was never declared reports `Unknown array:`.

- use "library"
    Declares a library compiled into the interpreter from src/libraries, making its instructions callable. No brackets and no semicolon, which is a shape nothing else in the language has.

```
use "shell_utilities"
clear();
end();
```

    Without the declaration the instruction refuses to run and says what is missing: `clear needs: use "shell_utilities"`. An unknown name reports `Unknown library:`. The declaration counts only for the file it is written in, and has to come before the instructions it enables.

- clear
    Empties the screen. Comes from shell_utilities.

- src/libraries and include/libraries
    Where a library lives. Dropping a .cpp and a header in needs no build change.

- Makefile
    `make` builds, `make clean` removes the binary. The point of it is that neither `-std=c++17` nor the libraries directory can be left off by accident, which had already happened twice by hand.

- Library presence check
    Before a file runs, every file named in a `pass` is looked for on disk and a true/false variable named after it records whether it is there. `pass("helper.bp")` sets `helper`, so a program can check before committing to a file that would otherwise make it wait forever.

```
if (helper) {
  pass("helper.bp");
}
```

- Memory readout
    With -v, the interpreter's own memory use is printed at the start and end of every pass.

- include/array.h, src/array.cpp
- include/validate.h, src/validate.cpp
- include/process_ram_kb.h, src/process_ram_kb.cpp
- include/libraries/shell_utilities.h, src/libraries/shell_utilities.cpp

Changes:

- use is now pass
    The instruction that runs another bP file repeatedly was renamed to make room for the library declaration. Behaviour is unchanged. Its cycle error still prints `Cyclic use:`, which was missed in the rename.

- Braces mean a block only on a line without a semicolon
    Needed so `arr[3] = {1,2,3};` reads as data. A line ending in `{` is still a block, a lone `}` still closes one, and `if (x) { shout("a"); }` on one line is still refused with `Braces belong on a line of their own`.

- eval_expr understands brackets
    `[` and `]` are tokens now, an indexed name resolves to a slot, and the evaluator engages on a bracket as well as on an operator — without that, `arr[i]` with a variable index could not be resolved by name. A lone slot is answered straight from storage rather than through the arithmetic, so a string slot is not rejected for not being a number.

- Verbose output is readable
    Each statement is numbered and printed before it runs rather than after, so what it prints appears underneath its own line instead of running into it. Blocks are traced too, and a skipped block shows as a gap in the numbering.

- bP_user_guide.md, interpreter/compile.md, interpreter/architecture.md
    Updated for all of the above.

Fixes:

- Statements and their output no longer collide under -v
    shout writes no trailing newline, so the old trace printed `17993shout x`. Each trace line now starts on a fresh line.

- src/libraries/shell_utilities.cpp did not compile
    Missing `<unistd.h>`, `<iostream>` and `<cstdlib>`.

- src/process_ram_kb.cpp called sscanf without `<cstdio>`
    It compiled only because another header happened to pull it in.

Known issues:

- Arrays cannot be printed whole
    `shout(arr);` prints nothing. Print the slots one at a time. There is no length instruction either, though the length is stored.

- A static array will not take a sum in a slot
    `int t[2]; t[0] = 1 + 1;` is a type error, the same restriction static variables have had since 0.4.2. Dynamic arrays take them fine.

- Declaring an array again empties it
    The length is fixed at declaration, and re-declaring resets every slot.

- A library declaration does not cross a pass
    A file reached by `pass` has to declare `use "library"` again for itself.

- Cyclic use:
    The cycle message still names an instruction that no longer exists.

## Latest: exec() function implemented into shell_utilities library 0.7.0
