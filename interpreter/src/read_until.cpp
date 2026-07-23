#include "../include/read_until.h"
#include <fstream>

std::string read_until(const std::string& filename, char stop_char, int count, int skip) {
  std::ifstream file(filename);
  std::string result;
  int skipped = 0;
  int seen = 0;
  char c;

  while (skipped < skip && file.get(c)) {
    if (c == stop_char) skipped++;
  }

  while (file.get(c)) {
    result += c;
    if (c == stop_char) {
      seen++;
      if (seen >= count) break;
    }
  }

  return result;
}
