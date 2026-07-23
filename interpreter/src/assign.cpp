#include "../include/assign.h"
#include "../include/split.h"
#include "../include/between.h"
#include "../include/trim.h"
#include "../include/parse_literal.h"
#include <iostream>

void assign_variable(std::map<std::string, Variable>& variables, const std::string& statement) {
  std::string lhs = trim(split(statement, '=')[0]);
  std::string rhs = trim(between(statement, '=', ';'));

  std::vector<std::string> lhs_tokens = split_multi(lhs, " \t\n\r");

  std::string name;
  std::optional<VarType> declared_type;

  if (lhs_tokens.size() == 1) {
    name = lhs_tokens[0];
  } else if (lhs_tokens.size() == 2) {
    declared_type = type_from_keyword(lhs_tokens[0]);
    if (!declared_type) {
      std::cerr << "Unknown type: " << lhs_tokens[0] << "\n";
      return;
    }
    name = lhs_tokens[1];
  } else {
    std::cerr << "Invalid variable declaration: " << statement << "\n";
    return;
  }

  if (declared_type) {
    auto value = parse_literal_as(rhs, *declared_type);
    if (!value) {
      std::cerr << "Type error: cannot assign '" << rhs << "' to " << var_type_name(*declared_type) << " " << name << "\n";
      return;
    }
    variables[name] = {*declared_type, true, *value};
    return;
  }

  auto existing = variables.find(name);
  if (existing != variables.end() && existing->second.is_static) {
    auto value = parse_literal_as(rhs, existing->second.type);
    if (!value) {
      std::cerr << "Type error: " << name << " is " << var_type_name(existing->second.type)
                 << ", cannot assign '" << rhs << "'\n";
      return;
    }
    existing->second.value = *value;
    return;
  }

  VarType inferred_type;
  auto value = infer_literal(rhs, inferred_type);
  if (!value) {
    std::cerr << "Could not infer type for: " << rhs << "\n";
    return;
  }
  variables[name] = {inferred_type, false, *value};
}
