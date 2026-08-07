using namespace std;
#include <map>
#include <set>
#include <cstdlib>
#include <cctype>
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <type_traits>
#include "../include/trim.h"
#include "../include/has_extension.h"
#include "../include/split.h"
#include "../include/array.h"
#include "../include/assign.h"
#include "../include/between.h"
#include "../include/variable.h"
#include "../include/validate.h"
#include "../include/eval_expr.h"
#include "../include/parse_literal.h"
#include "../include/process_ram_kb.h"
#include "../include/string_contains.h"
#include "../include/instruction_loop.h"
#include "../include/libraries/random.h"
#include "../include/libraries/shell_utilities.h"

//  Where a `for` body is written before being run, when the loop was not given
//  a name of its own. Left visible: it is a real file you can open and edit
//  while the loop runs, which is hard to discover if it is hidden.
static const string for_file = "for.bp";

//  A loop body is named after the file it was written in, so a loop nested
//  inside another cannot overwrite the body its parent is still re-reading:
//  a for in main.bp writes main.for.bp, and a for inside that body writes
//  main.for.for.bp. Deriving it from the filename keeps it deterministic —
//  nothing has to be looked up or kept in step.
static string for_file_for(const string& filename) {
  const string stem = filesystem::path(filename).stem().string();
  return stem.empty() ? for_file : stem + "." + for_file;
}

//  Two spellings of one path ("x.bp", "./x.bp") must compare equal or a cycle
//  slips through and recurses until the stack runs out.
static string canonical_path(const string& path) {
  error_code error;
  const filesystem::path resolved = filesystem::weakly_canonical(path, error);
  if (!error) return resolved.string();

  //  weakly_canonical can fail on a path that does not exist yet, and falling
  //  back to the raw text would make the same file compare unequal depending
  //  on whether it had been written at the time. Absolute is enough to match.
  const filesystem::path absolute = filesystem::absolute(path, error);
  return error ? path : absolute.lexically_normal().string();
}

//  True for a lone '=' outside a string. '==', '!=', '<=' and '>=' are
//  comparisons, not assignment, so a condition is not mistaken for one.
static bool has_assignment(const string& statement) {
  bool in_string = false;
  for (size_t i = 0; i < statement.size(); i++) {
    const char c = statement[i];
    if (c == '"') { in_string = !in_string; continue; }
    if (in_string || c != '=') continue;

    if (i + 1 < statement.size() && statement[i + 1] == '=') { i++; continue; }
    if (i > 0 && (statement[i - 1] == '!' || statement[i - 1] == '<'
      || statement[i - 1] == '>')) continue;

    return true;
  }
  return false;
}

//  Splits on `separator` only where it sits outside quotes and outside any
//  bracket, so `shout("a, b", f(1, 2))` is two arguments and `a && (b && c)`
//  is two factors. Always returns at least one part.
static vector<string> split_top_level(const string& text, const string& separator) {
  vector<string> parts;
  bool in_string = false;
  int depth = 0;
  size_t start = 0;

  for (size_t i = 0; i < text.size(); i++) {
    const char c = text[i];
    if (c == '"') { in_string = !in_string; continue; }
    if (in_string) continue;

    if (c == '(' || c == '[') { depth++; continue; }
    if (c == ')' || c == ']') { depth--; continue; }
    if (depth != 0) continue;

    if (text.compare(i, separator.size(), separator) == 0) {
      parts.push_back(text.substr(start, i - start));
      i += separator.size() - 1;
      start = i + 1;
    }
  }

  parts.push_back(text.substr(start));
  return parts;
}

//  A condition operand: a quoted string, an expression, a variable, or a literal.
static optional<Variable> resolve_operand(const string& text,
                                          const map<string, Variable>& variables) {
  const string operand = trim(text);
  if (operand.empty()) return nullopt;

  if (operand.size() >= 2 && operand.front() == '"' && operand.back() == '"')
    return Variable{VarType::String, false, between(operand, '"', '"')};

  if (auto value = eval_expr(operand, variables)) {
    const VarType type = holds_alternative<double>(*value) ? VarType::Float
                       : holds_alternative<long long>(*value) ? VarType::Long
                       : VarType::Int;
    return Variable{type, false, *value};
  }

  auto it = variables.find(operand);
  if (it != variables.end()) return it->second;

  VarType inferred;
  if (auto value = infer_literal(operand, inferred)) return Variable{inferred, false, *value};

  return nullopt;
}

//  Numeric view of a variable; nullopt for strings, which compare as text.
static optional<double> as_number(const Variable& variable) {
  return visit([](auto&& value) -> optional<double> {
    using T = decay_t<decltype(value)>;
    if constexpr (is_same_v<T, string>) return nullopt;
    else if constexpr (is_same_v<T, bool>) return value ? 1.0 : 0.0;
    else return static_cast<double>(value);
  }, variable.value);
}

//  Position and length of the comparison operator outside any string literal.
static bool find_comparison(const string& condition, size_t& pos, size_t& length) {
  bool in_string = false;
  for (size_t i = 0; i < condition.size(); i++) {
    const char c = condition[i];
    if (c == '"') { in_string = !in_string; continue; }
    if (in_string) continue;

    if (i + 1 < condition.size() && condition[i + 1] == '='
      && (c == '=' || c == '!' || c == '<' || c == '>')) {
      pos = i;
      length = 2;
      return true;
    }
    if (c == '<' || c == '>') {
      pos = i;
      length = 1;
      return true;
    }
  }
  return false;
}

//  Evaluates "a > b" style comparisons, or the truthiness of a lone operand.
//  `ok` comes back false when the condition can't be resolved at all.
static bool evaluate_condition(const string& condition,
                               const map<string, Variable>& variables, bool& ok) {
  ok = true;
  size_t pos = 0;
  size_t length = 0;

  //  `||` binds loosest, then `&&`, then the comparisons below. Both stop early
  //  once the answer is settled, so a later unresolvable term is not reported.
  const vector<string> any = split_top_level(condition, "||");
  if (any.size() > 1) {
    for (const string& term : any) {
      if (evaluate_condition(term, variables, ok)) return true;
      if (!ok) return false;
    }
    return false;
  }

  const vector<string> all = split_top_level(condition, "&&");
  if (all.size() > 1) {
    for (const string& term : all) {
      if (!evaluate_condition(term, variables, ok) || !ok) return false;
    }
    return true;
  }

  //  A term wrapped in its own brackets: (a > 1) && (b < 2).
  const string bare = trim(condition);
  if (!bare.empty() && bare.front() == '(' && between_matching(bare, '(', ')').size() + 2 == bare.size())
    return evaluate_condition(between_matching(bare, '(', ')'), variables, ok);

  if (!find_comparison(condition, pos, length)) {  //  if (flag) / if (count)
    auto operand = resolve_operand(condition, variables);
    if (!operand) { ok = false; return false; }
    if (auto number = as_number(*operand)) return *number != 0;
    return !get<string>(operand->value).empty();
  }

  const string op = condition.substr(pos, length);
  auto left = resolve_operand(condition.substr(0, pos), variables);
  auto right = resolve_operand(condition.substr(pos + length), variables);
  if (!left || !right) { ok = false; return false; }

  const bool left_is_text = holds_alternative<string>(left->value);
  if (left_is_text != holds_alternative<string>(right->value)) {
    ok = false;  //  Comparing text against a number is meaningless.
    return false;
  }

  if (left_is_text) {
    const string& a = get<string>(left->value);
    const string& b = get<string>(right->value);
    if (op == "==") return a == b;
    if (op == "!=") return a != b;
    if (op == "<") return a < b;
    if (op == ">") return a > b;
    if (op == "<=") return a <= b;
    return a >= b;
  }

  const double a = *as_number(*left);
  const double b = *as_number(*right);
  if (op == "==") return a == b;
  if (op == "!=") return a != b;
  if (op == "<") return a < b;
  if (op == ">") return a > b;
  if (op == "<=") return a <= b;
  return a >= b;
}

//  Writes a value a library instruction produced into `name`, creating the
//  variable if it is new and refusing to change a static one's type.
static void store_result(map<string, Variable>& variables, const string& name,
                         VarType type, const VarValue& value) {
  auto it = variables.find(name);

  if (it != variables.end() && it->second.is_static) {
    if (it->second.type != type) {
      cerr << "Type error: " << name << " is " << var_type_name(it->second.type)
        << ", cannot hold " << var_type_name(type) << endl;
      return;
    }
    it->second.value = value;
    return;
  }

  variables[name] = {type, false, value};
}

//  Resolves every argument after the first as a number, for library
//  instructions shaped `name(variable, a, b)`. False if any will not resolve.
static bool numeric_arguments(const vector<string>& arguments,
                              const map<string, Variable>& variables,
                              vector<double>& out) {
  for (size_t i = 1; i < arguments.size(); i++) {
    auto operand = resolve_operand(arguments[i], variables);
    if (!operand) return false;

    auto number = as_number(*operand);
    if (!number) return false;

    out.push_back(*number);
  }
  return true;
}

//  Libraries compiled in from src/libraries, and the instructions each one
//  brings. `use "name"` makes a library's instructions callable; without it
//  they stay refused, so a program has to declare what it depends on.
static const map<string, set<string>> libraries = {
  {"shell_utilities", {"clear", "exec"}},
  {"random", {"randomint", "randomdouble", "randombool", "doublebellcurve"}},
};

//  system() takes a C string and is marked warn_unused_result, so both call
//  sites go through here rather than ignoring what the shell reported.
static void run_shell(const string& command) {
  if (system(command.c_str()) != 0)
    cerr << "exec failed: " << command << endl;
}

//  The library providing `name`, or "" when no library does.
static string providing_library(const string& name) {
  for (const auto& [library, instructions] : libraries)
    if (instructions.count(name)) return library;

  return "";
}

//  A pass over the statements before any of them run. Every `pass("name.bp")`
//  is noted and a bool named after the library records whether that file is
//  actually on disk, so a program can check before it commits to running one.
//  The variable takes the file's stem: pass("libs/maths.bp") sets `maths`.
static void scan_libraries(const vector<string>& statements,
                           map<string, Variable>& variables, bool verbose) {
  for (const string& statement : statements) {
    if (statement.empty() || statement.back() == '{') continue;

    const size_t paren = statement.find('(');
    if (paren == string::npos) continue;
    if (trim(statement.substr(0, paren)) != "pass") continue;

    const string target = between(between(statement, '(', ')'), '"', '"');
    if (target.empty()) continue;

    const string name = filesystem::path(target).stem().string();
    if (name.empty()) continue;

    const bool found = filesystem::exists(target);
    variables[name] = {VarType::Bool, false, found};

    if (verbose)
      cout << "[library] " << name << " = " << (found ? "true" : "false") << endl;
  }
}

//  Index of the '}' closing the block that opens at `start`.
static size_t matching_close(const vector<string>& statements, size_t start) {
  int depth = 0;
  for (size_t i = start; i < statements.size(); i++) {
    if (statements[i] == "}") {
      depth--;
      if (depth == 0) return i;
    } else if (!statements[i].empty() && statements[i].back() == '{') {
      depth++;
    }
  }
  return statements.size();  //  Validation guarantees balance; stay safe anyway.
}

void instruction_loop(bool &flag, const string& filename, const vector<string>& statements,
                      map<string, Variable>& variables, bool verbose,
                      set<string>& active, set<string>& created,
                      set<string> enabled) {
  const string delimiters = "();";
  const string self = canonical_path(filename);
  active.insert(self);

  scan_libraries(statements, variables, verbose);

  bool stop = false;
  for (size_t i = 0; i < statements.size() && !stop; i++) {
    const string& statement = statements[i];
    vector<string> tokens = split_multi(statement, delimiters);

    if (verbose) {
      //  Traced before the statement runs, so anything it prints lands under
      //  its own label. The leading newline guarantees the trace starts at
      //  column 0 even when the statement before it printed without one.
      cout << endl << "[" << i + 1 << "] ";

      bool first = true;
      for (const string& token : tokens) {
        //  split_multi keeps the space that sat against a '(' or '{'.
        const string text = trim(token);
        if (text.empty()) continue;

        if (!first) cout << " ";
        cout << text;
        first = false;
      }
      cout << endl;
    }

    if (statement == "}") continue;  //  Block end, nothing to run.

    if (statement.rfind("use ", 0) == 0 || statement.rfind("use\t", 0) == 0) {
      const string library = between(statement, '"', '"');

      if (!libraries.count(library)) cerr << "Unknown library: " << library << endl;
      else enabled.insert(library);

      continue;
    }

    if (statement.back() == '{') {
      const string header = trim(statement.substr(0, statement.size() - 1));
      const size_t paren = header.find('(');
      const string keyword = trim(header.substr(0, paren));

      if (keyword == "if") {
        const string condition = between_matching(header, '(', ')');
        bool ok = true;
        const bool met = evaluate_condition(condition, variables, ok);

        if (!ok) cerr << "Bad condition: " << condition << endl;
        if (!ok || !met) i = matching_close(statements, i);
      } else if (keyword == "for") {
        const size_t close = matching_close(statements, i);

        //  for("name") writes the body there instead of the default, so two
        //  loops in one program can be told apart while they run.
        const string chosen = between(between_matching(header, '(', ')'), '"', '"');
        const string body_path = chosen.empty() ? for_file_for(filename)
          : (has_extension(chosen, ".bp") ? chosen : chosen + ".bp");

        created.insert(canonical_path(body_path));

        {  //  The body is copied out to a file of its own and then run the
           //  same way `pass` runs one.
          ofstream body_file(body_path);
          for (size_t j = i + 1; j < close && j < statements.size(); j++)
            body_file << statements[j] << endl;
        }

        //  Its own flag, so `end` inside the body ends the loop rather than
        //  the file the loop is written in. The body is re-read every turn,
        //  so editing the hidden file changes the loop while it runs, and a
        //  file caught half-written recovers on the next turn.
        bool body_flag = false;
        vector<string> reported;

        while (!body_flag) {
          vector<string> body;
          vector<string> errors;
          int body_statements = 0;
          int body_semicolons = 0;

          if (!validate_and_count(body_path, body, body_statements,
              body_semicolons, errors)) {
            if (errors != reported) {
              for (const string& error : errors) cerr << error << endl;
              reported = errors;
            }
            continue;
          }
          reported.clear();

          instruction_loop(body_flag, body_path, body, variables, verbose, active, created,
            enabled);
        }

        i = close;
      } else if (!header.empty()) {
        cerr << "Unknown block: " << keyword << endl;
        i = matching_close(statements, i);
      }

      continue;
    }

    if (array_statement(variables, statement)) {
      //  Declaration or element assignment, already applied.
    } else if (has_assignment(statement)) {
      assign_variable(variables, statement);
    } else {
      //  Anything else is a "name(arg)" call; the name alone selects the
      //  builtin, so only the matching one is looked at.
      const size_t paren = statement.find('(');
      if (paren == string::npos) {
        cerr << "Not a statement: " << statement << endl;
        continue;
      }

      const string name = trim(statement.substr(0, paren));
      const string arg = between_matching(statement, '(', ')');

      if (name == "shout") {
        //  Arguments print in turn with nothing between them, so any spacing
        //  you want goes inside the literals: shout("hi ", name, "!");
        for (const string& part : split_top_level(arg, ",")) {
          const string piece = trim(part);

          if (string_contains(piece, "\"")) {           // shout("text");
            cout << between(piece, '"', '"');
          } else if (auto value = eval_expr(piece, variables)) {  // shout(1 + 2);
            print_variable(Variable{VarType::Int, false, *value});
          } else {                                    // shout(z);
            auto it = variables.find(piece);
            if (it != variables.end()) {print_variable(it->second);}
          }
        }
      } else if (name == "break") {
        if (arg.empty()) {
          cout << endl;
        } else try {
          int count = stoi(arg);
          for (int i = 0; i < count; i++)
            cout << endl;
        } catch (const invalid_argument &e) {
          auto it = variables.find(arg);
          if (it != variables.end()) {
            auto value = as_integer(it->second);
            if (value) for (long long i = 0; i < *value; i++)
                cout << endl;
          }
        }
      } else if (const string library = providing_library(name); !library.empty()) {
        if (!enabled.count(library)) {
          cerr << name << " needs: use \"" << library << "\"" << endl;
        } else if (name == "randomint" || name == "randomdouble"
          || name == "randombool" || name == "doublebellcurve") {
          const vector<string> arguments = split_top_level(arg, ",");
          const size_t wanted = name == "randombool" ? 1 : 3;
          vector<double> numbers;

          if (arguments.size() != wanted) {
            cerr << name << " needs " << wanted << " argument(s): "
              << name << (wanted == 1 ? "(variable)" : "(variable, from, to)") << endl;
          } else if (!numeric_arguments(arguments, variables, numbers)) {
            cerr << name << ": could not read its numbers from " << arg << endl;
          } else {
            const string target = trim(arguments[0]);

            if (name == "randomint") {
              int result = 0;
              randomint(result, static_cast<int>(numbers[0]), static_cast<int>(numbers[1]));
              store_result(variables, target, VarType::Int, result);
            } else if (name == "randomdouble") {
              double result = 0;
              randomdouble(result, numbers[0], numbers[1]);
              store_result(variables, target, VarType::Float, result);
            } else if (name == "randombool") {
              bool result = false;
              randombool(result);
              store_result(variables, target, VarType::Bool, result);
            } else {
              double result = 0;
              doublebellcurve(result, numbers[0], numbers[1]);
              store_result(variables, target, VarType::Float, result);
            }
          }
        } else if (name == "clear") {
          clear_screen();
        } else if (name == "exec") {
          if (string_contains(arg, "\"")) {          // exec("ls");
            run_shell(between(arg, '"', '"'));
          } else {                                   // exec(command);
            auto it = variables.find(arg);

            if (it == variables.end()) {
              cerr << "Unknown variable: " << arg << endl;
            } else if (!holds_alternative<string>(it->second.value)) {
              cerr << "exec needs a string, " << arg << " is "
                << var_type_name(it->second.type) << endl;
            } else {
              run_shell(get<string>(it->second.value));
            }
          }
        }
      } else if (name == "end") {
        flag = true;
        stop = true;
      } else if (name == "pass") {
        const string target = between(arg, '"', '"');

        vector<string> target_statements;
        vector<string> errors;
        vector<string> reported;  //  Errors already on screen, so no spam.
        int target_count = 0;
        int target_semicolons = 0;

        //  The file repeats until its own `end` fires. That flag is private,
        //  so ending the repeat cannot stop this file too. It is re-read
        //  every time round, so an edit lands mid-loop and a file caught
        //  half-written recovers on the next turn instead of trapping us.
        bool target_flag = false;
        while (!target_flag) {
          if (!validate_and_count(target, target_statements, target_count,
              target_semicolons, errors)) {
            if (errors != reported) {
              for (const string& error : errors) cerr << error << endl;
                reported = errors;
            }
            continue;
          }
          reported.clear();

          instruction_loop(target_flag, target, target_statements, variables,
            verbose, active, created
          );
        }
      }
      else {cerr << "Unknown function: " << name << endl;}
    }
  }
  active.erase(self);
}
