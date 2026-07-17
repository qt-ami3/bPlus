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

## Latest: C refactor (0.2.2)

Changes:

- ./interpreter/main.c
    Rewrote the interpreter as a single pass; each statement is parsed and executed as soon as its ';' is read, instead of parsing the whole file first. Language features are unchanged from 0.2.1 (shout, break, dynamic and static variables, same error messages). shout and break now live as branches inside the statement loop; only has_extension remains as a separate file in ./interpreter/src/.
