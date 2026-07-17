using namespace std;
#include <map>
#include <string>
#include <vector>
#include <cctype>
#include <fstream>
#include <iostream>
#include "./include/has_extension.h"

void print_usage(const string& program_name) {
  cerr << "Usage:" << endl
    << program_name << " <filename>" << endl << endl
      << "Options:" << endl
        << "-v or -verbose: " << "Runs with detailed interpreter output.\n";
}

struct Variable {
  string type;    //  "string" or "int"
  bool is_static; //  Declared with a type in front; reassignments must keep that type.
  string value;
};

string trim(const string& text) {
  const size_t start = text.find_first_not_of(" \t\r\n");
  if (start == string::npos) return "";
  const size_t end = text.find_last_not_of(" \t\r\n");
  return text.substr(start, end - start + 1);
}

bool is_string_literal(const string& text) {
  return text.size() >= 2 && text.front() == '"' && text.back() == '"';
}

bool is_int_literal(const string& text) {
  size_t i = (!text.empty() && text[0] == '-') ? 1 : 0;
  if (i == text.size()) return false;
  for (; i < text.size(); i++)
    if (!isdigit(static_cast<unsigned char>(text[i]))) return false;
  return true;
}

string literal_value(const string& text) { //  The string literal without its quotes.
  return text.substr(1, text.size() - 2);
}

//  Splits "a, b, c" into {"a", "b", "c"}; commas inside quotes don't split.
vector<string> split_args(const string& text) {
  if (trim(text).empty()) return {};
  vector<string> args;
  string current;
  bool in_string = false;
  for (const char c : text) {
    if (c == '"') in_string = !in_string;
    if (!in_string && c == ',') {
      args.push_back(trim(current));
      current.clear();
    } else {
      current += c;
    }
  }
  args.push_back(trim(current));
  return args;
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
  int statement_executed = 0;

  string current;         //  Accumulates text until a ';' ends the statement.
  bool in_string = false; //  Inside a "..." literal, ';' and braces are ordinary characters.
  char c;

  while (file.get(c)) {
    if (c == '"') in_string = !in_string;
    if (in_string) {
      current += c;
      continue;
    }
    if (c == '{' || c == '}') { //  Block delimiters are ignored for now.
      current.clear();
      continue;
    }
    if (c != ';') {
      current += c;
      continue;
    }

    statement_count++;
    const string stmt = trim(current);
    current.clear();

    if (verbose) cout << endl << "Statement read: " << stmt << endl;

    const size_t eq = stmt.find('=');
    const size_t open = stmt.find('(');

    if (eq != string::npos && (open == string::npos || eq < open)) { //  '=' before any '(' means assignment.
      string name = trim(stmt.substr(0, eq));
      const string value = trim(stmt.substr(eq + 1));

      string decl_type; //  Stays empty for dynamic variables.
      const size_t space = name.find_first_of(" \t");
      if (space != string::npos) {
        decl_type = name.substr(0, space);
        name = trim(name.substr(space + 1));
        if (decl_type != "string" && decl_type != "int") {
          cerr << "Error: unknown data type: " << decl_type << "\n";
          return 1;
        }
      }
      if (name.empty() || name.find_first_of(" \t\"()") != string::npos) {
        cerr << "Error: invalid variable name: " << name << "\n";
        return 1;
      }

      string value_type;
      if (is_string_literal(value)) value_type = "string";
      else if (is_int_literal(value)) value_type = "int";
      else {
        cerr << "Error: expected a string or integer value, got: " << value << "\n";
        return 1;
      }

      if (!decl_type.empty() && decl_type != value_type) {
        cerr << "Error: " << name << " is declared " << decl_type
          << " but assigned " << value_type << "\n";
        return 1;
      }

      const auto existing = variables.find(name);
      const bool was_static = existing != variables.end() && existing->second.is_static;
      if (decl_type.empty() && was_static && existing->second.type != value_type) {
        cerr << "Error: " << name << " is static " << existing->second.type
          << " but assigned " << value_type << "\n";
        return 1;
      }

      variables[name] = {value_type, !decl_type.empty() || was_static,
        value_type == "string" ? literal_value(value) : value};
      statement_executed++;
      continue;
    }

    if (open == string::npos || stmt.empty() || stmt.back() != ')') {
      cerr << "Error: statement is not an instruction call: " << stmt << "\n";
      return 1;
    }

    const string instruction = trim(stmt.substr(0, open));
    const vector<string> args = split_args(stmt.substr(open + 1, stmt.size() - open - 2));

    if (instruction == "shout") {
      if (args.empty()) {
        cerr << "Error: shout expects at least one argument\n";
        return 1;
      }
      string text;
      for (const string& arg : args) {
        if (is_string_literal(arg)) {
          text += literal_value(arg);
        } else if (variables.count(arg)) {
          text += variables[arg].value;
        } else {
          cerr << "Error: shout expects string literals or declared variables, got: " << arg << "\n";
          return 1;
        }
      }
      cout << text;
    } else if (instruction == "break") {
      int count = 1;
      if (!args.empty()) {
        if (args.size() > 1 || !is_int_literal(args[0]) || stoi(args[0]) < 0) {
          cerr << "Error: break expects one non-negative integer\n";
          return 1;
        }
        count = stoi(args[0]);
      }
      for (int i = 0; i < count; i++) cout << "\n";
    } else {
      cerr << "Error: unknown instruction: " << instruction << "\n";
      return 1;
    }
    statement_executed++;
  }

  if (!trim(current).empty()) {
    cerr << "Error: unterminated statement: " << trim(current) << "\n";
    return 1;
  }

  if (verbose) {
    cout << endl << "Statements read: " << statement_count << endl
      << "Statements executed: " << statement_executed << endl;
  }
  return 0;
}
