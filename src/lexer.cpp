#include "lexer.h"
#include <cctype>

namespace ccc {

Lexer::Lexer(const std::string& source, const std::string& filename, ErrorHandler& errorHandler)
  : source(source), filename(filename), errorHandler(errorHandler) {
}

std::vector<Token> Lexer::tokenize() {
  std::vector<Token> tokens;
  
  // Reset state
  start = 0;
  current = 0;
  line = 1;
  column = 1;
  
  // Scan tokens until end of file
  while (!isAtEnd()) {
      start = current;
      scanToken(tokens);
  }
  
  // Add EOF token
  tokens.push_back(Token(TokenType::END_OF_FILE, "", filename, line, column));
  
  return tokens;
}

char Lexer::advance() {
  column++;
  return source[current++];
}

char Lexer::peek() const {
  if (isAtEnd()) return '\0';
  return source[current];
}

char Lexer::peekNext() const {
  if (current + 1 >= source.length()) return '\0';
  return source[current + 1];
}

bool Lexer::match(char expected) {
  if (isAtEnd() || source[current] != expected) return false;
  
  current++;
  column++;
  return true;
}

void Lexer::skipWhitespace() {
  while (true) {
      char c = peek();
      switch (c) {
          case ' ':
          case '\t':
          case '\r':
              advance();
              break;
          case '\n':
              line++;
              column = 1;
              advance();
              break;
          case '/':
              if (peekNext() == '/' || peekNext() == '*') {
                  comment();
              } else {
                  return;
              }
              break;
          default:
              return;
      }
  }
}

bool Lexer::isAtEnd() const {
  return current >= source.length();
}

Token Lexer::makeToken(TokenType type) const {
  std::string lexeme = source.substr(start, current - start);
  return Token(type, lexeme, filename, line, column - lexeme.length());
}

void Lexer::addToken(std::vector<Token>& tokens, TokenType type) {
  tokens.push_back(makeToken(type));
}

Token Lexer::errorToken(const std::string& message) const {
  return Token(TokenType::UNKNOWN, message, filename, line, column);
}

void Lexer::scanToken(std::vector<Token>& tokens) {
  skipWhitespace();
  
  if (isAtEnd()) return;
  
  char c = advance();
  
  // Check for identifiers
  if (std::isalpha(c) || c == '_') {
      current--; // Move back to start of identifier
      column--;
      identifier(tokens);
      return;
  }
  
  // Check for numbers
  if (std::isdigit(c)) {
      current--; // Move back to start of number
      column--;
      number(tokens);
      return;
  }
  
  // Single character tokens
  switch (c) {
      case '(': addToken(tokens, TokenType::LEFT_PAREN); break;
      case ')': addToken(tokens, TokenType::RIGHT_PAREN); break;
      case '{': addToken(tokens, TokenType::LEFT_BRACE); break;
      case '}': addToken(tokens, TokenType::RIGHT_BRACE); break;
      case '[': addToken(tokens, TokenType::LEFT_BRACKET); break;
      case ']': addToken(tokens, TokenType::RIGHT_BRACKET); break;
      case ';': addToken(tokens, TokenType::SEMICOLON); break;
      case ':': addToken(tokens, TokenType::COLON); break;
      case ',': addToken(tokens, TokenType::COMMA); break;
      case '.': 
          // Check for ellipsis
          if (peek() == '.' && peekNext() == '.') {
              advance(); // Consume second '.'
              advance(); // Consume third '.'
              addToken(tokens, TokenType::ELLIPSIS);
          } else {
              addToken(tokens, TokenType::OP_DOT);
          }
          break;
      case '~': addToken(tokens, TokenType::OP_TILDE); break;
      case '?': addToken(tokens, TokenType::OP_QUESTION); break;
      case '#': addToken(tokens, TokenType::HASH); break;
      
      // Operators that could be part of multi-character operators
      case '+':
          if (match('+')) {
              addToken(tokens, TokenType::OP_PLUS_PLUS);
          } else if (match('=')) {
              addToken(tokens, TokenType::OP_PLUS_EQUALS);
          } else {
              addToken(tokens, TokenType::OP_PLUS);
          }
          break;
      case '-':
          if (match('>')) {
              addToken(tokens, TokenType::OP_ARROW);
          } else if (match('-')) {
              addToken(tokens, TokenType::OP_MINUS_MINUS);
          } else if (match('=')) {
              addToken(tokens, TokenType::OP_MINUS_EQUALS);
          } else {
              addToken(tokens, TokenType::OP_MINUS);
          }
          break;
      case '*':
          if (match('=')) {
              addToken(tokens, TokenType::OP_STAR_EQUALS);
          } else {
              addToken(tokens, TokenType::OP_STAR);
          }
          break;
      case '/':
          if (match('=')) {
              addToken(tokens, TokenType::OP_SLASH_EQUALS);
          } else {
              addToken(tokens, TokenType::OP_SLASH);
          }
          break;
      case '%':
          if (match('=')) {
              addToken(tokens, TokenType::OP_PERCENT_EQUALS);
          } else {
              addToken(tokens, TokenType::OP_PERCENT);
          }
          break;
      case '&':
          if (match('&')) {
              addToken(tokens, TokenType::OP_LOGICAL_AND);
          } else if (match('=')) {
              addToken(tokens, TokenType::OP_AND_EQUALS);
          } else {
              addToken(tokens, TokenType::OP_AMPERSAND);
          }
          break;
      case '|':
          if (match('|')) {
              addToken(tokens, TokenType::OP_LOGICAL_OR);
          } else if (match('=')) {
              addToken(tokens, TokenType::OP_OR_EQUALS);
          } else {
              addToken(tokens, TokenType::OP_PIPE);
          }
          break;
      case '^':
          if (match('=')) {
              addToken(tokens, TokenType::OP_XOR_EQUALS);
          } else {
              addToken(tokens, TokenType::OP_CARET);
          }
          break;
      case '!':
          if (match('=')) {
              addToken(tokens, TokenType::OP_NOT_EQUALS);
          } else {
              addToken(tokens, TokenType::OP_EXCLAMATION);
          }
          break;
      case '=':
          if (match('=')) {
              addToken(tokens, TokenType::OP_EQUALS_EQUALS);
          } else {
              addToken(tokens, TokenType::OP_EQUALS);
          }
          break;
      case '<':
          if (match('=')) {
              addToken(tokens, TokenType::OP_LESS_EQUALS);
          } else if (match('<')) {
              if (match('=')) {
                  addToken(tokens, TokenType::OP_SHL_EQUALS);
              } else {
                  addToken(tokens, TokenType::OP_SHL);
              }
          } else {
              addToken(tokens, TokenType::OP_LESS);
          }
          break;
      case '>':
          if (match('=')) {
              addToken(tokens, TokenType::OP_GREATER_EQUALS);
          } else if (match('>')) {
              if (match('=')) {
                  addToken(tokens, TokenType::OP_SHR_EQUALS);
              } else {
                  addToken(tokens, TokenType::OP_SHR);
              }
          } else {
              addToken(tokens, TokenType::OP_GREATER);
          }
          break;
          
      // String and character literals
      case '"':
          string(tokens);
          break;
      case '\'':
          character(tokens);
          break;
          
      // Unknown character
      default:
          errorHandler.error(line, column, "Unexpected character: " + std::string(1, c));
          break;
  }
}

void Lexer::identifier(std::vector<Token>& tokens) {
  while (std::isalnum(peek()) || peek() == '_') {
      advance();
  }
  
  std::string text = source.substr(start, current - start);
  
  // Check if the identifier is a keyword
  auto it = Keywords.find(text);
  if (it != Keywords.end()) {
      addToken(tokens, it->second);
  } else {
      addToken(tokens, TokenType::IDENTIFIER);
  }
}

void Lexer::number(std::vector<Token>& tokens) {
  bool isFloat = false;
  
  // Consume digits
  while (std::isdigit(peek())) {
      advance();
  }
  
  // Check for fractional part
  if (peek() == '.' && std::isdigit(peekNext())) {
      isFloat = true;
      
      // Consume the '.'
      advance();
      
      // Consume digits after decimal point
      while (std::isdigit(peek())) {
          advance();
      }
  }
  
  // Check for exponent
  if (peek() == 'e' || peek() == 'E') {
      isFloat = true;
      
      // Consume the 'e' or 'E'
      advance();
      
      // Consume optional sign
      if (peek() == '+' || peek() == '-') {
          advance();
      }
      
      // Must have at least one digit in exponent
      if (!std::isdigit(peek())) {
          errorHandler.error(line, column, "Invalid floating point number: exponent has no digits");
          return;
      }
      
      // Consume exponent digits
      while (std::isdigit(peek())) {
          advance();
      }
  }
  
  // Check for suffixes (f, F, l, L, u, U, etc.)
  if (peek() == 'f' || peek() == 'F') {
      isFloat = true;
      advance();
  } else if (peek() == 'l' || peek() == 'L') {
      advance();
      if (peek() == 'u' || peek() == 'U') {
          advance();
      }
  } else if (peek() == 'u' || peek() == 'U') {
      advance();
      if (peek() == 'l' || peek() == 'L') {
          advance();
      }
  }
  
  addToken(tokens, isFloat ? TokenType::FLOAT_LITERAL : TokenType::INTEGER_LITERAL);
}

void Lexer::string(std::vector<Token>& tokens) {
  // Capture the starting position for error reporting
  int startLine = line;
  int startColumn = column - 1; // -1 because we already consumed the opening quote
  
  // Consume characters until we hit the closing quote or end of file
  while (peek() != '"' && !isAtEnd()) {
      if (peek() == '\n') {
          line++;
          column = 1;
      }
      
      // Handle escape sequences
      if (peek() == '\\') {
          advance(); // Consume the backslash
          if (isAtEnd()) {
              errorHandler.error(startLine, startColumn, "Unterminated string literal: expected escape sequence");
              break;
          }
          
          // Skip the escaped character
          advance();
      } else {
          advance();
      }
  }
  
  // Check if we ran out of input before finding the closing quote
  if (isAtEnd()) {
      errorHandler.error(startLine, startColumn, "Unterminated string literal");
      return;
  }
  
  // Consume the closing quote
  advance();
  
  // Add the string token (including the quotes)
  addToken(tokens, TokenType::STRING_LITERAL);
}

void Lexer::character(std::vector<Token>& tokens) {
  // Capture the starting position for error reporting
  int startLine = line;
  int startColumn = column - 1; // -1 because we already consumed the opening quote
  
  // Handle escape sequences
  if (peek() == '\\') {
      advance(); // Consume the backslash
      if (isAtEnd()) {
          errorHandler.error(startLine, startColumn, "Unterminated character literal: expected escape sequence");
          return;
      }
      
      // Skip the escaped character
      advance();
  } else if (peek() != '\'' && !isAtEnd()) {
      advance();
  } else {
      errorHandler.error(startLine, startColumn, "Empty character literal");
      if (peek() == '\'') advance(); // Consume the closing quote if present
      return;
  }
  
  // Check for closing quote
  if (peek() != '\'') {
      errorHandler.error(startLine, startColumn, "Multi-character character literal or missing closing quote");
      // Try to recover by finding the next quote
      while (peek() != '\'' && !isAtEnd()) {
          advance();
      }
  }
  
  // Consume the closing quote if we found it
  if (peek() == '\'') {
      advance();
      addToken(tokens, TokenType::CHAR_LITERAL);
  } else {
      errorHandler.error(startLine, startColumn, "Unterminated character literal");
  }
}

void Lexer::comment() {
  if (peek() == '/' && peekNext() == '/') {
      // Line comment
      advance(); // Consume the first '/'
      advance(); // Consume the second '/'
      
      // Consume until end of line or end of file
      while (peek() != '\n' && !isAtEnd()) {
          advance();
      }
  } else if (peek() == '/' && peekNext() == '*') {
      // Block comment
      advance(); // Consume the '/'
      advance(); // Consume the '*'
      
      // Capture the starting position for error reporting
      int startLine = line;
      int startColumn = column - 2; // -2 because we already consumed /* 
      
      // Consume until closing */ or end of file
      while (!isAtEnd()) {
          if (peek() == '*' && peekNext() == '/') {
              advance(); // Consume the '*'
              advance(); // Consume the '/'
              return;
          }
          
          // Handle newlines in comments
          if (peek() == '\n') {
              line++;
              column = 1;
          }
          
          advance();
      }
      
      // If we get here, we ran out of input before finding the closing */
      errorHandler.error(startLine, startColumn, "Unterminated block comment");
  }
}

char Lexer::processEscapeSequence() {
  char c = advance();
  switch (c) {
      case 'a': return '\a';
      case 'b': return '\b';
      case 'f': return '\f';
      case 'n': return '\n';
      case 'r': return '\r';
      case 't': return '\t';
      case 'v': return '\v';
      case '\\': return '\\';
      case '\'': return '\'';
      case '"': return '"';
      case '?': return '\?';
      case '0': return '\0';
      
      // TODO: Handle octal and hex escape sequences
      
      default:
          errorHandler.error(line, column, "Unknown escape sequence: \\" + std::string(1, c));
          return c;
  }
}

} // namespace ccc