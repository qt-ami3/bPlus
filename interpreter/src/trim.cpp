#include "../include/trim.h"

std::string trim(const std::string& str) {
  size_t first = str.find_first_not_of(" \n\t\r");
  size_t last = str.find_last_not_of(" \n\t\r");
  return (first == std::string::npos) ? "" : str.substr(first, last - first + 1);
}
