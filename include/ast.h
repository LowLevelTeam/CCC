#ifndef CCC_AST_H
#define CCC_AST_H

#include <string>
#include <vector>
#include <memory>
#include "token.h"

namespace ccc {

// Base class for all AST nodes
class ASTNode {
public:
  virtual ~ASTNode() = default;
  virtual std::string getNodeType() const = 0;
};

// Expression nodes
class ExpressionNode : public ASTNode {
public:
  virtual ~ExpressionNode() = default;
};

// Literal expression (numbers, strings, etc.)
class LiteralNode : public ExpressionNode {
public:
  LiteralNode(const Token& token) : token(token) {}
  virtual ~LiteralNode() = default;
  
  std::string getNodeType() const override { return "LiteralNode"; }
  
  Token token;
};

// Variable reference
class VariableNode : public ExpressionNode {
public:
  VariableNode(const Token& name) : name(name) {}
  virtual ~VariableNode() = default;
  
  std::string getNodeType() const override { return "VariableNode"; }
  
  Token name;
};

// Unary operation
class UnaryNode : public ExpressionNode {
public:
  UnaryNode(const Token& op, std::unique_ptr<ExpressionNode> operand)
      : op(op), operand(std::move(operand)) {}
  virtual ~UnaryNode() = default;
  
  std::string getNodeType() const override { return "UnaryNode"; }
  
  Token op;
  std::unique_ptr<ExpressionNode> operand;
};

// Binary operation
class BinaryNode : public ExpressionNode {
public:
  BinaryNode(std::unique_ptr<ExpressionNode> left, const Token& op, std::unique_ptr<ExpressionNode> right)
      : left(std::move(left)), op(op), right(std::move(right)) {}
  virtual ~BinaryNode() = default;
  
  std::string getNodeType() const override { return "BinaryNode"; }
  
  std::unique_ptr<ExpressionNode> left;
  Token op;
  std::unique_ptr<ExpressionNode> right;
};

// Function call
class CallNode : public ExpressionNode {
public:
  CallNode(std::unique_ptr<ExpressionNode> callee, std::vector<std::unique_ptr<ExpressionNode>> arguments)
      : callee(std::move(callee)), arguments(std::move(arguments)) {}
  virtual ~CallNode() = default;
  
  std::string getNodeType() const override { return "CallNode"; }
  
  std::unique_ptr<ExpressionNode> callee;
  std::vector<std::unique_ptr<ExpressionNode>> arguments;
};

// Array access
class ArrayAccessNode : public ExpressionNode {
public:
  ArrayAccessNode(std::unique_ptr<ExpressionNode> array, std::unique_ptr<ExpressionNode> index)
      : array(std::move(array)), index(std::move(index)) {}
  virtual ~ArrayAccessNode() = default;
  
  std::string getNodeType() const override { return "ArrayAccessNode"; }
  
  std::unique_ptr<ExpressionNode> array;
  std::unique_ptr<ExpressionNode> index;
};

// Member access (a.b or a->b)
class MemberAccessNode : public ExpressionNode {
public:
  MemberAccessNode(std::unique_ptr<ExpressionNode> object, const Token& op, const Token& member)
      : object(std::move(object)), op(op), member(member) {}
  virtual ~MemberAccessNode() = default;
  
  std::string getNodeType() const override { return "MemberAccessNode"; }
  
  std::unique_ptr<ExpressionNode> object;
  Token op;  // . or ->
  Token member;
};

// Conditional expression (a ? b : c)
class ConditionalNode : public ExpressionNode {
public:
  ConditionalNode(std::unique_ptr<ExpressionNode> condition, 
                  std::unique_ptr<ExpressionNode> trueExpr, 
                  std::unique_ptr<ExpressionNode> falseExpr)
      : condition(std::move(condition)), 
        trueExpr(std::move(trueExpr)), 
        falseExpr(std::move(falseExpr)) {}
  virtual ~ConditionalNode() = default;
  
  std::string getNodeType() const override { return "ConditionalNode"; }
  
  std::unique_ptr<ExpressionNode> condition;
  std::unique_ptr<ExpressionNode> trueExpr;
  std::unique_ptr<ExpressionNode> falseExpr;
};

// Type representation
class TypeNode : public ASTNode {
public:
  TypeNode(const Token& name, bool isConst = false, bool isVolatile = false)
      : name(name), isConst(isConst), isVolatile(isVolatile), isPointer(false), pointerLevel(0) {}
  
  TypeNode(const Token& name, bool isConst, bool isVolatile, bool isPointer, int pointerLevel)
      : name(name), isConst(isConst), isVolatile(isVolatile), isPointer(isPointer), pointerLevel(pointerLevel) {}
  
  virtual ~TypeNode() = default;
  
  std::string getNodeType() const override { return "TypeNode"; }
  
  Token name;
  bool isConst;
  bool isVolatile;
  bool isPointer;
  int pointerLevel;  // For pointers, number of * (e.g., int** has level 2)
};

// Statement nodes
class StatementNode : public ASTNode {
public:
  virtual ~StatementNode() = default;
};

// Expression statement
class ExpressionStatementNode : public StatementNode {
public:
  ExpressionStatementNode(std::unique_ptr<ExpressionNode> expression)
      : expression(std::move(expression)) {}
  virtual ~ExpressionStatementNode() = default;
  
  std::string getNodeType() const override { return "ExpressionStatementNode"; }
  
  std::unique_ptr<ExpressionNode> expression;
};

// Block statement
class BlockNode : public StatementNode {
public:
  BlockNode(std::vector<std::unique_ptr<StatementNode>> statements)
      : statements(std::move(statements)) {}
  virtual ~BlockNode() = default;
  
  std::string getNodeType() const override { return "BlockNode"; }
  
  std::vector<std::unique_ptr<StatementNode>> statements;
};

// Variable declaration
class VariableDeclarationNode : public StatementNode {
public:
  VariableDeclarationNode(std::unique_ptr<TypeNode> type, const Token& name, 
                        std::unique_ptr<ExpressionNode> initializer = nullptr)
      : type(std::move(type)), name(name), initializer(std::move(initializer)) {}
  virtual ~VariableDeclarationNode() = default;
  
  std::string getNodeType() const override { return "VariableDeclarationNode"; }
  
  std::unique_ptr<TypeNode> type;
  Token name;
  std::unique_ptr<ExpressionNode> initializer;
};

// If statement
class IfNode : public StatementNode {
public:
  IfNode(std::unique_ptr<ExpressionNode> condition, std::unique_ptr<StatementNode> thenBranch,
        std::unique_ptr<StatementNode> elseBranch = nullptr)
      : condition(std::move(condition)), thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {}
  virtual ~IfNode() = default;
  
  std::string getNodeType() const override { return "IfNode"; }
  
  std::unique_ptr<ExpressionNode> condition;
  std::unique_ptr<StatementNode> thenBranch;
  std::unique_ptr<StatementNode> elseBranch;
};

// While statement
class WhileNode : public StatementNode {
public:
  WhileNode(std::unique_ptr<ExpressionNode> condition, std::unique_ptr<StatementNode> body)
      : condition(std::move(condition)), body(std::move(body)) {}
  virtual ~WhileNode() = default;
  
  std::string getNodeType() const override { return "WhileNode"; }
  
  std::unique_ptr<ExpressionNode> condition;
  std::unique_ptr<StatementNode> body;
};

// Do-while statement
class DoWhileNode : public StatementNode {
public:
  DoWhileNode(std::unique_ptr<StatementNode> body, std::unique_ptr<ExpressionNode> condition)
      : body(std::move(body)), condition(std::move(condition)) {}
  virtual ~DoWhileNode() = default;
  
  std::string getNodeType() const override { return "DoWhileNode"; }
  
  std::unique_ptr<StatementNode> body;
  std::unique_ptr<ExpressionNode> condition;
};

// For statement
class ForNode : public StatementNode {
public:
  ForNode(std::unique_ptr<StatementNode> initializer, std::unique_ptr<ExpressionNode> condition,
          std::unique_ptr<ExpressionNode> increment, std::unique_ptr<StatementNode> body)
      : initializer(std::move(initializer)), condition(std::move(condition)), 
        increment(std::move(increment)), body(std::move(body)) {}
  virtual ~ForNode() = default;
  
  std::string getNodeType() const override { return "ForNode"; }
  
  std::unique_ptr<StatementNode> initializer;
  std::unique_ptr<ExpressionNode> condition;
  std::unique_ptr<ExpressionNode> increment;
  std::unique_ptr<StatementNode> body;
};

// Return statement
class ReturnNode : public StatementNode {
public:
  ReturnNode(std::unique_ptr<ExpressionNode> value = nullptr)
      : value(std::move(value)) {}
  virtual ~ReturnNode() = default;
  
  std::string getNodeType() const override { return "ReturnNode"; }
  
  std::unique_ptr<ExpressionNode> value;
};

// Break statement
class BreakNode : public StatementNode {
public:
  BreakNode() {}
  virtual ~BreakNode() = default;
  
  std::string getNodeType() const override { return "BreakNode"; }
};

// Continue statement
class ContinueNode : public StatementNode {
public:
  ContinueNode() {}
  virtual ~ContinueNode() = default;
  
  std::string getNodeType() const override { return "ContinueNode"; }
};

// Function parameter
class ParameterNode : public ASTNode {
public:
  ParameterNode(std::unique_ptr<TypeNode> type, const Token& name)
      : type(std::move(type)), name(name) {}
  virtual ~ParameterNode() = default;
  
  std::string getNodeType() const override { return "ParameterNode"; }
  
  std::unique_ptr<TypeNode> type;
  Token name;
};

// Function declaration
class FunctionDeclarationNode : public ASTNode {
public:
  FunctionDeclarationNode(std::unique_ptr<TypeNode> returnType, const Token& name,
                        std::vector<std::unique_ptr<ParameterNode>> parameters,
                        std::unique_ptr<BlockNode> body = nullptr)
      : returnType(std::move(returnType)), name(name), 
        parameters(std::move(parameters)), body(std::move(body)) {}
  virtual ~FunctionDeclarationNode() = default;
  
  std::string getNodeType() const override { return "FunctionDeclarationNode"; }
  
  std::unique_ptr<TypeNode> returnType;
  Token name;
  std::vector<std::unique_ptr<ParameterNode>> parameters;
  std::unique_ptr<BlockNode> body;
};

// Program node (top-level)
class ProgramNode : public ASTNode {
public:
  ProgramNode(std::vector<std::unique_ptr<ASTNode>> declarations)
      : declarations(std::move(declarations)) {}
  virtual ~ProgramNode() = default;
  
  std::string getNodeType() const override { return "ProgramNode"; }
  
  std::vector<std::unique_ptr<ASTNode>> declarations;
};

} // namespace ccc

#endif // CCC_AST_H