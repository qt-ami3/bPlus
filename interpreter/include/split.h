#pragma once
#include <string>
#include <vector>

std::vector<std::string> split(const std::string& str, char delimiter);
std::vector<std::string> split_multi(const std::string& str, const std::string& delimiters);
