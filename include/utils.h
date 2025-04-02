#ifndef CCC_UTILS_H
#define CCC_UTILS_H

#include <string>
#include <vector>
#include <cstdint>

namespace ccc {

// File I/O utilities
std::string readFile(const std::string& path);
void writeFile(const std::string& path, const std::vector<uint8_t>& data);

// String utilities
std::vector<std::string> splitString(const std::string& str, char delimiter);
std::string trimString(const std::string& str);
bool startsWith(const std::string& str, const std::string& prefix);
bool endsWith(const std::string& str, const std::string& suffix);

// Path utilities
std::string getFileExtension(const std::string& path);
std::string getFileName(const std::string& path);
std::string getDirectory(const std::string& path);
std::string joinPaths(const std::string& path1, const std::string& path2);

} // namespace ccc

#endif // CCC_UTILS_H