#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>

#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"
#include "error.h"
#include "utils.h"

namespace fs = std::filesystem;

// Print usage information
void printUsage(const char* programName) {
  std::cout << "Usage: " << programName << " [options] input.c -o output.coil\n"
            << "Options:\n"
            << "  -o <file>     Specify output file (default: a.coil)\n"
            << "  -O<level>     Optimization level (0-3)\n"
            << "  -I <dir>      Add include directory\n"
            << "  -D <name>[=value] Define macro\n"
            << "  -v            Verbose output\n"
            << "  -h, --help    Display help\n";
}

int main(int argc, char* argv[]) {
  // Default values
  std::string inputFile;
  std::string outputFile = "a.coil";
  std::vector<std::string> includeDirs;
  std::vector<std::string> defines;
  int optimizationLevel = 0;
  bool verbose = false;

  // Parse command line arguments
  for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      
      if (arg == "-h" || arg == "--help") {
          printUsage(argv[0]);
          return 0;
      } else if (arg == "-v") {
          verbose = true;
      } else if (arg == "-o" && i + 1 < argc) {
          outputFile = argv[++i];
      } else if (arg.substr(0, 2) == "-O") {
          optimizationLevel = std::stoi(arg.substr(2));
      } else if (arg == "-I" && i + 1 < argc) {
          includeDirs.push_back(argv[++i]);
      } else if (arg == "-D" && i + 1 < argc) {
          defines.push_back(argv[++i]);
      } else if (arg[0] == '-') {
          std::cerr << "Unknown option: " << arg << std::endl;
          printUsage(argv[0]);
          return 1;
      } else {
          inputFile = arg;
      }
  }

  // Check if input file is provided
  if (inputFile.empty()) {
      std::cerr << "Error: No input file specified\n";
      printUsage(argv[0]);
      return 1;
  }

  // Check if input file exists
  if (!fs::exists(inputFile)) {
      std::cerr << "Error: Input file '" << inputFile << "' does not exist\n";
      return 1;
  }

  try {
      // Read input file
      if (verbose) {
          std::cout << "Reading file: " << inputFile << std::endl;
      }
      
      std::string sourceCode = ccc::readFile(inputFile);
      
      // Initialize error handler
      ccc::ErrorHandler errorHandler;
      
      // Lexical analysis
      if (verbose) {
          std::cout << "Performing lexical analysis...\n";
      }
      
      ccc::Lexer lexer(sourceCode, inputFile, errorHandler);
      std::vector<ccc::Token> tokens = lexer.tokenize();
      
      if (errorHandler.hasErrors()) {
          errorHandler.printErrors();
          return 1;
      }
      
      // Syntax analysis
      if (verbose) {
          std::cout << "Performing syntax analysis...\n";
      }
      
      ccc::Parser parser(tokens, errorHandler);
      std::unique_ptr<ccc::ASTNode> ast = parser.parse();
      
      if (errorHandler.hasErrors()) {
          errorHandler.printErrors();
          return 1;
      }
      
      // Semantic analysis
      if (verbose) {
          std::cout << "Performing semantic analysis...\n";
      }
      
      ccc::SemanticAnalyzer semanticAnalyzer(errorHandler);
      semanticAnalyzer.analyze(ast.get());
      
      if (errorHandler.hasErrors()) {
          errorHandler.printErrors();
          return 1;
      }
      
      // Code generation
      if (verbose) {
          std::cout << "Generating COIL code...\n";
      }
      
      ccc::CodeGenerator codeGen(optimizationLevel, errorHandler);
      coil::CoilObject coilObject = codeGen.generate(ast.get());
      
      if (errorHandler.hasErrors()) {
          errorHandler.printErrors();
          return 1;
      }
      
      // Write output file
      if (verbose) {
          std::cout << "Writing output to: " << outputFile << std::endl;
      }
      
      std::vector<uint8_t> binaryData = coilObject.encode();
      ccc::writeFile(outputFile, binaryData);
      
      if (verbose) {
          std::cout << "Compilation successful: " << inputFile << " -> " << outputFile << std::endl;
      }
      
      return 0;
  } catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      return 1;
  }
}