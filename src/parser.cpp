#include "parser.h"
#include <stdexcept>

namespace ccc {

Parser::Parser(const std::vector<Token>& tokens, ErrorHandler& errorHandler)
    : tokens(tokens), errorHandler(errorHandler) {
}

std::unique_ptr<ASTNode> Parser::parse() {
    try {
        return program();
    } catch (const std::exception& e) {
        errorHandler.error(0, 0, std::string("Parse error: ") + e.what());
        return nullptr;
    }
}

bool Parser::isAtEnd() const {
    return current >= tokens.size() || tokens[current].type == TokenType::END_OF_FILE;
}

const Token& Parser::peek() const {
    return tokens[current];
}

const Token& Parser::previous() const {
    return tokens[current - 1];
}

const Token& Parser::advance() {
    if (!isAtEnd()) {
        current++;
    }
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType type : types) {
        if (match(type)) {
            return true;
        }
    }
    return false;
}

void Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        advance();
        return;
    }
    
    errorHandler.error(peek().line, peek().column, message);
    throw std::runtime_error(message);
}

bool Parser::isTypeSpecifier(const Token& token) const {
    return token.type == TokenType::KW_VOID ||
           token.type == TokenType::KW_CHAR ||
           token.type == TokenType::KW_SHORT ||
           token.type == TokenType::KW_INT ||
           token.type == TokenType::KW_LONG ||
           token.type == TokenType::KW_FLOAT ||
           token.type == TokenType::KW_DOUBLE ||
           token.type == TokenType::KW_SIGNED ||
           token.type == TokenType::KW_UNSIGNED ||
           token.type == TokenType::KW_CONST ||
           token.type == TokenType::KW_VOLATILE;
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;
        
        switch (peek().type) {
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_FOR:
            case TokenType::KW_RETURN:
            case TokenType::KW_BREAK:
            case TokenType::KW_CONTINUE:
            case TokenType::KW_VOID:
            case TokenType::KW_CHAR:
            case TokenType::KW_SHORT:
            case TokenType::KW_INT:
            case TokenType::KW_LONG:
            case TokenType::KW_FLOAT:
            case TokenType::KW_DOUBLE:
                return;
        }
        
        advance();
    }
}

Token Parser::errorToken(const std::string& message) const {
    return Token(TokenType::UNKNOWN, message, peek().filename, peek().line, peek().column);
}

std::unique_ptr<ProgramNode> Parser::program() {
    std::vector<std::unique_ptr<ASTNode>> declarations;
    
    while (!isAtEnd()) {
        try {
            auto decl = declaration();
            if (decl) {
                declarations.push_back(std::move(decl));
            }
        } catch (const std::exception& e) {
            errorHandler.error(peek().line, peek().column, e.what());
            synchronize();
        }
    }
    
    return std::make_unique<ProgramNode>(std::move(declarations));
}

std::unique_ptr<ASTNode> Parser::declaration() {
    // Look ahead to determine if this is a function or variable declaration
    if (isTypeSpecifier(peek())) {
        // Parse the type
        size_t startPos = current;
        auto type = typeSpecifier();
        
        // Check if it's a function declaration (name followed by left paren)
        if (check(TokenType::IDENTIFIER) && 
            (current + 1 < tokens.size() && tokens[current + 1].type == TokenType::LEFT_PAREN)) {
            // Reset position and parse as function
            current = startPos;
            return functionDeclaration();
        }
        
        // Reset position and parse as variable
        current = startPos;
        return variableDeclaration();
    }
    
    // Handle preprocessor directives, typedefs, etc. (not implemented)
    errorHandler.error(peek().line, peek().column, "Unsupported declaration");
    synchronize();
    return nullptr;
}

std::unique_ptr<FunctionDeclarationNode> Parser::functionDeclaration() {
    // Parse return type
    auto returnType = typeSpecifier();
    
    // Parse function name
    const Token& name = peek();
    consume(TokenType::IDENTIFIER, "Expected function name");
    
    // Parse parameters
    consume(TokenType::LEFT_PAREN, "Expected '(' after function name");
    std::vector<std::unique_ptr<ParameterNode>> parameters;
    
    if (!check(TokenType::RIGHT_PAREN)) {
        parameters = parameterList();
    }
    
    consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters");
    
    // Parse body (or just declaration)
    std::unique_ptr<BlockNode> body = nullptr;
    if (match(TokenType::LEFT_BRACE)) {
        current--; // Move back to the '{' for the block parser
        body = block();
    } else {
        consume(TokenType::SEMICOLON, "Expected ';' after function declaration");
    }
    
    return std::make_unique<FunctionDeclarationNode>(
        std::move(returnType),
        name,
        std::move(parameters),
        std::move(body)
    );
}

std::unique_ptr<VariableDeclarationNode> Parser::variableDeclaration() {
    // Parse type
    auto type = typeSpecifier();
    
    // Parse name
    const Token& name = peek();
    consume(TokenType::IDENTIFIER, "Expected variable name");
    
    // Parse initializer if present
    std::unique_ptr<ExpressionNode> initializer = nullptr;
    if (match(TokenType::OP_EQUALS)) {
        initializer = expression();
    }
    
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    
    return std::make_unique<VariableDeclarationNode>(
        std::move(type),
        name,
        std::move(initializer)
    );
}

std::unique_ptr<TypeNode> Parser::typeSpecifier() {
  bool isConst = false;
  bool isVolatile = false;
  
  // Handle const and volatile qualifiers
  while (match({TokenType::KW_CONST, TokenType::KW_VOLATILE})) {
      if (previous().type == TokenType::KW_CONST) {
          isConst = true;
      } else if (previous().type == TokenType::KW_VOLATILE) {
          isVolatile = true;
      }
  }
  
  // Require a base type
  if (!isTypeSpecifier(peek())) {
      errorHandler.error(peek().line, peek().column, "Expected type specifier");
      throw std::runtime_error("Expected type specifier");
  }
  
  // Parse the base type
  Token baseType = advance();
  
  // Handle pointers
  bool isPointer = false;
  int pointerLevel = 0;
  while (match(TokenType::OP_STAR)) {
      isPointer = true;
      pointerLevel++;
  }
  
  return std::make_unique<TypeNode>(baseType, isConst, isVolatile, isPointer, pointerLevel);
}

std::unique_ptr<ParameterNode> Parser::parameter() {
  // Parse parameter type
  auto type = typeSpecifier();
  
  // Parse parameter name (allow unnamed parameters)
  Token name = {"", TokenType::IDENTIFIER, "", 0, 0};
  if (check(TokenType::IDENTIFIER)) {
      name = advance();
  }
  
  return std::make_unique<ParameterNode>(std::move(type), name);
}

std::vector<std::unique_ptr<ParameterNode>> Parser::parameterList() {
  std::vector<std::unique_ptr<ParameterNode>> parameters;
  
  // Parse first parameter
  parameters.push_back(parameter());
  
  // Parse additional parameters
  while (match(TokenType::COMMA)) {
      parameters.push_back(parameter());
  }
  
  return parameters;
}

std::unique_ptr<BlockNode> Parser::block() {
  consume(TokenType::LEFT_BRACE, "Expected '{' before block");
  
  std::vector<std::unique_ptr<StatementNode>> statements;
  
  while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
      try {
          auto stmt = statement();
          if (stmt) {
              statements.push_back(std::move(stmt));
          }
      } catch (const std::exception& e) {
          errorHandler.error(peek().line, peek().column, e.what());
          synchronize();
      }
  }
  
  consume(TokenType::RIGHT_BRACE, "Expected '}' after block");
  
  return std::make_unique<BlockNode>(std::move(statements));
}

std::unique_ptr<StatementNode> Parser::statement() {
  if (match(TokenType::LEFT_BRACE)) {
      current--; // Move back to the '{' for the block parser
      return block();
  }
  if (match(TokenType::KW_IF)) {
      return ifStatement();
  }
  if (match(TokenType::KW_WHILE)) {
      return whileStatement();
  }
  if (match(TokenType::KW_DO)) {
      return doWhileStatement();
  }
  if (match(TokenType::KW_FOR)) {
      return forStatement();
  }
  if (match(TokenType::KW_RETURN)) {
      return returnStatement();
  }
  if (match(TokenType::KW_BREAK)) {
      return breakStatement();
  }
  if (match(TokenType::KW_CONTINUE)) {
      return continueStatement();
  }
  
  // Check if this is a declaration
  if (isTypeSpecifier(peek())) {
      return variableDeclaration();
  }
  
  return expressionStatement();
}

std::unique_ptr<StatementNode> Parser::expressionStatement() {
  auto expr = expression();
  consume(TokenType::SEMICOLON, "Expected ';' after expression");
  return std::make_unique<ExpressionStatementNode>(std::move(expr));
}

std::unique_ptr<StatementNode> Parser::ifStatement() {
  consume(TokenType::LEFT_PAREN, "Expected '(' after 'if'");
  auto condition = expression();
  consume(TokenType::RIGHT_PAREN, "Expected ')' after if condition");
  
  auto thenBranch = statement();
  std::unique_ptr<StatementNode> elseBranch = nullptr;
  
  if (match(TokenType::KW_ELSE)) {
      elseBranch = statement();
  }
  
  return std::make_unique<IfNode>(
      std::move(condition),
      std::move(thenBranch),
      std::move(elseBranch)
  );
}

std::unique_ptr<StatementNode> Parser::whileStatement() {
  consume(TokenType::LEFT_PAREN, "Expected '(' after 'while'");
  auto condition = expression();
  consume(TokenType::RIGHT_PAREN, "Expected ')' after while condition");
  
  auto body = statement();
  
  return std::make_unique<WhileNode>(
      std::move(condition),
      std::move(body)
  );
}

std::unique_ptr<StatementNode> Parser::doWhileStatement() {
  auto body = statement();
  
  consume(TokenType::KW_WHILE, "Expected 'while' after do body");
  consume(TokenType::LEFT_PAREN, "Expected '(' after 'while'");
  auto condition = expression();
  consume(TokenType::RIGHT_PAREN, "Expected ')' after while condition");
  
  consume(TokenType::SEMICOLON, "Expected ';' after do-while statement");
  
  return std::make_unique<DoWhileNode>(
      std::move(body),
      std::move(condition)
  );
}

std::unique_ptr<StatementNode> Parser::forStatement() {
  consume(TokenType::LEFT_PAREN, "Expected '(' after 'for'");
  
  // Initializer
  std::unique_ptr<StatementNode> initializer = nullptr;
  if (!check(TokenType::SEMICOLON)) {
      if (isTypeSpecifier(peek())) {
          initializer = variableDeclaration();
      } else {
          initializer = expressionStatement();
      }
  } else {
      consume(TokenType::SEMICOLON, "Expected ';'");
  }
  
  // Condition
  std::unique_ptr<ExpressionNode> condition = nullptr;
  if (!check(TokenType::SEMICOLON)) {
      condition = expression();
  }
  consume(TokenType::SEMICOLON, "Expected ';' after for condition");
  
  // Increment
  std::unique_ptr<ExpressionNode> increment = nullptr;
  if (!check(TokenType::RIGHT_PAREN)) {
      increment = expression();
  }
  consume(TokenType::RIGHT_PAREN, "Expected ')' after for clauses");
  
  // Body
  auto body = statement();
  
  return std::make_unique<ForNode>(
      std::move(initializer),
      std::move(condition),
      std::move(increment),
      std::move(body)
  );
}

std::unique_ptr<StatementNode> Parser::returnStatement() {
  std::unique_ptr<ExpressionNode> value = nullptr;
  if (!check(TokenType::SEMICOLON)) {
      value = expression();
  }
  
  consume(TokenType::SEMICOLON, "Expected ';' after return value");
  
  return std::make_unique<ReturnNode>(std::move(value));
}

std::unique_ptr<StatementNode> Parser::breakStatement() {
  consume(TokenType::SEMICOLON, "Expected ';' after 'break'");
  return std::make_unique<BreakNode>();
}

std::unique_ptr<StatementNode> Parser::continueStatement() {
  consume(TokenType::SEMICOLON, "Expected ';' after 'continue'");
  return std::make_unique<ContinueNode>();
}

std::unique_ptr<ExpressionNode> Parser::expression() {
  return assignment();
}

std::unique_ptr<ExpressionNode> Parser::assignment() {
  auto expr = conditionalExpr();
  
  if (match({
      TokenType::OP_EQUALS, 
      TokenType::OP_PLUS_EQUALS, TokenType::OP_MINUS_EQUALS,
      TokenType::OP_STAR_EQUALS, TokenType::OP_SLASH_EQUALS,
      TokenType::OP_PERCENT_EQUALS, TokenType::OP_AND_EQUALS,
      TokenType::OP_OR_EQUALS, TokenType::OP_XOR_EQUALS,
      TokenType::OP_SHL_EQUALS, TokenType::OP_SHR_EQUALS
  })) {
      Token op = previous();
      auto value = assignment();
      
      // Convert += to +, etc.
      TokenType binaryOp;
      switch (op.type) {
          case TokenType::OP_PLUS_EQUALS: binaryOp = TokenType::OP_PLUS; break;
          case TokenType::OP_MINUS_EQUALS: binaryOp = TokenType::OP_MINUS; break;
          case TokenType::OP_STAR_EQUALS: binaryOp = TokenType::OP_STAR; break;
          case TokenType::OP_SLASH_EQUALS: binaryOp = TokenType::OP_SLASH; break;
          case TokenType::OP_PERCENT_EQUALS: binaryOp = TokenType::OP_PERCENT; break;
          case TokenType::OP_AND_EQUALS: binaryOp = TokenType::OP_AMPERSAND; break;
          case TokenType::OP_OR_EQUALS: binaryOp = TokenType::OP_PIPE; break;
          case TokenType::OP_XOR_EQUALS: binaryOp = TokenType::OP_CARET; break;
          case TokenType::OP_SHL_EQUALS: binaryOp = TokenType::OP_SHL; break;
          case TokenType::OP_SHR_EQUALS: binaryOp = TokenType::OP_SHR; break;
          default: binaryOp = TokenType::UNKNOWN; break;
      }
      
      // Create a combined assignment (a += b becomes a = a + b)
      if (binaryOp != TokenType::UNKNOWN) {
          Token binaryToken(binaryOp, op.lexeme.substr(0, op.lexeme.size() - 1), op.filename, op.line, op.column);
          
          auto right = std::make_unique<BinaryNode>(
              std::move(expr),
              binaryToken,
              std::move(value)
          );
          
          // Create a = (a + b)
          Token equalsToken(TokenType::OP_EQUALS, "=", op.filename, op.line, op.column);
          return std::make_unique<BinaryNode>(
              std::move(expr), // Will be cloned in codegen since it's used twice
              equalsToken,
              std::move(right)
          );
      }
      
      return std::make_unique<BinaryNode>(
          std::move(expr),
          op,
          std::move(value)
      );
  }
  
  return expr;
}

std::unique_ptr<ExpressionNode> Parser::conditionalExpr() {
  auto condition = logicalOr();
  
  if (match(TokenType::OP_QUESTION)) {
      auto trueExpr = expression();
      consume(TokenType::COLON, "Expected ':' in conditional expression");
      auto falseExpr = conditionalExpr();
      
      return std::make_unique<ConditionalNode>(
          std::move(condition),
          std::move(trueExpr),
          std::move(falseExpr)
      );
  }
  
  return condition;
}

std::unique_ptr<ExpressionNode> Parser::logicalOr() {
  auto expr = logicalAnd();
  
  while (match(TokenType::OP_LOGICAL_OR)) {
      Token op = previous();
      auto right = logicalAnd();
      expr = std::make_unique<BinaryNode>(
          std::move(expr),
          op,
          std::move(right)
      );
  }
  
  return expr;
}

std::unique_ptr<ExpressionNode> Parser::logicalAnd() {
  auto expr = bitwiseOr();
  
  while (match(TokenType::OP_LOGICAL_AND)) {
      Token op = previous();
      auto right = bitwiseOr();
      expr = std::make_unique<BinaryNode>(
          std::move(expr),
          op,
          std::move(right)
      );
  }
  
  return expr;
}

std::unique_ptr<ExpressionNode> Parser::bitwiseOr() {
  auto expr = bitwiseXor();
  
  while (match(TokenType::OP_PIPE)) {
      Token op = previous();
      auto right = bitwiseXor();
      expr = std::make_unique<BinaryNode>(
          std::move(expr),
          op,
          std::move(right)
      );
  }
  
  return expr;
}

std::unique_ptr<ExpressionNode> Parser::bitwiseXor() {
  auto expr = bitwiseAnd();
  
  while (match(TokenType::OP_CARET)) {
      Token op = previous();
      auto right = bitwiseAnd();
      expr = std::make_unique<BinaryNode>(
          std::move(expr),
          op,
          std::move(right)
      );
  }
  
  return expr;
}

std::unique_ptr<ExpressionNode> Parser::bitwiseAnd() {
  auto expr = equality();
  
  while (match(TokenType::OP_AMPERSAND)) {
      Token op = previous();
      auto right = equality();
      expr = std::make_unique<BinaryNode>(
          std::move(expr),
          op,
          std::move(right)
      );
  }
  
  return expr;
}

std::unique_ptr<ExpressionNode> Parser::equality() {
  auto expr = comparison();
  
  while (match({TokenType::OP_EQUALS_EQUALS, TokenType::OP_NOT_EQUALS})) {
      Token op = previous();
      auto right = comparison();
      expr = std::make_unique<BinaryNode>(
          std::move(expr),
          op,
          std::move(right)
      );
  }
  
  return expr;
}

std::unique_ptr<ExpressionNode> Parser::comparison() {
  auto expr = shift();
  
  while (match({
      TokenType::OP_LESS, TokenType::OP_LESS_EQUALS,
      TokenType::OP_GREATER, TokenType::OP_GREATER_EQUALS
  })) {
      Token op = previous();
      auto right = shift();
      expr = std::make_unique<BinaryNode>(
          std::move(expr),
          op,
          std::move(right)
      );
  }
  
  return expr;
}

std::unique_ptr<ExpressionNode> Parser::shift() {
  auto expr = term();
  
  while (match({TokenType::OP_SHL, TokenType::OP_SHR})) {
      Token op = previous();
      auto right = term();
      expr = std::make_unique<BinaryNode>(
          std::move(expr),
          op,
          std::move(right)
      );
  }
  
  return expr;
}

std::unique_ptr<ExpressionNode> Parser::term() {
  auto expr = factor();
  
  while (match({TokenType::OP_PLUS, TokenType::OP_MINUS})) {
      Token op = previous();
      auto right = factor();
      expr = std::make_unique<BinaryNode>(
          std::move(expr),
          op,
          std::move(right)
      );
  }
  
  return expr;
}

std::unique_ptr<ExpressionNode> Parser::factor() {
  auto expr = unary();
  
  while (match({TokenType::OP_STAR, TokenType::OP_SLASH, TokenType::OP_PERCENT})) {
      Token op = previous();
      auto right = unary();
      expr = std::make_unique<BinaryNode>(
          std::move(expr),
          op,
          std::move(right)
      );
  }
  
  return expr;
}

std::unique_ptr<ExpressionNode> Parser::unary() {
  if (match({
      TokenType::OP_MINUS, TokenType::OP_PLUS, TokenType::OP_EXCLAMATION, 
      TokenType::OP_TILDE, TokenType::OP_STAR, TokenType::OP_AMPERSAND,
      TokenType::OP_PLUS_PLUS, TokenType::OP_MINUS_MINUS
  })) {
      Token op = previous();
      auto operand = unary();
      return std::make_unique<UnaryNode>(op, std::move(operand));
  }
  
  return postfix();
}

std::unique_ptr<ExpressionNode> Parser::postfix() {
  auto expr = primary();
  
  while (true) {
      if (match(TokenType::LEFT_BRACKET)) {
          // Array access
          auto index = expression();
          consume(TokenType::RIGHT_BRACKET, "Expected ']' after array index");
          expr = std::make_unique<ArrayAccessNode>(std::move(expr), std::move(index));
      } else if (match(TokenType::LEFT_PAREN)) {
          // Function call
          std::vector<std::unique_ptr<ExpressionNode>> arguments;
          
          if (!check(TokenType::RIGHT_PAREN)) {
              do {
                  arguments.push_back(expression());
              } while (match(TokenType::COMMA));
          }
          
          consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments");
          expr = std::make_unique<CallNode>(std::move(expr), std::move(arguments));
      } else if (match({TokenType::OP_DOT, TokenType::OP_ARROW})) {
          // Member access
          Token op = previous();
          const Token& member = peek();
          consume(TokenType::IDENTIFIER, "Expected identifier after '.' or '->'");
          expr = std::make_unique<MemberAccessNode>(std::move(expr), op, member);
      } else if (match({TokenType::OP_PLUS_PLUS, TokenType::OP_MINUS_MINUS})) {
          // Postfix increment/decrement
          Token op = previous();
          // Create a unary operation with postfix flag
          expr = std::make_unique<UnaryNode>(op, std::move(expr));
      } else {
          break;
      }
  }
  
  return expr;
}

std::unique_ptr<ExpressionNode> Parser::primary() {
  if (match(TokenType::INTEGER_LITERAL) || 
      match(TokenType::FLOAT_LITERAL) || 
      match(TokenType::CHAR_LITERAL) || 
      match(TokenType::STRING_LITERAL)) {
      return std::make_unique<LiteralNode>(previous());
  }
  
  if (match(TokenType::IDENTIFIER)) {
      return std::make_unique<VariableNode>(previous());
  }
  
  if (match(TokenType::LEFT_PAREN)) {
      auto expr = expression();
      consume(TokenType::RIGHT_PAREN, "Expected ')' after expression");
      return expr;
  }
  
  // Handle other primary expressions
  
  errorHandler.error(peek().line, peek().column, "Expected expression");
  throw std::runtime_error("Expected expression");
}

} // namespace ccc