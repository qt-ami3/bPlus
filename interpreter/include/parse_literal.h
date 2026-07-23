#pragma once
#include <string>
#include <optional>
#include "variable.h"

std::optional<VarValue> parse_literal_as(const std::string& literal, VarType type);
std::optional<VarValue> infer_literal(const std::string& literal, VarType& out_type);
