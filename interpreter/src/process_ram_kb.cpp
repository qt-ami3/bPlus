#include <cstdio>
#include <fstream>
#include <string>
#include <iostream>

using namespace std;

long get_process_ram_kb() {
  ifstream f("/proc/self/status");
  string line;
  while (getline(f, line)) {
    if (line.substr(0, 6) == "VmRSS:") {
      long kb;
      sscanf(line.c_str(), "VmRSS: %ld", &kb);
      return kb;
    }
  }
  return 0;
}

void print_system_info(bool enabled) {
  if (!enabled) return;
  cout << "\nRAM: " << get_process_ram_kb() << " KB" << endl;
}
