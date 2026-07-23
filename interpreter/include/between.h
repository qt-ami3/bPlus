#pragma once
#include <string>

// Returns the substring strictly between the first `open` and the following
// `close`, excluding both. Returns "" if either character isn't found.
std::string between(const std::string& str, char open, char close);
