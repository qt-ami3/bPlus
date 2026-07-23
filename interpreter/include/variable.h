#pragma once
#include <string>
#include <variant>
#include <optional>

enum class VarType { Int, Long, Float, String, Bool };

using VarValue = std::variant<int, long long, double, std::string, bool>;

struct Variable {
  VarType type;
  bool is_static; // Declared with a type in front; reassignments must keep that type.
  VarValue value;
};

std::string var_type_name(VarType type);
std::optional<VarType> type_from_keyword(const std::string& word);
void print_variable(const Variable& variable);
std::optional<long long> as_integer(const Variable& variable);
