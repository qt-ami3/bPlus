#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>
#include "variable.h"

// Runs every statement in `statements` once, executing as it goes.
// `flag` is set by the `end` instruction, which also stops this loop; a file
// pulled in by `use` gets its own flag, so its `end` cannot stop the caller.
// `active` holds the files currently being run, so `use` can refuse a cycle.
// `variables` is shared with every file `use` pulls in.
void instruction_loop(bool& flag, const std::string& filename,
                      const std::vector<std::string>& statements,
                      std::map<std::string, Variable>& variables, bool verbose,
                      std::set<std::string>& active);
