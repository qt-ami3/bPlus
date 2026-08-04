#pragma once
#include <string>

// Returns the substring strictly between the first `open` and the following
// `close`, excluding both. Returns "" if either character isn't found.
std::string between(const std::string& str, char open, char close);

// Same, but pairs the first `open` with the `close` that actually matches it,
// counting nesting and ignoring both characters inside "..." literals. This is
// what lets an argument hold brackets of its own: shout((2 + 3) * 4).
// Returns "" when there is no `open`, or nothing closes it.
std::string between_matching(const std::string& str, char open, char close);
