using namespace std;
#include <string>
#include <vector>
#include <fstream>
#include "../include/trim.h"
#include "../include/validate.h"

bool validate_and_count(const string& filename, vector<string>& statements,
                        int& statement_count, int& semicolon_count, vector<string>& errors) {
  statements.clear();
  statement_count = 0;
  semicolon_count = 0;
  errors.clear();

  ifstream file(filename);
  if (!file) {
    errors.push_back("Could not open file: " + filename);
    return false;
  }

  bool in_string = false;
  int depth = 0;
  int line_number = 0;
  string line;
  while (getline(file, line)) {
    line_number++;

    vector<size_t> semicolons;  //  Positions of every ';' outside a string.
    int opens = 0;
    int closes = 0;
    for (size_t i = 0; i < line.size(); i++) {
      if (line[i] == '"') in_string = !in_string;
      else if (in_string) continue;
      else if (line[i] == ';') semicolons.push_back(i);
      else if (line[i] == '{') opens++;
      else if (line[i] == '}') closes++;
    }

    const string statement = trim(line);
    if (statement.empty()) continue;

    statement_count++;
    semicolon_count += semicolons.size();

    //  A block opens with a header ending in '{' and closes with a lone '}',
    //  each on a line of its own and neither carrying a ';'.
    if (semicolons.empty() && opens == 1 && closes == 0 && statement.back() == '{') {
      depth++;
      statements.push_back(statement);
      continue;
    }

    if (semicolons.empty() && closes == 1 && opens == 0 && statement == "}") {
      if (depth == 0) {
        errors.push_back("Unmatched '}' on line " + to_string(line_number));
        continue;
      }
      depth--;
      statements.push_back(statement);
      continue;
    }

    if (opens > 0 || closes > 0) {
      errors.push_back("Braces belong on a line of their own, line " + to_string(line_number)
        + ": " + statement);
      continue;
    }

    if (semicolons.empty()) {
      errors.push_back("Missing ';' for statement line " + to_string(line_number)
        + ": " + statement + "*;*");
      continue;
    }

    //  A statement ends at its first ';', so any that follow are extra.
    for (size_t i = 1; i < semicolons.size(); i++) {
      errors.push_back("Extra ';' for statement line " + to_string(line_number)
        + ": " + trim(line.substr(0, semicolons[i])) + "*;*"
        + trim(line.substr(semicolons[i] + 1)));
    }

    statements.push_back(statement);
  }

  if (depth > 0)
    errors.push_back("Unclosed '{': " + to_string(depth) + " block(s) never closed");

  return errors.empty();
}
