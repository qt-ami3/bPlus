using namespace std;
#include <string>
#include <vector>
#include <fstream>
#include "../include/trim.h"
#include "../include/validate.h"

namespace {

//  What a piece of text is made of, ignoring anything inside a quoted literal
//  of either kind, so a ';' or a brace in "text" or 'A' does not count.
struct Shape {
  vector<size_t> semicolons;
  int opens = 0;
  int closes = 0;
};

Shape shape_of(const string& text) {
  Shape shape;
  char quote = 0;  //  Which quote opened the text we are inside, if any.

  for (size_t i = 0; i < text.size(); i++) {
    if (quote) { if (text[i] == quote) quote = 0; continue; }
    if (text[i] == '"' || text[i] == '\'') { quote = text[i]; continue; }

    if (text[i] == ';') shape.semicolons.push_back(i);
    else if (text[i] == '{') shape.opens++;
    else if (text[i] == '}') shape.closes++;
  }

  return shape;
}

//  The word before the first '(' — `if`, `for`, or an instruction name.
string keyword_of(const string& text) {
  return trim(text.substr(0, text.find('(')));
}

}  // namespace

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

  int depth = 0;
  int line_number = 0;
  int began_on = 0;    //  Line the statement being gathered started on.
  string pending;      //  Lines joined until they make a whole statement.
  string line;

  while (getline(file, line)) {
    line_number++;

    const string piece = trim(line);
    if (piece.empty() && pending.empty()) continue;
    if (piece.empty()) continue;  //  A blank line inside a statement is spacing.

    if (pending.empty()) began_on = line_number;
    pending += pending.empty() ? piece : " " + piece;

    const Shape shape = shape_of(pending);
    const string keyword = keyword_of(pending);

    //  A statement is whole once one of these is true; until then the next
    //  line belongs to it, which is what lets a condition or an array
    //  initialiser be written across several lines.
    const bool block_header = pending.back() == '{'
      && (keyword == "if" || keyword == "for" || pending == "{");
    const bool block_close = pending == "}";
    const bool declaration = pending.rfind("use ", 0) == 0 || pending.rfind("use\t", 0) == 0;
    const bool ended = !shape.semicolons.empty() && shape.opens == shape.closes;

    if (!block_header && !block_close && !declaration && !ended) continue;

    const string statement = pending;
    pending.clear();

    statement_count++;
    semicolon_count += shape.semicolons.size();

    if (block_header) {
      depth++;
      statements.push_back(statement);
      continue;
    }

    if (block_close) {
      if (depth == 0) {
        errors.push_back("Unmatched '}' on line " + to_string(line_number));
        continue;
      }
      depth--;
      statements.push_back(statement);
      continue;
    }

    //  `use "library"` is a declaration rather than a statement: it names a
    //  library compiled in from src/libraries and carries no ';'.
    if (declaration) {
      statements.push_back(statement);
      continue;
    }

    //  Braces on a finished statement are data — an array initialiser — but a
    //  block keyword carrying its own braces is a block written on one line,
    //  which would otherwise fail later as an unknown instruction.
    if (shape.opens > 0 && (keyword == "if" || keyword == "for")) {
      errors.push_back("Braces belong on a line of their own, line " + to_string(began_on)
        + ": " + statement);
      continue;
    }

    //  A statement ends at its first ';', so any that follow are extra.
    for (size_t i = 1; i < shape.semicolons.size(); i++) {
      errors.push_back("Extra ';' for statement line " + to_string(began_on)
        + ": " + trim(statement.substr(0, shape.semicolons[i])) + "*;*"
        + trim(statement.substr(shape.semicolons[i] + 1)));
    }

    statements.push_back(statement);
  }

  //  Anything still being gathered at the end never finished.
  if (!pending.empty()) {
    const Shape shape = shape_of(pending);
    if (shape.opens != shape.closes)
      errors.push_back("Unclosed '{' for statement line " + to_string(began_on)
        + ": " + pending);
    else
      errors.push_back("Missing ';' for statement line " + to_string(began_on)
        + ": " + pending + "*;*");
  }

  if (depth > 0)
    errors.push_back("Unclosed '{': " + to_string(depth) + " block(s) never closed");

  return errors.empty();
}
