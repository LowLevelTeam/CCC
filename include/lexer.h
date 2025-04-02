#ifndef CCC_LEXER_H
#define CCC_LEXER_H

#include <string>
#include <vector>
#include "token.h"
#include "error.h"

namespace ccc {

class Lexer {
public:
  // Constructor
  Lexer(const std::string& source, const std::string& filename, ErrorHandler& errorHandler);
  
  // Tokenize the source code
  std::vector<Token> tokenize();

private:
  // Source code and scanning state
  std::string source;
  std::string filename;
  ErrorHandler& errorHandler;
  int start = 0;
  int current = 0;
  int line = 1;
  int column = 1;
  
  // Lexer operations
  char advance();
  char peek() const;
  char peekNext() const;
  bool match(char expected);
  void skipWhitespace();
  bool isAtEnd() const;
  
  // Token creation
  Token makeToken(TokenType type) const;
  void addToken(std::vector<Token>& tokens, TokenType type);
  Token errorToken(const std::string& message) const;
  
  // Token scanning
  void scanToken(std::vector<Token>& tokens);
  void identifier(std::vector<Token>& tokens);
  void number(std::vector<Token>& tokens);
  void string(std::vector<Token>& tokens);
  void character(std::vector<Token>& tokens);
  void comment();
  
  // Helper for character literals
  char processEscapeSequence();
};

} // namespace ccc

#endif // CCC_LEXER_H