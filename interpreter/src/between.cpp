#include "../include/between.h"

std::string between(const std::string& str, char open, char close) {
  size_t start = str.find(open);
  if (start == std::string::npos) return "";

  size_t end = str.find(close, start + 1);
  if (end == std::string::npos) return "";

  return str.substr(start + 1, end - start - 1);
}
