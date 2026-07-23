#include "../include/split.h"
#include <sstream>

std::vector<std::string> split(const std::string &str, char delimiter) {
  std::vector<std::string> tokens;
  std::stringstream ss(str);
  std::string item;

  while (getline(ss, item, delimiter)) {
    tokens.push_back(item); // Keep empty tokens if delimiters are consecutive
  }

  return tokens;
}

std::vector<std::string> split_multi(const std::string &str, const std::string &delimiters) {
  std::vector<std::string> tokens;
  size_t start = 0;
  size_t end = 0;

  while ((end = str.find_first_of(delimiters, start)) != std::string::npos) {
    if (end != start) { // Avoid empty tokens from consecutive delimiters
      tokens.push_back(str.substr(start, end - start));
    }
      start = end + 1;
  }

    // Add the last token if not empty
  if (start < str.size()) {
    tokens.push_back(str.substr(start));
  }

  return tokens;
}
