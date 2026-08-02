using namespace std;
#include <map>
#include <set>
#include <string>
#include <vector>
#include <cctype>
#include <fstream>
#include <sstream>
#include <iostream>
#include "./include/trim.h"
#include "./include/split.h"
#include "./include/assign.h"
#include "./include/between.h"
#include "./include/variable.h"
#include "./include/eval_expr.h"
#include "./include/validate.h"
#include "./include/has_extension.h"
#include "./include/string_contains.h"
#include "./include/instruction_loop.h"

void print_usage(const string& program_name) {
  cerr << "Usage:" << endl
    << program_name << " <filename>" << endl << endl
      << "Options:" << endl
        << "-v or -verbose: " << "Runs with detailed interpreter output.\n";
}

int main(int argc, char* argv[]) {
  if (argc < 2 || argc > 3) {
    print_usage(argv[0]);
    return 1;
  }

  const string filename = argv[1];

  bool verbose = false;
  if (argc == 3) { //  argv[2] only exists when a flag was passed.
    const string argument = argv[2];
    if (argument != "-v" && argument != "-verbose") {
      print_usage(argv[0]);
      return 1;
    }
    verbose = true;
  }

  ifstream file(filename);
  if (!file) {
    cerr << "Could not open file: " << filename << "\n";
    return 1;
  }

  if (!has_extension(filename, ".bp")) {
    cerr << "Error: expected a .bp file\n";
    return 1;
  }

  int statement_count = 0;
  int semicolon_count = 0;

  vector<string> statements;
  vector<string> errors;
  vector<string> reported;  //  Errors already on screen, so a broken file isn't spammed.

  bool instruction_loop_break = false;
  while (!instruction_loop_break) {
    if (!validate_and_count(filename, statements, statement_count, semicolon_count, errors)) {
      if (errors != reported) {
        for (const string& error : errors) cerr << error << endl;
        reported = errors;
      }
      continue;  //  Hold here, re-reading, until the file is valid again.
    }
    reported.clear();

    map<string, Variable> variables;  //  Each pass starts from a clean state.
    set<string> active;               //  Files being run, so `use` can refuse a cycle.
    instruction_loop(instruction_loop_break, filename, statements, variables, verbose, active);
  }

  if (verbose) {
    cout << "Statements read;" << endl;
    for (const string& statement : statements) cout << statement << endl;

    cout << "Counted statements: " << statement_count << endl;
    cout << "Semicolons counted: " << semicolon_count;
  }

  cout << "\n";
}
