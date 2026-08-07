using namespace std;
#include <cmath>
#include <iomanip>
#include <limits>
#include <variant>
#include <iostream>
#include <type_traits>
#include "../include/trim.h"
#include "../include/split.h"
#include "../include/assign.h"
#include "../include/between.h"
#include "../include/eval_expr.h"
#include "../include/parse_literal.h"

//  Narrows an expression result to a declared type. A whole number fits int or
//  long; anything numeric fits float. A fractional result will not fit a whole
//  number type, so `int n = 7 / 2;` is still a type error rather than silently
//  losing the half.
static std::optional<VarValue> narrow(const VarValue& value, VarType type, bool& reported) {
  return std::visit([&](auto&& held) -> std::optional<VarValue> {
    using T = std::decay_t<decltype(held)>;
    if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, bool>) {
      return std::nullopt;
    } else {
      const double number = static_cast<double>(held);

      if (type == VarType::Float) return VarValue(number);
      if (number != std::floor(number)) return std::nullopt;

      //  Casting a double that does not fit the target is undefined, and in
      //  practice wraps: doubling an int past its limit came back negative.
      //  Checked against the type's own range instead, and said out loud,
      //  since "cannot assign 'i * 2'" reads like a type mistake rather than
      //  a number that grew too big.
      //  Compared as double, and (double)LLONG_MAX rounds *up* to 2^63, which
      //  is the first value that does not fit. So long needs a strict <, while
      //  int's limits are both exact in a double.
      const bool fits = type == VarType::Int
        ? number >= static_cast<double>(std::numeric_limits<int>::min())
          && number <= static_cast<double>(std::numeric_limits<int>::max())
        : number >= static_cast<double>(std::numeric_limits<long long>::min())
          && number < std::ldexp(1.0, 63);

      if (!fits) {
        std::cerr << "Out of range for " << var_type_name(type) << ": "
          << std::fixed << std::setprecision(0) << number << "\n";
        reported = true;
        return std::nullopt;
      }

      if (type == VarType::Int) return VarValue(static_cast<int>(number));
      if (type == VarType::Long) return VarValue(static_cast<long long>(number));
      return std::nullopt;
    }
  }, value);
}

//  A value for a variable of `type`: a plain literal, or an expression whose
//  result fits it. Without the expression half, a typed variable could never be
//  built from anything but a literal, so `int i = 0; i = i + 1;` failed.
static std::optional<VarValue> value_for(const std::string& rhs, VarType type,
                                         const std::map<std::string, Variable>& variables,
                                         bool& reported) {
  if (auto literal = parse_literal_as(rhs, type)) return literal;

  auto result = eval_expr(rhs, variables);
  if (!result) return std::nullopt;

  return narrow(*result, type, reported);
}

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
    bool reported = false;
    auto value = value_for(rhs, *declared_type, variables, reported);
    if (!value) {
      if (!reported) std::cerr << "Type error: cannot assign '" << rhs << "' to " 
        << var_type_name(*declared_type) << " " << name << "\n";
      return;
    }
    variables[name] = {*declared_type, true, *value};
    return;
  }

  auto existing = variables.find(name);
  if (existing != variables.end() && existing->second.is_static) {
    bool reported = false;
    auto value = value_for(rhs, existing->second.type, variables, reported);
    if (!value) {
      if (!reported)
        std::cerr << "Type error: " << name << " is " << var_type_name(existing->second.type)
                 << ", cannot assign '" << rhs << "'\n";
      return;
    }
    existing->second.value = *value;
    return;
  }

  if (auto expr_value = eval_expr(rhs, variables)) {   // z = x + y;
    VarType t = std::holds_alternative<double>(*expr_value) ? VarType::Float
              : std::holds_alternative<long long>(*expr_value) ? VarType::Long
              : VarType::Int;
    variables[name] = {t, false, *expr_value};
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
