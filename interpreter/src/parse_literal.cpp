#include "../include/parse_literal.h"
#include "../include/between.h"
#include "../include/string_contains.h"
#include <stdexcept>

std::optional<VarValue> parse_literal_as(const std::string& literal, VarType type) {
  try {
    size_t pos;
    switch (type) {
      case VarType::Int: {
        int value = std::stoi(literal, &pos);
        if (pos != literal.size()) return std::nullopt;
        return VarValue(value);
      }
      case VarType::Long: {
        long long value = std::stoll(literal, &pos);
        if (pos != literal.size()) return std::nullopt;
        return VarValue(value);
      }
      case VarType::Float: {
        double value = std::stod(literal, &pos);
        if (pos != literal.size()) return std::nullopt;
        return VarValue(value);
      }
      case VarType::String: {
        if (literal.size() < 2 || literal.front() != '"' || literal.back() != '"') return std::nullopt;
        return VarValue(between(literal, '"', '"'));
      }
      case VarType::Bool: {
        if (literal == "true") return VarValue(true);
        if (literal == "false") return VarValue(false);
        return std::nullopt;
      }
    }
  } catch (const std::invalid_argument&) {
    return std::nullopt;
  } catch (const std::out_of_range&) {
    return std::nullopt;
  }
  return std::nullopt;
}

std::optional<VarValue> infer_literal(const std::string& literal, VarType& out_type) {
  if (literal.size() >= 2 && literal.front() == '"' && literal.back() == '"') {
    out_type = VarType::String;
    return parse_literal_as(literal, VarType::String);
  }

  if (literal == "true" || literal == "false") {
    out_type = VarType::Bool;
    return parse_literal_as(literal, VarType::Bool);
  }

  if (string_contains(literal, ".")) {
    auto value = parse_literal_as(literal, VarType::Float);
    if (value) {
      out_type = VarType::Float;
      return value;
    }
    return std::nullopt;
  }

  auto int_value = parse_literal_as(literal, VarType::Int);
  if (int_value) {
    out_type = VarType::Int;
    return int_value;
  }

  auto long_value = parse_literal_as(literal, VarType::Long);
  if (long_value) {
    out_type = VarType::Long;
    return long_value;
  }

  return std::nullopt;
}
