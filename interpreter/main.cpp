using namespace std;
#include <map>
#include <string>
#include <vector>
#include <cctype>
#include <fstream>
#include <sstream>
#include <iostream>
#include "./include/has_extension.h"
#include "./include/read_until.h"
#include "./include/between.h"
#include "./include/string_contains.h"
#include "./include/split.h"
#include "./include/variable.h"
#include "./include/assign.h"

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

  map<string, Variable> variables;
  int statement_count = 0;
  int semicolon_count = 0;

  string current;         //  Accumulates text until a ';' ends the statement.
  bool in_string = false; //  Inside a "..." literal, ';' and braces are ordinary characters.
  char c;

  while (file.get(c)) {
    if (c == '"') in_string = !in_string;
    if (in_string) {
      current += c;
      continue;
    }
    if (c != ';') {
      current += c;
      continue;
    }
    if (c == ';') {
      semicolon_count++;
    }

    statement_count++;
  }

  string delimiters = "();";

  for (int i = 0; i < semicolon_count; i++) {
    string statement = read_until(filename, ';', 1, i);
    vector<string> tokens = split_multi(statement, delimiters);

    if (string_contains(statement, "=")) {
      assign_variable(variables, statement);
    }

    if (string_contains(statement, "shout") && string_contains(statement, "\"")) {
      cout << between(statement, '"', '"');
    } else if (string_contains(statement, "shout")) {
      string name = between(statement, '(', ')');
      auto it = variables.find(name);
      if (it != variables.end()) {
        print_variable(it->second);
      }
    }

    if (string_contains(statement, "break")) {
      string arg = between(statement, '(', ')');
      if (arg.empty()) {
        cout << endl << endl;
      } else try {
        int count = stoi(arg);
        for (int i = 0; i <= count; i++) {
          cout << endl;
        }
      } catch (const invalid_argument &e) {
        auto it = variables.find(arg);
        if (it != variables.end()) {
          auto value = as_integer(it->second);
          if (value) {
            for (long long i = 0; i <= *value; i++) {
              cout << endl;
            }
          }
        }
      }
    }

    if (verbose) {
      for (size_t j = 0; j < tokens.size(); j++) {
        cout << tokens[j];
        if (j + 1 < tokens.size()) cout << " ";
      }
      cout << endl;
    }
  }
  
  if (verbose) {
    cout << "Statements read;" << endl;
    for (int i = 0; i < semicolon_count; i++) {
      cout << read_until(filename, ';', 1, i) << endl;
    }

    cout << "Counted statements: " << statement_count << endl;
    cout << "Semicolons counted: " << semicolon_count;
  }

  cout << "\n";
}
