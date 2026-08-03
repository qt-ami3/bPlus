make

Builds ./bp from main.cpp, src/*.cpp and src/libraries/*.cpp. `make clean`
removes the binary. Adding a .cpp to any of those directories needs no
Makefile edit; the wildcards pick it up.

By hand, if you'd rather:

g++ -Wall -std=c++17 main.cpp src/*.cpp src/libraries/*.cpp -o bp

Two things that are easy to get wrong by hand:

-std=c++17 is required, not optional. Leaving it off works only where the
compiler already defaults to C++17 or later. std::variant, std::optional,
if constexpr and std::filesystem all depend on it.

src/libraries/*.cpp must be listed. Leaving it off still links today, only
because nothing in the interpreter calls into those libraries yet — the
moment an instruction does, omitting it becomes an undefined reference at
link time rather than an obvious error. `make` always includes them.

Layout the build expects:

  main.cpp              interpreter entry point
  src/*.cpp             core interpreter
  src/libraries/*.cpp   optional libraries, kept out of core
  include/*.h           core headers
  include/libraries/*.h optional library headers

src/has_extension.c is the superseded 0.2.2 C version. The wildcards match
*.cpp only, so it stays out of the build.
