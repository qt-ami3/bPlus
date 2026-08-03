#pragma once

// Terminal helpers, kept out of core include/ and src/: they are optional,
// platform specific, and nothing in interpreting a .bp file depends on them.

// Turns line buffering and echo off, so a keypress arrives without Enter
// having been pressed. Call with true to put the terminal back as it was.
void setBufferedInput(bool enable);

void clearScreen();
