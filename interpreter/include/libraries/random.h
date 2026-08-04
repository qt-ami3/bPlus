#pragma once

// Random numbers, kept out of core include/ and src/: nothing in interpreting
// a .bp file depends on them.
//
// Each function writes through its first parameter rather than returning,
// which is how a bP instruction hands a result back — instructions are
// statements and cannot appear inside an expression.

// Whole number in [from, up_to], both ends included. Order does not matter.
void randomint(int& result, int from, int up_to);

// Real number in [from, up_to). Order does not matter.
void randomdouble(double& result, double from, double up_to);

// true or false, evenly.
void randombool(bool& result);

// Normally distributed around `mean`, spread by `deviation`. A negative
// deviation is treated as its absolute value; zero always gives `mean`.
void doublebellcurve(double& result, double mean, double deviation);
