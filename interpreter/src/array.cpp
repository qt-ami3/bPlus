using namespace std;
#include <map>
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <iostream>
#include "../include/trim.h"
#include "../include/split.h"
#include "../include/array.h"
#include "../include/between.h"
#include "../include/variable.h"
#include "../include/eval_expr.h"
#include "../include/parse_literal.h"

string array_element(const string& name, long long index) {
  return name + "[" + to_string(index) + "]";
}

string array_header(const string& name) {
  return name + "[]";
}

//  What a freshly declared element holds, so an unset slot reads as a zero of
//  its type instead of coming back as a missing variable.
static VarValue default_value(VarType type) {
  switch (type) {
    case VarType::Int: return VarValue(0);
    case VarType::Long: return VarValue(static_cast<long long>(0));
    case VarType::Float: return VarValue(0.0);
    case VarType::String: return VarValue(string());
    case VarType::Bool: return VarValue(false);
  }
  return VarValue(0);
}

//  A value on the right of an '=', or a length inside the brackets: an
//  expression, a variable, or a literal.
static optional<VarValue> resolve_value(const string& text,
                                        const map<string, Variable>& variables,
                                        VarType& type) {
  const string value = trim(text);
  if (value.empty()) return nullopt;

  if (auto result = eval_expr(value, variables)) {
    type = holds_alternative<double>(*result) ? VarType::Float
         : holds_alternative<long long>(*result) ? VarType::Long
         : VarType::Int;
    return result;
  }

  auto it = variables.find(value);
  if (it != variables.end()) {
    type = it->second.type;
    return it->second.value;
  }

  VarType inferred;
  if (auto literal = infer_literal(value, inferred)) {
    type = inferred;
    return literal;
  }

  return nullopt;
}

//  Whole number out of resolved text, for a length or an index.
static optional<long long> as_whole(const string& text,
                                    const map<string, Variable>& variables) {
  VarType type;
  auto value = resolve_value(text, variables, type);
  if (!value) return nullopt;

  return as_integer(Variable{type, false, *value});
}

bool array_statement(map<string, Variable>& variables, const string& statement) {
  const size_t equals = statement.find('=');
  const string lhs = statement.substr(0, equals == string::npos ? statement.size() : equals);

  //  A '[' left of the '=' is what marks an array statement. `shout(arr[0]);`
  //  has its bracket inside a call, so it is left alone.
  const size_t open = lhs.find('[');
  if (open == string::npos || lhs.find('(') != string::npos) return false;

  const size_t close = lhs.find(']', open);
  if (close == string::npos) return false;

  const vector<string> words = split_multi(trim(lhs.substr(0, open)), " \t");

  string name;
  optional<VarType> declared;
  if (words.size() == 1) {
    name = words[0];
  } else if (words.size() == 2) {
    declared = type_from_keyword(words[0]);
    if (!declared) {
      cerr << "Unknown type: " << words[0] << endl;
      return true;
    }
    name = words[1];
  } else {
    cerr << "Invalid array declaration: " << statement << endl;
    return true;
  }

  const string index_text = trim(lhs.substr(open + 1, close - open - 1));
  const string rhs = equals == string::npos ? "" : trim(between(statement, '=', ';'));
  const bool initialiser = !rhs.empty() && rhs.front() == '{';

  if (declared || initialiser || equals == string::npos) {
    const auto length = as_whole(index_text, variables);
    if (!length || *length < 0) {
      cerr << "Bad array length: " << index_text << endl;
      return true;
    }

    const VarType element_type = declared ? *declared : VarType::Int;
    variables[array_header(name)] =
      {element_type, declared.has_value(), VarValue(static_cast<int>(*length))};

    for (long long i = 0; i < *length; i++)
      variables[array_element(name, i)] =
        {element_type, declared.has_value(), default_value(element_type)};

    if (!initialiser) return true;

    const vector<string> items = split(between(rhs, '{', '}'), ',');
    if (static_cast<long long>(items.size()) > *length)
      cerr << "Too many values for " << name << endl;

    for (size_t i = 0; i < items.size() && static_cast<long long>(i) < *length; i++) {
      const string item = trim(items[i]);

      if (declared) {
        auto typed = parse_literal_as(item, *declared);
        if (!typed) {
          cerr << "Type error: cannot assign '" << item << "' to "
            << var_type_name(*declared) << " " << name << endl;
          continue;
        }
        variables[array_element(name, i)] = {*declared, true, *typed};
        continue;
      }

      VarType type;
      auto value = resolve_value(item, variables, type);
      if (!value) {
        cerr << "Could not infer type for: " << item << endl;
        continue;
      }
      variables[array_element(name, i)] = {type, false, *value};
    }

    return true;
  }

  //  arr[0] = value;
  auto header = variables.find(array_header(name));
  if (header == variables.end()) {
    cerr << "Unknown array: " << name << endl;
    return true;
  }

  const auto index = as_whole(index_text, variables);
  const auto length = as_integer(header->second);
  if (!index || !length || *index < 0 || *index >= *length) {
    cerr << "Index out of range: " << name << "[" << index_text << "]" << endl;
    return true;
  }

  if (header->second.is_static) {
    auto typed = parse_literal_as(rhs, header->second.type);
    if (!typed) {
      cerr << "Type error: cannot assign '" << rhs << "' to "
        << var_type_name(header->second.type) << " " << name << endl;
      return true;
    }
    variables[array_element(name, *index)] = {header->second.type, true, *typed};
    return true;
  }

  VarType type;
  auto value = resolve_value(rhs, variables, type);
  if (!value) {
    cerr << "Could not infer type for: " << rhs << endl;
    return true;
  }
  variables[array_element(name, *index)] = {type, false, *value};

  return true;
}
