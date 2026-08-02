#pragma once
#include <string>
#include <vector>

// Reads `filename` into `statements`, one per non-empty line: either a
// ';'-terminated statement, a block header ending in '{', or a lone '}'.
// Checks that every statement line carries exactly one ';', that braces sit on
// a line of their own, and that they balance. Fills `errors` and returns false
// when they don't. Called once per pass, and again per file `use` pulls in.
bool validate_and_count(const std::string& filename, std::vector<std::string>& statements,
                        int& statement_count, int& semicolon_count,
                        std::vector<std::string>& errors);
