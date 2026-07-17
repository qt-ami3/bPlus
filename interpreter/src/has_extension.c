#include "../include/has_extension.h"

bool has_extension(const std::string& filename, const std::string& ext) {
  if (filename.size() < ext.size()) return false;
  return filename.compare(filename.size() - ext.size(), ext.size(), ext) == 0;
}
