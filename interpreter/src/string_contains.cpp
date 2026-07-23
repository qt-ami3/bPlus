#include "../include/string_contains.h"

bool string_contains(const std::string& str, const std::string& needle) {
  return str.find(needle) != std::string::npos;
}
