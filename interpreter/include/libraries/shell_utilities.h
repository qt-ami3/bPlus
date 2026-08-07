#pragma once

// Terminal helpers, kept out of core include/ and src/: they are optional,
// platform specific, and nothing in interpreting a .bp file depends on them.

// Turns line buffering and echo off, so a keypress arrives without Enter
// having been pressed. Call with true to put the terminal back as it was.
void set_buffered_input(bool enable);

void clear_screen();

// Reads a single keypress, without waiting for Enter. Needs the terminal put
// into unbuffered mode first with set_buffered_input(false).
//
// An arrow key arrives as the three bytes ESC [ A, which no program could
// write as a literal, so those come back named: "up", "down", "left",
// "right". A bare ESC comes back as "escape", Enter as "enter", and anything
// else as the character itself. `key` is left empty at end of input.
void read_key(std::string& key);
