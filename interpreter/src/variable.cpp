#include "../include/variable.h"
#include <iostream>
#include <type_traits>

std::string var_type_name(VarType type) {
  switch (type) {
    case VarType::Int: return "int";
    case VarType::Long: return "long";
    case VarType::Float: return "float";
    case VarType::String: return "string";
    case VarType::Bool: return "bool";
  }
  return "unknown";
}

std::optional<VarType> type_from_keyword(const std::string& word) {
  if (word == "int") return VarType::Int;
  if (word == "long") return VarType::Long;
  if (word == "float" || word == "double") return VarType::Float;
  if (word == "string") return VarType::String;
  if (word == "bool") return VarType::Bool;
  return std::nullopt;
}

void print_variable(const Variable& variable) {
  std::visit([](auto&& value) {
    using T = std::decay_t<decltype(value)>;
    if constexpr (std::is_same_v<T, bool>) {
      std::cout << (value ? "true" : "false");
    } else {
      std::cout << value;
    }
  }, variable.value);
}

std::optional<long long> as_integer(const Variable& variable) {
  return std::visit([](auto&& value) -> std::optional<long long> {
    using T = std::decay_t<decltype(value)>;
    if constexpr (std::is_same_v<T, std::string>) {
      return std::nullopt;
    } else if constexpr (std::is_same_v<T, bool>) {
      return value ? 1 : 0;
    } else {
      return static_cast<long long>(value);
    }
  }, variable.value);
}
