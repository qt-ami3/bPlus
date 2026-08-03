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

// Name under which `name`'s declaration is stored.
std::string array_header(const std::string& name);

// Handles the array forms: "arr[3];", "int arr[3];", "arr[3] = {1,2,3};",
// "int arr[3] = {1,2,3};" and element assignment "arr[0] = value;".
// Declaring fills every element with a zero of the element type, so an unset
// slot reads as 0 rather than as a missing variable. Returns false when the
// statement is not an array statement at all, leaving it for the caller.
bool array_statement(std::map<std::string, Variable>& variables,
                     const std::string& statement);
