#include "token.h"
#include <sstream>

namespace ccc {

// Constructor
Token::Token(TokenType type, const std::string& lexeme, const std::string& filename, int line, int column)
  : type(type), lexeme(lexeme), filename(filename), line(line), column(column) {
}

// Get a string representation of the token type
std::string Token::getTypeName() const {
  switch (type) {
      case TokenType::END_OF_FILE: return "EOF";
      case TokenType::UNKNOWN: return "UNKNOWN";
      
      // Literals
      case TokenType::IDENTIFIER: return "IDENTIFIER";
      case TokenType::INTEGER_LITERAL: return "INTEGER";
      case TokenType::FLOAT_LITERAL: return "FLOAT";
      case TokenType::STRING_LITERAL: return "STRING";
      case TokenType::CHAR_LITERAL: return "CHAR";
      
      // Keywords
      case TokenType::KW_AUTO: return "auto";
      case TokenType::KW_BREAK: return "break";
      case TokenType::KW_CASE: return "case";
      case TokenType::KW_CHAR: return "char";
      case TokenType::KW_CONST: return "const";
      case TokenType::KW_CONTINUE: return "continue";
      case TokenType::KW_DEFAULT: return "default";
      case TokenType::KW_DO: return "do";
      case TokenType::KW_DOUBLE: return "double";
      case TokenType::KW_ELSE: return "else";
      case TokenType::KW_ENUM: return "enum";
      case TokenType::KW_EXTERN: return "extern";
      case TokenType::KW_FLOAT: return "float";
      case TokenType::KW_FOR: return "for";
      case TokenType::KW_GOTO: return "goto";
      case TokenType::KW_IF: return "if";
      case TokenType::KW_INT: return "int";
      case TokenType::KW_LONG: return "long";
      case TokenType::KW_REGISTER: return "register";
      case TokenType::KW_RETURN: return "return";
      case TokenType::KW_SHORT: return "short";
      case TokenType::KW_SIGNED: return "signed";
      case TokenType::KW_SIZEOF: return "sizeof";
      case TokenType::KW_STATIC: return "static";
      case TokenType::KW_STRUCT: return "struct";
      case TokenType::KW_SWITCH: return "switch";
      case TokenType::KW_TYPEDEF: return "typedef";
      case TokenType::KW_UNION: return "union";
      case TokenType::KW_UNSIGNED: return "unsigned";
      case TokenType::KW_VOID: return "void";
      case TokenType::KW_VOLATILE: return "volatile";
      case TokenType::KW_WHILE: return "while";
      
      // Operators and punctuation
      case TokenType::OP_PLUS: return "+";
      case TokenType::OP_MINUS: return "-";
      case TokenType::OP_STAR: return "*";
      case TokenType::OP_SLASH: return "/";
      case TokenType::OP_PERCENT: return "%";
      case TokenType::OP_AMPERSAND: return "&";
      case TokenType::OP_PIPE: return "|";
      case TokenType::OP_CARET: return "^";
      case TokenType::OP_TILDE: return "~";
      case TokenType::OP_EXCLAMATION: return "!";
      case TokenType::OP_EQUALS: return "=";
      case TokenType::OP_LESS: return "<";
      case TokenType::OP_GREATER: return ">";
      case TokenType::OP_DOT: return ".";
      case TokenType::OP_ARROW: return "->";
      case TokenType::OP_PLUS_PLUS: return "++";
      case TokenType::OP_MINUS_MINUS: return "--";
      case TokenType::OP_PLUS_EQUALS: return "+=";
      case TokenType::OP_MINUS_EQUALS: return "-=";
      case TokenType::OP_STAR_EQUALS: return "*=";
      case TokenType::OP_SLASH_EQUALS: return "/=";
      case TokenType::OP_PERCENT_EQUALS: return "%=";
      case TokenType::OP_AND_EQUALS: return "&=";
      case TokenType::OP_OR_EQUALS: return "|=";
      case TokenType::OP_XOR_EQUALS: return "^=";
      case TokenType::OP_SHL_EQUALS: return "<<=";
      case TokenType::OP_SHR_EQUALS: return ">>=";
      case TokenType::OP_EQUALS_EQUALS: return "==";
      case TokenType::OP_NOT_EQUALS: return "!=";
      case TokenType::OP_LESS_EQUALS: return "<=";
      case TokenType::OP_GREATER_EQUALS: return ">=";
      case TokenType::OP_SHL: return "<<";
      case TokenType::OP_SHR: return ">>";
      case TokenType::OP_LOGICAL_AND: return "&&";
      case TokenType::OP_LOGICAL_OR: return "||";
      case TokenType::OP_QUESTION: return "?";
      case TokenType::SEMICOLON: return ";";
      case TokenType::COLON: return ":";
      case TokenType::COMMA: return ",";
      case TokenType::LEFT_PAREN: return "(";
      case TokenType::RIGHT_PAREN: return ")";
      case TokenType::LEFT_BRACE: return "{";
      case TokenType::RIGHT_BRACE: return "}";
      case TokenType::LEFT_BRACKET: return "[";
      case TokenType::RIGHT_BRACKET: return "]";
      case TokenType::HASH: return "#";
      case TokenType::ELLIPSIS: return "...";
      
      default: return "UNKNOWN_TOKEN";
  }
}

// Debug string representation
std::string Token::toString() const {
  std::stringstream ss;
  ss << "[" << getTypeName() << "] '" << lexeme << "' at " 
      << filename << ":" << line << ":" << column;
  return ss.str();
}

// Keyword mapping
const std::unordered_map<std::string, TokenType> Keywords = {
  {"auto", TokenType::KW_AUTO},
  {"break", TokenType::KW_BREAK},
  {"case", TokenType::KW_CASE},
  {"char", TokenType::KW_CHAR},
  {"const", TokenType::KW_CONST},
  {"continue", TokenType::KW_CONTINUE},
  {"default", TokenType::KW_DEFAULT},
  {"do", TokenType::KW_DO},
  {"double", TokenType::KW_DOUBLE},
  {"else", TokenType::KW_ELSE},
  {"enum", TokenType::KW_ENUM},
  {"extern", TokenType::KW_EXTERN},
  {"float", TokenType::KW_FLOAT},
  {"for", TokenType::KW_FOR},
  {"goto", TokenType::KW_GOTO},
  {"if", TokenType::KW_IF},
  {"int", TokenType::KW_INT},
  {"long", TokenType::KW_LONG},
  {"register", TokenType::KW_REGISTER},
  {"return", TokenType::KW_RETURN},
  {"short", TokenType::KW_SHORT},
  {"signed", TokenType::KW_SIGNED},
  {"sizeof", TokenType::KW_SIZEOF},
  {"static", TokenType::KW_STATIC},
  {"struct", TokenType::KW_STRUCT},
  {"switch", TokenType::KW_SWITCH},
  {"typedef", TokenType::KW_TYPEDEF},
  {"union", TokenType::KW_UNION},
  {"unsigned", TokenType::KW_UNSIGNED},
  {"void", TokenType::KW_VOID},
  {"volatile", TokenType::KW_VOLATILE},
  {"while", TokenType::KW_WHILE}
};

} // namespace ccc