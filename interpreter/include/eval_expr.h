#pragma once
#include <map>
#include <string>
#include <optional>
#include "variable.h"

// Evaluates a "+ - * /" expression with standard precedence and parentheses.
// Operands are numeric literals or variable names resolved from `variables`.
// Returns nullopt when `expr` contains no operator (so the caller can fall
// back to single-literal parsing) or is syntactically not an expression.
// Semantic errors (unknown name, divide-by-zero) are reported to stderr and
// also return nullopt.
std::optional<VarValue> eval_expr(const std::string& expr,
                                  const std::map<std::string, Variable>& variables);
