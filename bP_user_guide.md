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

- break();
Ends the current line. `break()` prints 2 newlines; `break(N)` (a literal integer or a variable) prints `N + 1` newlines, so `break(1)` also prints 2 and `break(2)` prints 3.

### Known issue: shout is single-argument

`shout` no longer joins comma-separated arguments. In practice:

- `shout("hi ", "there");` prints only `hi ` — everything after the first `"..."` literal is dropped.
- `shout(name, other);` prints nothing at all.

Older examples in this guide and in `examples/dataTypes.bp` that call `shout` with more than one argument no longer work as written; call `shout` once per value instead.

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
