import subprocess

# ./bp here is a copy of ../bp, so re-copy it after rebuilding the interpreter
# or these tests will pass against a stale binary.
BP = "./bp"

USAGE = ("Usage:\n"
         "./bp <filename>\n"
         "\n"
         "Options:\n"
         "-v or -verbose: Runs with detailed interpreter output.")

VERBOSE_HELLO = ('Hello World!shout "Hello World!"\n'
                 "Statements read;\n"
                 'shout("Hello World!");\n'
                 "Counted statements: 1\n"
                 "Semicolons counted: 1")

def run_command_and_compare(command, expected_output, expected_error, expected_status):
    returnValue = 1
    try:
        # Run the command and capture output as a string
        result = subprocess.run (
            command,                # Command as a list for safety
            capture_output=True,    # Capture stdout and stderr
            text=True               # Decode bytes to string
        )                           # No check=True; a failing exit code is itself a test case.

        # Strip trailing newlines/spaces for clean comparison
        output = result.stdout.strip()
        error = result.stderr.strip()

        # Compare with expected strings
        if output == expected_output and error == expected_error and result.returncode == expected_status:
            returnValue = 0
        else:
            returnValue = 1
            if output != expected_output:
                print(f"  stdout expected {expected_output!r}, got {output!r}")
            if error != expected_error:
                print(f"  stderr expected {expected_error!r}, got {error!r}")
            if result.returncode != expected_status:
                print(f"  exit code expected {expected_status}, got {result.returncode}")

    except FileNotFoundError:
        print("Command not found. Please check the command name.")
    except Exception as e:
        print(f"Unexpected error: {e}")

    return returnValue

tests = []

def case(arguments, expected_output, expected_error = "", expected_status = 0):
    tests.append((arguments, expected_output, expected_error, expected_status))

#  Language features.
case(["hello.bp"], "Hello World!")
case(["addition.bp"], "3 3")
case(["arithmetic.bp"], "14 3 42")
case(["division.bp"], "4 3.5")
case(["floats.bp"], "3.75 3")
case(["negatives.bp"], "-5 -7")
case(["booleans.bp"], "true false")
case(["types.bp"], "5 9000000000 2.5 text false")
case(["reassign.bp"], "12now a string")
case(["breaks.bp"], "a\n\nb\n\n\nc")
case(["break_variable.bp"], "a\n\nb")
case(["name_contains_keyword.bp"], "3 4")
case(["empty.bp"], "")

#  Errors. These are non-fatal, so the interpreter keeps going and exits 0.
case(["unknown_function.bp"], "ab", "Unknown function: nope")
case(["not_a_statement.bp"], "", "Not a statement: justtext;")
case(["unknown_type.bp"], "", "Unknown type: frob")
case(["type_error.bp"], "1", "Type error: n is int, cannot assign '\"oops\"'")
case(["unknown_variable_expression.bp"], "", "Unknown variable in expression: q")
case(["division_by_zero.bp"], "", "Division by zero")
case(["string_in_expression.bp"], "", "Cannot use string in expression")

#  Semicolon checking. These are fatal and run before any statement executes.
case(["missing_semicolon.bp"], "", "Missing ';' for statement line 1: shout(\"a\")*;*", 1)
case(["extra_semicolon.bp"], "", "Extra ';' for statement line 1: shout(\"a\");*;*", 1)

#  Command line handling.
case([], "", USAGE, 1)
case(["nosuchfile.bp"], "", "Could not open file: nosuchfile.bp", 1)
case(["main.py"], "", "Error: expected a .bp file", 1)
case(["hello.bp", "-x"], "", USAGE, 1)
case(["hello.bp", "-v"], VERBOSE_HELLO)
case(["hello.bp", "-verbose"], VERBOSE_HELLO)

#  Known limitations, locked in so a fix shows up here as a failure.
case(["parens.bp"], "")            #  between() stops at the inner ')', so (2 + 3) * 4 prints nothing.
case(["unknown_variable.bp"], "")  #  shout(q) on an undefined name is silent, no error.
case(["equals_in_string.bp"], "", 'Could not infer type for: b")')  #  An '=' inside a string reads as an assignment.

tests_passed = 0
all_tests_pass_message = "All tests passed ✔"

for arguments, expected_output, expected_error, expected_status in tests:
    label = " ".join(arguments) if arguments else "(no arguments)"
    if run_command_and_compare([BP] + arguments, expected_output, expected_error, expected_status) == 0:
        print(f"{label} passed ✓")
        tests_passed += 1
    else:
        print(f"Unexpected output from {label}")

print(f"\n{tests_passed}/{len(tests)} tests passed")
if tests_passed == len(tests):
    print(all_tests_pass_message)
