using namespace std;
#include <map>
#include <set>
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <filesystem>
#include <type_traits>
#include <cctype>
#include <iostream>
#include "../include/trim.h"
#include "../include/split.h"
#include "../include/assign.h"
#include "../include/between.h"
#include "../include/variable.h"
#include "../include/eval_expr.h"
#include "../include/validate.h"
#include "../include/parse_literal.h"
#include "../include/string_contains.h"
#include "../include/instruction_loop.h"

//  Two spellings of one path ("x.bp", "./x.bp") must compare equal or a cycle
//  slips through and recurses until the stack runs out.
static string canonical_path(const string& path) {
  error_code error;
  const filesystem::path resolved = filesystem::weakly_canonical(path, error);
  return error ? path : resolved.string();
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
                      set<string>& active) {
  const string delimiters = "();";
  const string self = canonical_path(filename);
  active.insert(self);

  bool stop = false;
  for (size_t i = 0; i < statements.size() && !stop; i++) {
    const string& statement = statements[i];
    vector<string> tokens = split_multi(statement, delimiters);

    if (statement == "}") continue;  //  Block end, nothing to run.

    if (statement.back() == '{') {
      const string header = trim(statement.substr(0, statement.size() - 1));
      const size_t paren = header.find('(');
      const string keyword = trim(header.substr(0, paren));

      if (keyword == "if") {
        const string condition = between(header, '(', ')');
        bool ok = true;
        const bool met = evaluate_condition(condition, variables, ok);

        if (!ok) cerr << "Bad condition: " << condition << endl;
        if (!ok || !met) i = matching_close(statements, i);
      } else if (!header.empty()) {
        cerr << "Unknown block: " << keyword << endl;
        i = matching_close(statements, i);
      }

      continue;
    }

    if (has_assignment(statement)) {
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
      const string arg = between(statement, '(', ')');

      if (name == "shout") {
        if (string_contains(arg, "\"")) {             // shout("text");
          cout << between(arg, '"', '"');
        } else if (auto value = eval_expr(arg, variables)) {  // shout(1 + 2);
          print_variable(Variable{VarType::Int, false, *value});
        } else {                                      // shout(z);
          auto it = variables.find(arg);
          if (it != variables.end()) print_variable(it->second);
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
      } else if (name == "end") {
        flag = true;
        stop = true;
      } else if (name == "use") {
        const string target = between(arg, '"', '"');

        if (active.count(canonical_path(target))) {
          cerr << "Cyclic use: " << target << endl;
        } else {
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
              verbose, active);
          }
        }
      }

      else {
        cerr << "Unknown function: " << name << endl;
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

  active.erase(self);
}
