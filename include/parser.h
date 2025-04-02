#ifndef CCC_PARSER_H
#define CCC_PARSER_H

#include <vector>
#include <memory>
#include <string>
#include "token.h"
#include "ast.h"
#include "error.h"

namespace ccc {

class Parser {
public:
  // Constructor
  Parser(const std::vector<Token>& tokens, ErrorHandler& errorHandler);
  
  // Parse the tokens into an AST
  std::unique_ptr<ASTNode> parse();

private:
  // Token access
  const std::vector<Token>& tokens;
  ErrorHandler& errorHandler;
  size_t current = 0;
  
  // Helper methods for parsing
  bool isAtEnd() const;
  const Token& peek() const;
  const Token& previous() const;
  const Token& advance();
  bool check(TokenType type) const;
  bool match(TokenType type);
  bool match(std::initializer_list<TokenType> types);
  void consume(TokenType type, const std::string& message);
  bool isTypeSpecifier(const Token& token) const;
  
  // Error handling
  void synchronize();
  Token errorToken(const std::string& message) const;
  
  // Parsing rules (recursive descent)
  std::unique_ptr<ProgramNode> program();
  std::unique_ptr<ASTNode> declaration();
  std::unique_ptr<FunctionDeclarationNode> functionDeclaration();
  std::unique_ptr<VariableDeclarationNode> variableDeclaration();
  std::unique_ptr<TypeNode> typeSpecifier();
  std::unique_ptr<ParameterNode> parameter();
  std::vector<std::unique_ptr<ParameterNode>> parameterList();
  std::unique_ptr<BlockNode> block();
  std::unique_ptr<StatementNode> statement();
  std::unique_ptr<StatementNode> expressionStatement();
  std::unique_ptr<StatementNode> ifStatement();
  std::unique_ptr<StatementNode> whileStatement();
  std::unique_ptr<StatementNode> doWhileStatement();
  std::unique_ptr<StatementNode> forStatement();
  std::unique_ptr<StatementNode> returnStatement();
  std::unique_ptr<StatementNode> breakStatement();
  std::unique_ptr<StatementNode> continueStatement();
  
  // Expression parsing
  std::unique_ptr<ExpressionNode> expression();
  std::unique_ptr<ExpressionNode> assignment();
  std::unique_ptr<ExpressionNode> conditionalExpr();
  std::unique_ptr<ExpressionNode> logicalOr();
  std::unique_ptr<ExpressionNode> logicalAnd();
  std::unique_ptr<ExpressionNode> bitwiseOr();
  std::unique_ptr<ExpressionNode> bitwiseXor();
  std::unique_ptr<ExpressionNode> bitwiseAnd();
  std::unique_ptr<ExpressionNode> equality();
  std::unique_ptr<ExpressionNode> comparison();
  std::unique_ptr<ExpressionNode> shift();
  std::unique_ptr<ExpressionNode> term();
  std::unique_ptr<ExpressionNode> factor();
  std::unique_ptr<ExpressionNode> unary();
  std::unique_ptr<ExpressionNode> postfix();
  std::unique_ptr<ExpressionNode> primary();
};

} // namespace ccc

#endif // CCC_PARSER_H