#ifndef CCC_ERROR_H
#define CCC_ERROR_H

#include <string>
#include <vector>
#include <iostream>

namespace ccc {

// Error severity levels
enum class ErrorLevel {
  INFO,
  WARNING,
  ERROR
};

// Error entry structure
struct ErrorEntry {
  ErrorLevel level;
  std::string message;
  std::string filename;
  int line;
  int column;
  
  ErrorEntry(ErrorLevel level, const std::string& message, 
              const std::string& filename, int line, int column)
      : level(level), message(message), filename(filename), line(line), column(column) {
  }
  
  // Format error message
  std::string toString() const {
      std::string levelStr;
      switch (level) {
          case ErrorLevel::INFO:    levelStr = "info"; break;
          case ErrorLevel::WARNING: levelStr = "warning"; break;
          case ErrorLevel::ERROR:   levelStr = "error"; break;
      }
      
      return filename + ":" + std::to_string(line) + ":" + std::to_string(column) + 
              ": " + levelStr + ": " + message;
  }
};

// Error handler class
class ErrorHandler {
public:
  // Add errors at different severity levels
  void info(int line, int column, const std::string& message, const std::string& filename = "") {
      addError(ErrorLevel::INFO, message, filename, line, column);
  }
  
  void warning(int line, int column, const std::string& message, const std::string& filename = "") {
      addError(ErrorLevel::WARNING, message, filename, line, column);
  }
  
  void error(int line, int column, const std::string& message, const std::string& filename = "") {
      addError(ErrorLevel::ERROR, message, filename, line, column);
      hadError = true;
  }
  
  // Add errors with associated source location
  void addError(ErrorLevel level, const std::string& message, 
                const std::string& filename, int line, int column) {
      errors.emplace_back(level, message, filename.empty() ? currentFilename : filename, line, column);
      
      if (level == ErrorLevel::ERROR) {
          hadError = true;
      }
  }
  
  // Set the current filename (for convenience when handling multiple files)
  void setCurrentFilename(const std::string& filename) {
      currentFilename = filename;
  }
  
  // Print all errors to stderr
  void printErrors() const {
      for (const auto& error : errors) {
          std::cerr << error.toString() << std::endl;
      }
  }
  
  // Check if there were any errors
  bool hasErrors() const {
      return hadError;
  }
  
  // Check if there were any warnings
  bool hasWarnings() const {
      for (const auto& error : errors) {
          if (error.level == ErrorLevel::WARNING) {
              return true;
          }
      }
      return false;
  }
  
  // Get the number of errors
  size_t errorCount() const {
      size_t count = 0;
      for (const auto& error : errors) {
          if (error.level == ErrorLevel::ERROR) {
              count++;
          }
      }
      return count;
  }
  
  // Get the number of warnings
  size_t warningCount() const {
      size_t count = 0;
      for (const auto& error : errors) {
          if (error.level == ErrorLevel::WARNING) {
              count++;
          }
      }
      return count;
  }
  
  // Clear all errors
  void clear() {
      errors.clear();
      hadError = false;
  }

private:
  std::vector<ErrorEntry> errors;
  std::string currentFilename;
  bool hadError = false;
};

} // namespace ccc

#endif // CCC_ERROR_H