#pragma once
#include <map>
#include <string>
#include "variable.h"

// Parses "name = value;" or "type name = value;", applies it to `variables`.
// A type prefix always (re)declares the variable as static with that type.
// A bare name reassigns an existing static variable (type-checked against
// its stored type) or, for a new/dynamic name, infers the type from `value`.
void assign_variable(std::map<std::string, Variable>& variables, const std::string& statement);
