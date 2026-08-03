## Vocabulary
- "Interpreter"
The interpreter is the compiled code that runs your bP code line by line.

- "End user(s)"
A end user is any particular person who is not involed with the project however still using it.

- "Developer(s)"
A developer is any person who has contibuted to the bP project.

## Commands

- "make"
Builds the interpreter, whilst in the interpreter directory, to a program called "bp", which the rest of this guide assumes you kept. `make clean` removes it.

- "g++ -Wall -std=c++17 main.cpp src/*.cpp src/libraries/*.cpp -o bp"
The same thing by hand, if you would rather. C++17 is required, and the libraries directory has to be listed. `make` is the safer habit because neither can be left off by accident.


- "./bp filename.bp"
Runs interpreter with specified file.

The interpreter does not run your file once and stop. It reads the file, runs it, then reads it again, over and over, until your program calls `end();`. Two things follow from that. Editing the file while it is running changes what it does on the very next pass, so you can watch a change take effect without restarting. And a file with no `end();` in it runs forever — press Ctrl-C to stop it.

Every pass starts with no variables at all, so one pass behaves exactly like a fresh run.

If you save the file with a mistake in it, the interpreter prints the problem once and waits, re-reading, until you fix it. It does not exit and it does not repeat the message.

- "./bp filename.bp -v"
Runs interpreter with specified file and provides a detailed reading of the interpreter, for developers. Also accepts -verbose.

Each statement is numbered and printed just before it runs, so anything it prints appears underneath its own line. Blocks are numbered too, and a block that was skipped shows up as a gap in the numbering:

```
[1] x = 17992
[2] break 2
[3] shout x
17993
[4] if x > 5 {
[5] shout "big"
big
[6] }
```

Memory use is reported at the start and end of each pass, and the libraries found by the `pass` check are listed before anything runs.

## Basic functionality

- shout();
Prints a single argument to the shell: a string literal, or a variable's value.

```
shout("hi ");
shout(name);
```

Only one argument is supported. `shout("hi ", name, "!");` does **not** print all three parts — see "Known issue: shout is single-argument" below.

`shout` prints a string literal, a variable, or an expression. A bare number on its own prints nothing: `shout(5);` and `shout(true);` produce no output, while `shout(5 + 0);` prints `5`. Put the value in a variable, or give it an operator.

- break();
Ends the current line. `break()` and `break(1)` print one newline, `break(2)` prints two, `break(0)` prints none. The argument may be a literal integer or a variable.

- end();
Ends the file it is written in. In the file you ran, this is what stops the interpreter, so most programs finish with `end();` on the last line.

```
shout("done");
end();
```

- pass("otherfile.bp");
Runs another bP file and shares every variable with it: the other file can read variables you have set, and anything it sets is still there when it hands control back.

`pass` is not a way of including a library. **It runs the other file over and over until that file calls its own `end();`.** That is how you write a loop:

```
main.bp                    second.bp
x = 0;                     x = x + 1;
pass("second.bp");         shout(x);
end();                     break();
                           if (x == 5) {
                             end();
                           }
```

Running `./bp main.bp` prints 1, 2, 3, 4, 5 on separate lines and stops. `end();` inside second.bp ends only second.bp's repeat; main.bp carries on to its own `end();`.

Two things to watch. Anything you want counted has to live in the file that repeats — an increment in main.bp runs once and never changes again. And a file passed to without a reachable `end();` repeats forever.

The file name has to be written out in quotes. A name held in a variable does not work: `name = "second.bp"; pass(name);` cannot find the file and waits forever for it.

A file cannot pass to itself, or to a file that passes back to it. That prints `Cyclic use:` and the program carries on. (The message still says "use" — it was not renamed with the instruction.)

Before your program starts, every file named in a `pass` is checked, and a `true`/`false` variable named after it records whether it is really there. `pass("helper.bp")` sets `helper`. That lets you look before you leap:

```
if (helper) {
  pass("helper.bp");
}
```

Without that guard, passing to a file that does not exist waits forever for it to appear. The variable takes the file's name without the folder or the `.bp`, so `pass("lib/maths.bp")` sets `maths`. Do not use that name for anything else — it is overwritten every time round.

- use "library"
Declares a library that is built into the interpreter, making its instructions available to the file. Note the shape: no brackets, and **no semicolon**.

```
use "shell_utilites"
clear();
end();
```

`clear();` empties the screen. It comes from the `shell_utilites` library, so without the `use` line it refuses to run and tells you what is missing:

```
clear needs: use "shell_utilites"
```

A name that is not a library reports `Unknown library:` and the program carries on.

Two rules. The `use` line has to come before the instructions it enables, and it only counts for the file it is written in — a file reached by `pass` has to declare the library again for itself.

These libraries are written in C++ and live in `interpreter/src/libraries/`, compiled into `bp`. `use` does not load anything while your program runs; it decides what your program is allowed to call.

### Known issue: shout is single-argument

`shout` no longer joins comma-separated arguments. In practice:

- `shout("hi ", "there");` prints only `hi ` — everything after the first `"..."` literal is dropped.
- `shout(name, other);` prints nothing at all.

Older examples in this guide and in `examples/dataTypes.bp` that call `shout` with more than one argument no longer work as written; call `shout` once per value instead.

## Making decisions

- if

```
if (x > 5) {
  shout("x is big");
  break();
}
```

The block runs only when the condition holds. Conditions accept `==`, `!=`, `<`, `>`, `<=` and `>=`. Either side can be a number, a variable, or a whole expression:

```
if (count == 0) {
  shout("empty");
}
if (x + 1 < y * 2) {
  shout("still room");
}
```

Two strings compare as text, so `if (name == "carl")` and `if (name != "bob")` both work. Comparing text against a number is not meaningful and reports `Bad condition:`.

A condition with no operator is true when the value is not zero, not `false`, and not an empty string:

```
bool ready = true;
if (ready) {
  shout("go");
}
```

`if` blocks nest, and a block that does not run is skipped whole, including any blocks inside it.

**Braces need a line of their own.** The opening brace ends the `if` line, and the closing brace sits alone:

```
if (x > 5) {        correct
  shout("a");
}

if (x > 5) { shout("a"); }        rejected
```

Writing it on one line reports `Braces belong on a line of their own`. Leaving a block unclosed reports `Unclosed '{'`, and a stray closing brace reports `Unmatched '}'`.

There is no `else` yet. Write the opposite condition as a second `if`.

Conditions cannot contain brackets of their own: `if ((a + b) > 5)` reports `Bad condition:`, because the condition is read as the text between the first `(` and the first `)`. Work it out into a variable first.

## Variables

Variables are declared by assignment and are dynamically typed by default; putting a data type in front makes the variable static. Supported types: `int`, `long`, `float` (or `double`, stored the same way), `string`, `bool`.

```
phrase = "Hello world";
string greeting = "Hi";
int count = 3;
long big = 123456789012;
float pi = 3.14;
bool done = true;
shout(phrase);
```

Values must be string literals (`"..."`), integer literals, floating-point literals (containing `.`), or `true`/`false`. When no type is given, the type is inferred from the value: quoted text is `string`, `true`/`false` is `bool`, a literal containing `.` is `float`, otherwise it's parsed as `int` then `long`. A dynamic variable may later be reassigned to a different type; reassigning a static variable to a different type is an error.

Variables can be worked with using standard mathamatical notation:
  
  pemdas.bp (target)                             33 — shout(1+2)→3, shout(z)→3, z=x+y ✓

  Precedence 2+3*4                               14 ✓

  Parens in assignment (2+3)*4, nested           20, 12 ✓

  Real division 7/2, 6/2                         3.5, 3 ✓

  Float propagation 1.5+1                        2.5 ✓
  
  Divide by zero                                 Division by zero → stderr, continues, exit 0 ✓

  Unknown variable                               Unknown variable in expression: a → stderr, continues, exit 0
  
  String with - ("a-b")                          not evaluated, prints a-b ✓ (tokenizer rejects the quote)

## Arrays

An array holds several values under one name, reached by a number in square brackets. Declare it with the number of slots you want:

```
arr[3];
int scores[3];
```

Every slot starts at zero of its type, so a freshly declared array can be read straight away: `arr[3];` then `shout(arr[0]);` prints `0`. A `string` array starts with empty text, a `bool` array with `false`.

Give the values up front if you have them:

```
nums[3] = {1,2,3};
int typed[2] = {10,20};
string words[2] = {"hi","there"};
```

Slots are counted from 0, so `nums[3] = {1,2,3};` fills `nums[0]`, `nums[1]` and `nums[2]`. There is no `nums[3]`.

Read and write a slot the same way you would a variable:

```
arr[0] = 7;
shout(arr[0]);
```

The number in the brackets can be worked out rather than written down, which is what makes arrays worth having:

```
nums[4] = {10,20,30,40};
i = 2;
shout(nums[i]);          prints 30
shout(nums[i + 1]);      prints 40
nums[i] = nums[0] * 2;   sets nums[2] to 20
```

Slots work anywhere a variable does, including inside sums and conditions:

```
x = nums[0] + nums[1];
if (nums[3] > 35) {
  shout("high");
}
```

Typing follows the same rule as ordinary variables. `arr[3];` is dynamic and each slot takes whatever you put in it. `int scores[3];` fixes the type, and putting anything else in a slot is an error:

```
int t[2];
t[0] = "text";      Type error: cannot assign '"text"' to int t
```

Things to watch:

Reaching past the end reports `Index out of range:` and carries on, both writing and reading. `small[2];` has slots 0 and 1 only.

The length is fixed when you declare it, and declaring the same array again empties every slot.

Using a slot of an array you never declared reports `Unknown array:`.

There is no way to print a whole array at once. `shout(arr);` prints nothing — print the slots one at a time.

A static array will not take a sum in a slot: `int t[2]; t[0] = 1 + 1;` is a type error, the same restriction static variables already have. A dynamic array takes them fine.
