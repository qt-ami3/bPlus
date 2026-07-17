## Vocabulary
- "Interpreter"
The interpreter is the compiled code that runs your bP code line by line.

- "End user(s)"
A end user is any particular person who is not involed with the project however still using it.

- "Developer(s)"
A developer is any person who has contibuted to the bP project.

## Commands

- "g++ -Wall main.c src/*.c -o interpreter"
Compiles main.c, whilst in interpreter directory, to the name of the file as specified after -o, the guide assumes the end user left it as "interpreter"


- "./interpreter filename.bp"
Runs interpreter with specified file.

- "./interpreter filename.bp -v"
Runs interpreter with specified file and provides a detailed reading of the interpreter, for developers. Also accepts -verbose.

## Basic functionality

- shout();
Accepts string literals and variables, separated by commas, and prints them joined together to the shell.

```
shout("hi ", name, ", you are ", age, "!");
```

- break();
Ends the current line, accepts literal integer values for multiple line breaks.

## Variables

Variables are declared by assignment and are dynamically typed by default; putting a data type (string or int) in front makes the variable static.

```
phrase = "Hello world";
string greeting = "Hi";
int count = 3;
shout(phrase);
```

Values must be string literals ("...") or integer literals. A dynamic variable may later be reassigned to a different type; reassigning a static variable to a different type is an error.
