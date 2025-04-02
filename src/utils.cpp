#include "utils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

namespace ccc {

std::string readFile(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
      throw std::runtime_error("Failed to open file: " + path);
  }
  
  // Read the entire file into a string
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

void writeFile(const std::string& path, const std::vector<uint8_t>& data) {
  std::ofstream file(path, std::ios::binary);
  if (!file) {
      throw std::runtime_error("Failed to open file for writing: " + path);
  }
  
  file.write(reinterpret_cast<const char*>(data.data()), data.size());
  
  if (!file) {
      throw std::runtime_error("Failed to write to file: " + path);
  }
}

std::vector<std::string> splitString(const std::string& str, char delimiter) {
  std::vector<std::string> result;
  std::stringstream ss(str);
  std::string item;
  
  while (std::getline(ss, item, delimiter)) {
      result.push_back(item);
  }
  
  return result;
}

std::string trimString(const std::string& str) {
  auto start = str.begin();
  auto end = str.end();
  
  // Trim leading whitespace
  while (start != end && std::isspace(*start)) {
      ++start;
  }
  
  // Trim trailing whitespace
  while (start != end && std::isspace(*(end - 1))) {
      --end;
  }
  
  return std::string(start, end);
}

bool startsWith(const std::string& str, const std::string& prefix) {
  if (prefix.length() > str.length()) {
      return false;
  }
  
  return std::equal(prefix.begin(), prefix.end(), str.begin());
}

bool endsWith(const std::string& str, const std::string& suffix) {
  if (suffix.length() > str.length()) {
      return false;
  }
  
  return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

std::string getFileExtension(const std::string& path) {
  return fs::path(path).extension().string();
}

std::string getFileName(const std::string& path) {
  return fs::path(path).filename().string();
}

std::string getDirectory(const std::string& path) {
  return fs::path(path).parent_path().string();
}

std::string joinPaths(const std::string& path1, const std::string& path2) {
  return (fs::path(path1) / fs::path(path2)).string();
}

} // namespace ccc