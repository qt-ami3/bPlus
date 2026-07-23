#pragma once
#include <string>

// Reads a file's contents as a string, stopping once `stop_char` has
// occurred `count` times. The stop character(s) are included in the result.
// `skip` discards the file's content through the first `skip` occurrences
// of `stop_char` before reading begins (e.g. skip=1, count=1 reads from
// just after the first `stop_char` through the second).
std::string read_until(const std::string& filename, char stop_char, int count = 1, int skip = 0);
