#ifndef CCC_SEMANTIC_H
#define CCC_SEMANTIC_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "ast.h"
#include "error.h"

namespace ccc {

// Type information for semantic analysis
struct TypeInfo {
  enum class Kind {
      VOID,
      CHAR,
      INT,
      FLOAT,
      DOUBLE,
      STRUCT,
      ARRAY,
      POINTER,
      FUNCTION
  };
  
  Kind kind;
  bool isConst;
  bool isVolatile;
  int size;                          // Size in bytes
  std::unique_ptr<TypeInfo> base;    // For pointers, arrays, and functions
  std::vector<TypeInfo> parameters;  // For functions
  
  TypeInfo(Kind kind, bool isConst = false, bool isVolatile = false, int size = 0)
      : kind(kind), isConst(isConst), isVolatile(isVolatile), size(size) {}
  
  // Deep copy constructor
  TypeInfo(const TypeInfo& other)
      : kind(other.kind), isConst(other.isConst), isVolatile(other.isVolatile), size(other.size) {
      if (other.base) {
          base = std::make_unique<TypeInfo>(*other.base);
      }
      for (const auto& param : other.parameters) {
          parameters.push_back(param);
      }
  }
  
  bool isScalar() const {
      return kind == Kind::CHAR || kind == Kind::INT || 
              kind == Kind::FLOAT || kind == Kind::DOUBLE ||
              kind == Kind::POINTER;
  }
  
  bool isNumeric() const {
      return kind == Kind::CHAR || kind == Kind::INT || 
              kind == Kind::FLOAT || kind == Kind::DOUBLE;
  }
  
  bool isInteger() const {
      return kind == Kind::CHAR || kind == Kind::INT;
  }
  
  bool isFloatingPoint() const {
      return kind == Kind::FLOAT || kind == Kind::DOUBLE;
  }
  
  // Create standard type infos
  static TypeInfo createVoid() { return TypeInfo(Kind::VOID, false, false, 0); }
  static TypeInfo createChar() { return TypeInfo(Kind::CHAR, false, false, 1); }
  static TypeInfo createInt() { return TypeInfo(Kind::INT, false, false, 4); }
  static TypeInfo createFloat() { return TypeInfo(Kind::FLOAT, false, false, 4); }
  static TypeInfo createDouble() { return TypeInfo(Kind::DOUBLE, false, false, 8); }
  
  static TypeInfo createPointer(const TypeInfo& baseType) { 
      TypeInfo info(Kind::POINTER, false, false, 8);
      info.base = std::make_unique<TypeInfo>(baseType);
      return info;
  }
  
  static TypeInfo createArray(const TypeInfo& elementType, int size) {
      TypeInfo info(Kind::ARRAY, false, false, elementType.size * size);
      info.base = std::make_unique<TypeInfo>(elementType);
      return info;
  }
  
  static TypeInfo createFunction(const TypeInfo& returnType, const std::vector<TypeInfo>& paramTypes) {
      TypeInfo info(Kind::FUNCTION, false, false, 0);
      info.base = std::make_unique<TypeInfo>(returnType);
      info.parameters = paramTypes;
      return info;
  }
};

// Symbol information for semantic analysis
struct SymbolInfo {
  enum class Kind {
      VARIABLE,
      FUNCTION,
      PARAMETER,
      TYPEDEF
  };
  
  Kind kind;
  TypeInfo type;
  std::string name;
  int scopeLevel;
  
  SymbolInfo(Kind kind, const TypeInfo& type, const std::string& name, int scopeLevel)
      : kind(kind), type(type), name(name), scopeLevel(scopeLevel) {}
};

// Symbol table to track variables, functions, etc.
class SymbolTable {
public:
  SymbolTable();
  
  // Enter and leave scopes
  void enterScope();
  void leaveScope();
  int currentScopeLevel() const;
  
  // Add symbols
  void addVariable(const std::string& name, const TypeInfo& type);
  void addFunction(const std::string& name, const TypeInfo& type);
  void addParameter(const std::string& name, const TypeInfo& type);
  void addTypedef(const std::string& name, const TypeInfo& type);
  
  // Lookup symbols
  bool exists(const std::string& name) const;
  bool existsInCurrentScope(const std::string& name) const;
  const SymbolInfo* lookup(const std::string& name) const;
  
  // Clear the table
  void clear();

private:
  // Symbol storage - maps names to symbols
  std::vector<std::unordered_map<std::string, SymbolInfo>> scopes;
  int currentScope;
};

// Semantic analyzer
class SemanticAnalyzer {
public:
  SemanticAnalyzer(ErrorHandler& errorHandler);
  
  // Analyze the AST
  void analyze(ASTNode* root);
  
  // Get all errors
  bool hasErrors() const;
  
private:
  // Error reporting
  ErrorHandler& errorHandler;
  
  // Symbol table
  SymbolTable symbolTable;
  
  // Function context
  TypeInfo* currentFunctionReturnType;
  bool hasReturn;
  
  // Visitors for analysis
  void visitProgram(ProgramNode* node);
  void visitFunctionDeclaration(FunctionDeclarationNode* node);
  void visitVariableDeclaration(VariableDeclarationNode* node);
  void visitParameter(ParameterNode* node);
  void visitBlock(BlockNode* node);
  void visitExpressionStatement(ExpressionStatementNode* node);
  void visitIfStatement(IfNode* node);
  void visitWhileStatement(WhileNode* node);
  void visitDoWhileStatement(DoWhileNode* node);
  void visitForStatement(ForNode* node);
  void visitReturnStatement(ReturnNode* node);
  void visitBreakStatement(BreakNode* node);
  void visitContinueStatement(ContinueNode* node);
  
  TypeInfo visitExpression(ExpressionNode* node);
  TypeInfo visitLiteral(LiteralNode* node);
  TypeInfo visitVariable(VariableNode* node);
  TypeInfo visitUnary(UnaryNode* node);
  TypeInfo visitBinary(BinaryNode* node);
  TypeInfo visitCall(CallNode* node);
  TypeInfo visitArrayAccess(ArrayAccessNode* node);
  TypeInfo visitMemberAccess(MemberAccessNode* node);
  TypeInfo visitConditional(ConditionalNode* node);
  
  TypeInfo getTypeFromTypeNode(TypeNode* node);
  
  // Type checking
  bool areTypesCompatible(const TypeInfo& source, const TypeInfo& target);
  TypeInfo getCommonType(const TypeInfo& a, const TypeInfo& b);
};

} // namespace ccc

#endif // CCC_SEMANTIC_H