using namespace std;
#include <map>
#include <set>
#include <string>
#include <vector>
#include <cctype>
#include <fstream>
#include <sstream>
#include <csignal>
#include <unistd.h>
#include <filesystem>
#include <iostream>
#include "./include/trim.h"
#include "./include/split.h"
#include "./include/assign.h"
#include "./include/between.h"
#include "./include/variable.h"
#include "./include/eval_expr.h"
#include "./include/validate.h"
#include "./include/libraries/shell_utilities.h"
#include "./include/has_extension.h"
#include "./include/process_ram_kb.h"
#include "./include/string_contains.h"
#include "./include/instruction_loop.h"

//  The loop-body files, for the interrupt to clear away. A pointer rather than
//  a copy because the set is still being added to while the program runs, and
//  null until main has one, so an interrupt arriving early finds nothing.
static const set<string>* created_files = nullptr;

//  A program that turned buffered input off would otherwise leave the shell
//  with no echo once it stops. bP programs usually run until interrupted, so
//  Ctrl-C has to put the terminal back as much as a clean finish does.
//  set_buffered_input(true) does nothing when buffering was never turned off.
//
//  The body files go the same way. An interactive program is normally left
//  with Ctrl-C rather than `end();`, so skipping them here means the usual way
//  out is the one that litters. unlink, not filesystem::remove, which
//  allocates and so cannot be called from a handler.
static void restore_terminal(int signal_number) {
  set_buffered_input(true);

  if (created_files)
    for (const string& path : *created_files) unlink(path.c_str());

  _exit(128 + signal_number);
}

void print_usage(const string& program_name) {
  cerr << "Usage:" << endl
    << program_name << " <filename>" << endl << endl
      << "Options:" << endl
        << "-v or -verbose: " << "Runs with detailed interpreter output.\n";
}

int main(int argc, char* argv[]) {
  signal(SIGINT, restore_terminal);
  signal(SIGTERM, restore_terminal);

  if (argc < 2 || argc > 3) {
    print_usage(argv[0]);
    return 1;
  }

  const string filename = argv[1];

  bool verbose = false;
  if (argc == 3) { //  argv[2] only exists when a flag was passed.
    const string argument = argv[2];
    if (argument != "-v" && argument != "-verbose") {
      print_usage(argv[0]);
      return 1;
    }
    verbose = true;
  }

  ifstream file(filename);
  if (!file) {
    cerr << "Could not open file: " << filename << "\n";
    return 1;
  }

  if (!has_extension(filename, ".bp")) {
    cerr << "Error: expected a .bp file\n";
    return 1;
  }

  int statement_count = 0;
  int semicolon_count = 0;

  vector<string> statements;
  set<string> created;      //  Loop-body files, removed once the program ends.
  created_files = &created;
  vector<string> errors;
  vector<string> reported;  //  Errors already on screen, so a broken file isn't spammed.

  bool instruction_loop_break = false;
  while (!instruction_loop_break) {
    if (verbose) {
      print_system_info(verbose);
    }

    if (!validate_and_count(filename, statements, statement_count, semicolon_count, errors)) {
      if (errors != reported) {
        for (const string& error : errors) cerr << error << endl;
        reported = errors;
      }

      continue;  //  Hold here, re-reading, until the file is valid again.
    }
    reported.clear();

    map<string, Variable> variables;  //  Each pass starts from a clean state.
    set<string> active;               //  Files being run, so `use` can refuse a cycle.
    instruction_loop(instruction_loop_break, filename, statements, variables, verbose,
      active, created);


    if (verbose) {
      print_system_info(verbose);
    }
  }

  set_buffered_input(true);  //  Whatever the program did to the terminal, undo it.

  //  A `for` body is a real file while the program runs, so it can be read and
  //  edited, and is cleared away only once there is nothing left to run.
  for (const string& path : created) {
    error_code error;
    filesystem::remove(path, error);
  }

  if (verbose) {
    cout << "Statements read;" << endl;
    for (const string& statement : statements) cout << statement << endl;

    cout << "Counted statements: " << statement_count << endl;
    cout << "Semicolons counted: " << semicolon_count;
  }

  cout << "\n";
}
