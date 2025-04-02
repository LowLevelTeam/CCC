#ifndef CCC_TOKEN_H
#define CCC_TOKEN_H

#include <string>
#include <unordered_map>

namespace ccc {

// Token types
enum class TokenType {
  // Special tokens
  END_OF_FILE,
  UNKNOWN,
  
  // Literals
  IDENTIFIER,
  INTEGER_LITERAL,
  FLOAT_LITERAL,
  STRING_LITERAL,
  CHAR_LITERAL,
  
  // Keywords
  KW_AUTO,
  KW_BREAK,
  KW_CASE,
  KW_CHAR,
  KW_CONST,
  KW_CONTINUE,
  KW_DEFAULT,
  KW_DO,
  KW_DOUBLE,
  KW_ELSE,
  KW_ENUM,
  KW_EXTERN,
  KW_FLOAT,
  KW_FOR,
  KW_GOTO,
  KW_IF,
  KW_INT,
  KW_LONG,
  KW_REGISTER,
  KW_RETURN,
  KW_SHORT,
  KW_SIGNED,
  KW_SIZEOF,
  KW_STATIC,
  KW_STRUCT,
  KW_SWITCH,
  KW_TYPEDEF,
  KW_UNION,
  KW_UNSIGNED,
  KW_VOID,
  KW_VOLATILE,
  KW_WHILE,
  
  // Operators
  OP_PLUS,           // +
  OP_MINUS,          // -
  OP_STAR,           // *
  OP_SLASH,          // /
  OP_PERCENT,        // %
  OP_AMPERSAND,      // &
  OP_PIPE,           // |
  OP_CARET,          // ^
  OP_TILDE,          // ~
  OP_EXCLAMATION,    // !
  OP_EQUALS,         // =
  OP_LESS,           // <
  OP_GREATER,        // >
  OP_DOT,            // .
  OP_ARROW,          // ->
  OP_PLUS_PLUS,      // ++
  OP_MINUS_MINUS,    // --
  OP_PLUS_EQUALS,    // +=
  OP_MINUS_EQUALS,   // -=
  OP_STAR_EQUALS,    // *=
  OP_SLASH_EQUALS,   // /=
  OP_PERCENT_EQUALS, // %=
  OP_AND_EQUALS,     // &=
  OP_OR_EQUALS,      // |=
  OP_XOR_EQUALS,     // ^=
  OP_SHL_EQUALS,     // <<=
  OP_SHR_EQUALS,     // >>=
  OP_EQUALS_EQUALS,  // ==
  OP_NOT_EQUALS,     // !=
  OP_LESS_EQUALS,    // <=
  OP_GREATER_EQUALS, // >=
  OP_SHL,            // <<
  OP_SHR,            // >>
  OP_LOGICAL_AND,    // &&
  OP_LOGICAL_OR,     // ||
  OP_QUESTION,       // ?
  
  // Punctuation
  SEMICOLON,         // ;
  COLON,             // :
  COMMA,             // ,
  LEFT_PAREN,        // (
  RIGHT_PAREN,       // )
  LEFT_BRACE,        // {
  RIGHT_BRACE,       // }
  LEFT_BRACKET,      // [
  RIGHT_BRACKET,     // ]
  HASH,              // #
  ELLIPSIS           // ...
};

// Token structure
struct Token {
  TokenType type;
  std::string lexeme;
  std::string filename;
  int line;
  int column;
  
  // Constructor
  Token(TokenType type, const std::string& lexeme, const std::string& filename, int line, int column);
  
  // Get a string representation of the token type
  std::string getTypeName() const;
  
  // For debugging
  std::string toString() const;
};

// Keyword map
extern const std::unordered_map<std::string, TokenType> Keywords;

} // namespace ccc

#endif // CCC_TOKEN_H