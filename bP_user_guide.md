## Vocabulary
- "Interpreter"
The interpreter is the compiled code that runs your bP code line by line.

- "End user(s)"
A end user is any particular person who is not involed with the project however still using it.

- "Developer(s)"
A developer is any person who has contibuted to the bP project.

## Commands

- "g++ -Wall -std=c++17 main.cpp src/*.cpp -o bp"
Compiles main.cpp, whilst in the interpreter directory, to the name of the file as specified after -o, the guide assumes the end user left it as "bp". C++17 is required.


- "./bp filename.bp"
Runs interpreter with specified file.

The interpreter does not run your file once and stop. It reads the file, runs it, then reads it again, over and over, until your program calls `end();`. Two things follow from that. Editing the file while it is running changes what it does on the very next pass, so you can watch a change take effect without restarting. And a file with no `end();` in it runs forever — press Ctrl-C to stop it.

Every pass starts with no variables at all, so one pass behaves exactly like a fresh run.

If you save the file with a mistake in it, the interpreter prints the problem once and waits, re-reading, until you fix it. It does not exit and it does not repeat the message.

- "./bp filename.bp -v"
Runs interpreter with specified file and provides a detailed reading of the interpreter, for developers. Also accepts -verbose.

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

- use("otherfile.bp");
Runs another file and shares every variable with it: the other file can read variables you have set, and anything it sets is still there when it hands control back.

`use` is not a way of including a library. **It runs the other file over and over until that file calls its own `end();`.** That is how you write a loop:

```
main.bp                    second.bp
x = 0;                     x = x + 1;
use("second.bp");          shout(x);
end();                     break();
                           if (x == 5) {
                             end();
                           }
```

Running `./bp main.bp` prints 1, 2, 3, 4, 5 on separate lines and stops. `end();` inside second.bp ends only second.bp's repeat; main.bp carries on to its own `end();`.

Two things to watch. Anything you want counted has to live in the file that repeats — an increment in main.bp runs once and never changes again. And a file used without a reachable `end();` repeats forever.

A file cannot use itself, or use a file that uses it back. That prints `Cyclic use:` and the program carries on.

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
