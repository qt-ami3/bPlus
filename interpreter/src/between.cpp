#include "../include/between.h"

std::string between(const std::string& str, char open, char close) {
  size_t start = str.find(open);
  if (start == std::string::npos) return "";

  size_t end = str.find(close, start + 1);
  if (end == std::string::npos) return "";

  return str.substr(start + 1, end - start - 1);
}

std::string between_matching(const std::string& str, char open, char close) {
  bool in_string = false;
  size_t start = std::string::npos;

  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == '"') in_string = !in_string;
    else if (!in_string && str[i] == open) { start = i; break; }
  }

  if (start == std::string::npos) return "";

  int depth = 0;
  for (size_t i = start; i < str.size(); i++) {
    const char c = str[i];
    if (c == '"') in_string = !in_string;
    else if (in_string) continue;
    else if (c == open) depth++;
    else if (c == close && --depth == 0) return str.substr(start + 1, i - start - 1);
  }

  return "";
}
