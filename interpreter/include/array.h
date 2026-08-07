#pragma once
#include <map>
#include <string>
#include "variable.h"

// Arrays live in the ordinary variable map under mangled names: element i of
// `arr` is stored as `arr[i]`, and `arr[]` holds the declaration itself, with
// the element type in `type`, whether it was declared with a type in
// `is_static`, and the length in `value`. Nothing else in the interpreter has
// to know about arrays: lookups, printing and conditions all resolve an
// element by its mangled name like any other variable.

// Name under which element `index` of `name` is stored.
std::string array_element(const std::string& name, long long index);

// Name under which row `row`, column `column` of a two dimensional `name` is
// stored: `grid[1][2]`.
std::string array_element(const std::string& name, long long row, long long column);

// Name under which `name`'s declaration is stored: `name[]`, holding the
// length, or for a two dimensional array the number of rows.
std::string array_header(const std::string& name);

// Name under which a two dimensional `name` keeps its column count: `name[][]`.
// Its absence is what marks an array as one dimensional.
std::string array_columns(const std::string& name);

// Handles the array forms in one and two dimensions: "arr[3];", "int arr[3];",
// "arr[3] = {1,2,3};", "grid[2][2];", "grid[2][2] = {{1,2},{3,4}};" (a flat
// "{1,2,3,4}" is taken row by row), and element assignment "arr[0] = value;"
// or "grid[1][0] = value;".
// Declaring fills every element with a zero of the element type, so an unset
// slot reads as 0 rather than as a missing variable. Returns false when the
// statement is not an array statement at all, leaving it for the caller.
bool array_statement(std::map<std::string, Variable>& variables,
                     const std::string& statement);
