#include "semantic.h"

namespace ccc {

// SymbolTable implementation
SymbolTable::SymbolTable() : currentScope(0) {
  // Create the global scope
  scopes.push_back({});
}

void SymbolTable::enterScope() {
  currentScope++;
  if (currentScope >= scopes.size()) {
      scopes.push_back({});
  }
}

void SymbolTable::leaveScope() {
  if (currentScope == 0) {
      throw std::runtime_error("Cannot leave global scope");
  }
  currentScope--;
}

int SymbolTable::currentScopeLevel() const {
  return currentScope;
}

void SymbolTable::addVariable(const std::string& name, const TypeInfo& type) {
  scopes[currentScope][name] = SymbolInfo(SymbolInfo::Kind::VARIABLE, type, name, currentScope);
}

void SymbolTable::addFunction(const std::string& name, const TypeInfo& type) {
  scopes[0][name] = SymbolInfo(SymbolInfo::Kind::FUNCTION, type, name, 0);
}

void SymbolTable::addParameter(const std::string& name, const TypeInfo& type) {
  scopes[currentScope][name] = SymbolInfo(SymbolInfo::Kind::PARAMETER, type, name, currentScope);
}

void SymbolTable::addTypedef(const std::string& name, const TypeInfo& type) {
  scopes[currentScope][name] = SymbolInfo(SymbolInfo::Kind::TYPEDEF, type, name, currentScope);
}

bool SymbolTable::exists(const std::string& name) const {
  for (int i = currentScope; i >= 0; i--) {
      if (scopes[i].find(name) != scopes[i].end()) {
          return true;
      }
  }
  return false;
}

bool SymbolTable::existsInCurrentScope(const std::string& name) const {
  return scopes[currentScope].find(name) != scopes[currentScope].end();
}

const SymbolInfo* SymbolTable::lookup(const std::string& name) const {
  for (int i = currentScope; i >= 0; i--) {
      auto it = scopes[i].find(name);
      if (it != scopes[i].end()) {
          return &it->second;
      }
  }
  return nullptr;
}

void SymbolTable::clear() {
  scopes.clear();
  scopes.push_back({});
  currentScope = 0;
}

// SemanticAnalyzer implementation
SemanticAnalyzer::SemanticAnalyzer(ErrorHandler& errorHandler)
  : errorHandler(errorHandler), currentFunctionReturnType(nullptr), hasReturn(false) {
}

void SemanticAnalyzer::analyze(ASTNode* root) {
  if (!root) return;
  
  // Clear the symbol table for a fresh analysis
  symbolTable.clear();
  
  // Visit the root node
  if (root->getNodeType() == "ProgramNode") {
      visitProgram(static_cast<ProgramNode*>(root));
  } else {
      errorHandler.error(0, 0, "Expected program node as root");
  }
}

bool SemanticAnalyzer::hasErrors() const {
  return errorHandler.hasErrors();
}

void SemanticAnalyzer::visitProgram(ProgramNode* node) {
  for (const auto& declaration : node->declarations) {
      if (declaration->getNodeType() == "FunctionDeclarationNode") {
          visitFunctionDeclaration(static_cast<FunctionDeclarationNode*>(declaration.get()));
      } else if (declaration->getNodeType() == "VariableDeclarationNode") {
          visitVariableDeclaration(static_cast<VariableDeclarationNode*>(declaration.get()));
      }
      // Add other top-level declarations as needed
  }
}

void SemanticAnalyzer::visitFunctionDeclaration(FunctionDeclarationNode* node) {
  // Get the return type
  TypeInfo returnType = getTypeFromTypeNode(node->returnType.get());
  
  // Process parameters
  std::vector<TypeInfo> paramTypes;
  for (const auto& param : node->parameters) {
      TypeInfo paramType = getTypeFromTypeNode(param->type.get());
      paramTypes.push_back(paramType);
  }
  
  // Create function type
  TypeInfo functionType = TypeInfo::createFunction(returnType, paramTypes);
  
  // Check if function already exists
  const std::string& name = node->name.lexeme;
  if (symbolTable.existsInCurrentScope(name)) {
      errorHandler.error(node->name.line, node->name.column, 
                        "Function '" + name + "' already declared in this scope");
      return;
  }
  
  // Add function to symbol table
  symbolTable.addFunction(name, functionType);
  
  // Process function body if it exists
  if (node->body) {
      // Enter function scope
      symbolTable.enterScope();
      
      // Set current function return type for checking return statements
      currentFunctionReturnType = &returnType;
      hasReturn = returnType.kind == TypeInfo::Kind::VOID; // void functions implicitly return
      
      // Add parameters to symbol table
      for (const auto& param : node->parameters) {
          visitParameter(param.get());
      }
      
      // Process the function body
      visitBlock(node->body.get());
      
      // Check if function has a return statement (if needed)
      if (!hasReturn && returnType.kind != TypeInfo::Kind::VOID) {
          errorHandler.error(node->name.line, node->name.column, 
                            "Function '" + name + "' may not return a value");
      }
      
      // Reset function context
      currentFunctionReturnType = nullptr;
      
      // Leave function scope
      symbolTable.leaveScope();
  }
}

void SemanticAnalyzer::visitVariableDeclaration(VariableDeclarationNode* node) {
  // Get the variable type
  TypeInfo type = getTypeFromTypeNode(node->type.get());
  
  // Check if variable already exists in current scope
  const std::string& name = node->name.lexeme;
  if (symbolTable.existsInCurrentScope(name)) {
      errorHandler.error(node->name.line, node->name.column, 
                        "Variable '" + name + "' already declared in this scope");
      return;
  }
  
  // Check initializer if present
  if (node->initializer) {
      TypeInfo initType = visitExpression(node->initializer.get());
      
      // Ensure initializer type is compatible with variable type
      if (!areTypesCompatible(initType, type)) {
          errorHandler.error(node->name.line, node->name.column, 
                            "Cannot initialize variable of type '" + std::to_string(static_cast<int>(type.kind)) + 
                            "' with expression of type '" + std::to_string(static_cast<int>(initType.kind)) + "'");
      }
  }
  
  // Add variable to symbol table
  symbolTable.addVariable(name, type);
}

void SemanticAnalyzer::visitParameter(ParameterNode* node) {
  // Get the parameter type
  TypeInfo type = getTypeFromTypeNode(node->type.get());
  
  // Check if parameter name is empty (allowed in declarations)
  if (node->name.lexeme.empty()) {
      return;
  }
  
  // Check if parameter already exists in current scope
  const std::string& name = node->name.lexeme;
  if (symbolTable.existsInCurrentScope(name)) {
      errorHandler.error(node->name.line, node->name.column, 
                        "Parameter '" + name + "' already declared");
      return;
  }
  
  // Add parameter to symbol table
  symbolTable.addParameter(name, type);
}

void SemanticAnalyzer::visitBlock(BlockNode* node) {
  // Enter a new scope
  symbolTable.enterScope();
  
  // Process all statements in the block
  for (const auto& statement : node->statements) {
      if (statement->getNodeType() == "ExpressionStatementNode") {
          visitExpressionStatement(static_cast<ExpressionStatementNode*>(statement.get()));
      } else if (statement->getNodeType() == "VariableDeclarationNode") {
          visitVariableDeclaration(static_cast<VariableDeclarationNode*>(statement.get()));
      } else if (statement->getNodeType() == "BlockNode") {
          visitBlock(static_cast<BlockNode*>(statement.get()));
      } else if (statement->getNodeType() == "IfNode") {
          visitIfStatement(static_cast<IfNode*>(statement.get()));
      } else if (statement->getNodeType() == "WhileNode") {
          visitWhileStatement(static_cast<WhileNode*>(statement.get()));
      } else if (statement->getNodeType() == "DoWhileNode") {
          visitDoWhileStatement(static_cast<DoWhileNode*>(statement.get()));
      } else if (statement->getNodeType() == "ForNode") {
          visitForStatement(static_cast<ForNode*>(statement.get()));
      } else if (statement->getNodeType() == "ReturnNode") {
          visitReturnStatement(static_cast<ReturnNode*>(statement.get()));
      } else if (statement->getNodeType() == "BreakNode") {
          visitBreakStatement(static_cast<BreakNode*>(statement.get()));
      } else if (statement->getNodeType() == "ContinueNode") {
          visitContinueStatement(static_cast<ContinueNode*>(statement.get()));
      }
      // Add other statement types as needed
  }
  
  // Leave the scope
  symbolTable.leaveScope();
}

void SemanticAnalyzer::visitExpressionStatement(ExpressionStatementNode* node) {
  visitExpression(node->expression.get());
}

void SemanticAnalyzer::visitIfStatement(IfNode* node) {
  // Check condition expression
  TypeInfo condType = visitExpression(node->condition.get());
  
  // Ensure condition is a scalar type (can be evaluated as boolean)
  if (!condType.isScalar()) {
      errorHandler.error(0, 0, "If condition must be a scalar type");
  }
  
  // Process then branch
  if (node->thenBranch->getNodeType() == "BlockNode") {
      visitBlock(static_cast<BlockNode*>(node->thenBranch.get()));
  } else {
      // For non-block statements, create an implicit scope
      symbolTable.enterScope();
      
      if (node->thenBranch->getNodeType() == "ExpressionStatementNode") {
          visitExpressionStatement(static_cast<ExpressionStatementNode*>(node->thenBranch.get()));
      } else if (node->thenBranch->getNodeType() == "VariableDeclarationNode") {
          visitVariableDeclaration(static_cast<VariableDeclarationNode*>(node->thenBranch.get()));
      } else if (node->thenBranch->getNodeType() == "IfNode") {
          visitIfStatement(static_cast<IfNode*>(node->thenBranch.get()));
      } else if (node->thenBranch->getNodeType() == "WhileNode") {
          visitWhileStatement(static_cast<WhileNode*>(node->thenBranch.get()));
      } else if (node->thenBranch->getNodeType() == "DoWhileNode") {
          visitDoWhileStatement(static_cast<DoWhileNode*>(node->thenBranch.get()));
      } else if (node->thenBranch->getNodeType() == "ForNode") {
          visitForStatement(static_cast<ForNode*>(node->thenBranch.get()));
      } else if (node->thenBranch->getNodeType() == "ReturnNode") {
          visitReturnStatement(static_cast<ReturnNode*>(node->thenBranch.get()));
      } else if (node->thenBranch->getNodeType() == "BreakNode") {
          visitBreakStatement(static_cast<BreakNode*>(node->thenBranch.get()));
      } else if (node->thenBranch->getNodeType() == "ContinueNode") {
          visitContinueStatement(static_cast<ContinueNode*>(node->thenBranch.get()));
      }
      
      symbolTable.leaveScope();
  }
  
  // Process else branch if it exists
  if (node->elseBranch) {
      if (node->elseBranch->getNodeType() == "BlockNode") {
          visitBlock(static_cast<BlockNode*>(node->elseBranch.get()));
      } else {
          // For non-block statements, create an implicit scope
          symbolTable.enterScope();
          
          if (node->elseBranch->getNodeType() == "ExpressionStatementNode") {
              visitExpressionStatement(static_cast<ExpressionStatementNode*>(node->elseBranch.get()));
          } else if (node->elseBranch->getNodeType() == "VariableDeclarationNode") {
              visitVariableDeclaration(static_cast<VariableDeclarationNode*>(node->elseBranch.get()));
          } else if (node->elseBranch->getNodeType() == "IfNode") {
              visitIfStatement(static_cast<IfNode*>(node->elseBranch.get()));
          } else if (node->elseBranch->getNodeType() == "WhileNode") {
              visitWhileStatement(static_cast<WhileNode*>(node->elseBranch.get()));
          } else if (node->elseBranch->getNodeType() == "DoWhileNode") {
              visitDoWhileStatement(static_cast<DoWhileNode*>(node->elseBranch.get()));
          } else if (node->elseBranch->getNodeType() == "ForNode") {
              visitForStatement(static_cast<ForNode*>(node->elseBranch.get()));
          } else if (node->elseBranch->getNodeType() == "ReturnNode") {
              visitReturnStatement(static_cast<ReturnNode*>(node->elseBranch.get()));
          } else if (node->elseBranch->getNodeType() == "BreakNode") {
              visitBreakStatement(static_cast<BreakNode*>(node->elseBranch.get()));
          } else if (node->elseBranch->getNodeType() == "ContinueNode") {
              visitContinueStatement(static_cast<ContinueNode*>(node->elseBranch.get()));
          }
          
          symbolTable.leaveScope();
      }
  }
}

void SemanticAnalyzer::visitWhileStatement(WhileNode* node) {
  // Check condition expression
  TypeInfo condType = visitExpression(node->condition.get());
  
  // Ensure condition is a scalar type (can be evaluated as boolean)
  if (!condType.isScalar()) {
      errorHandler.error(0, 0, "While condition must be a scalar type");
  }
  
  // Process the body
  if (node->body->getNodeType() == "BlockNode") {
      visitBlock(static_cast<BlockNode*>(node->body.get()));
  } else {
      // For non-block statements, create an implicit scope
      symbolTable.enterScope();
      
      // Visit the appropriate statement type
      if (node->body->getNodeType() == "ExpressionStatementNode") {
          visitExpressionStatement(static_cast<ExpressionStatementNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "VariableDeclarationNode") {
          visitVariableDeclaration(static_cast<VariableDeclarationNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "IfNode") {
          visitIfStatement(static_cast<IfNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "WhileNode") {
          visitWhileStatement(static_cast<WhileNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "DoWhileNode") {
          visitDoWhileStatement(static_cast<DoWhileNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "ForNode") {
          visitForStatement(static_cast<ForNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "ReturnNode") {
          visitReturnStatement(static_cast<ReturnNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "BreakNode") {
          visitBreakStatement(static_cast<BreakNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "ContinueNode") {
          visitContinueStatement(static_cast<ContinueNode*>(node->body.get()));
      }
      
      symbolTable.leaveScope();
  }
}

void SemanticAnalyzer::visitDoWhileStatement(DoWhileNode* node) {
  // Process the body
  if (node->body->getNodeType() == "BlockNode") {
      visitBlock(static_cast<BlockNode*>(node->body.get()));
  } else {
      // For non-block statements, create an implicit scope
      symbolTable.enterScope();
      
      // Visit the appropriate statement type
      if (node->body->getNodeType() == "ExpressionStatementNode") {
          visitExpressionStatement(static_cast<ExpressionStatementNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "VariableDeclarationNode") {
          visitVariableDeclaration(static_cast<VariableDeclarationNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "IfNode") {
          visitIfStatement(static_cast<IfNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "WhileNode") {
          visitWhileStatement(static_cast<WhileNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "DoWhileNode") {
          visitDoWhileStatement(static_cast<DoWhileNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "ForNode") {
          visitForStatement(static_cast<ForNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "ReturnNode") {
          visitReturnStatement(static_cast<ReturnNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "BreakNode") {
          visitBreakStatement(static_cast<BreakNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "ContinueNode") {
          visitContinueStatement(static_cast<ContinueNode*>(node->body.get()));
      }
      
      symbolTable.leaveScope();
  }
  
  // Check condition expression
  TypeInfo condType = visitExpression(node->condition.get());
  
  // Ensure condition is a scalar type (can be evaluated as boolean)
  if (!condType.isScalar()) {
      errorHandler.error(0, 0, "Do-while condition must be a scalar type");
  }
}

void SemanticAnalyzer::visitForStatement(ForNode* node) {
  // Enter for loop scope
  symbolTable.enterScope();
  
  // Process initializer
  if (node->initializer) {
      if (node->initializer->getNodeType() == "ExpressionStatementNode") {
          visitExpressionStatement(static_cast<ExpressionStatementNode*>(node->initializer.get()));
      } else if (node->initializer->getNodeType() == "VariableDeclarationNode") {
          visitVariableDeclaration(static_cast<VariableDeclarationNode*>(node->initializer.get()));
      }
  }
  
  // Check condition if present
  if (node->condition) {
      TypeInfo condType = visitExpression(node->condition.get());
      
      // Ensure condition is a scalar type (can be evaluated as boolean)
      if (!condType.isScalar()) {
          errorHandler.error(0, 0, "For condition must be a scalar type");
      }
  }
  
  // Check increment expression if present
  if (node->increment) {
      visitExpression(node->increment.get());
  }
  
  // Process the body
  if (node->body->getNodeType() == "BlockNode") {
      visitBlock(static_cast<BlockNode*>(node->body.get()));
  } else {
      // For non-block statements, create an implicit scope
      symbolTable.enterScope();
      
      // Visit the appropriate statement type
      if (node->body->getNodeType() == "ExpressionStatementNode") {
          visitExpressionStatement(static_cast<ExpressionStatementNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "VariableDeclarationNode") {
          visitVariableDeclaration(static_cast<VariableDeclarationNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "IfNode") {
          visitIfStatement(static_cast<IfNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "WhileNode") {
          visitWhileStatement(static_cast<WhileNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "DoWhileNode") {
          visitDoWhileStatement(static_cast<DoWhileNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "ForNode") {
          visitForStatement(static_cast<ForNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "ReturnNode") {
          visitReturnStatement(static_cast<ReturnNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "BreakNode") {
          visitBreakStatement(static_cast<BreakNode*>(node->body.get()));
      } else if (node->body->getNodeType() == "ContinueNode") {
          visitContinueStatement(static_cast<ContinueNode*>(node->body.get()));
      }
      
      symbolTable.leaveScope();
  }
  
  // Leave for loop scope
  symbolTable.leaveScope();
}

void SemanticAnalyzer::visitReturnStatement(ReturnNode* node) {
  // Ensure we're inside a function
  if (!currentFunctionReturnType) {
      errorHandler.error(0, 0, "Return statement outside of function");
      return;
  }
  
  // Mark that we've seen a return statement
  hasReturn = true;
  
  // Check return value
  if (node->value) {
      TypeInfo valueType = visitExpression(node->value.get());
      
      // Ensure return value type is compatible with function return type
      if (!areTypesCompatible(valueType, *currentFunctionReturnType)) {
          errorHandler.error(0, 0, "Cannot return value of incompatible type");
      }
  } else {
      // No return value, ensure function return type is void
      if (currentFunctionReturnType->kind != TypeInfo::Kind::VOID) {
          errorHandler.error(0, 0, "Non-void function should return a value");
      }
  }
}

void SemanticAnalyzer::visitBreakStatement(BreakNode* node) {
  // Could check if we're inside a loop or switch (not implemented)
}

void SemanticAnalyzer::visitContinueStatement(ContinueNode* node) {
  // Could check if we're inside a loop (not implemented)
}

TypeInfo SemanticAnalyzer::visitExpression(ExpressionNode* node) {
  if (!node) {
      return TypeInfo::createVoid();
  }
  
  if (node->getNodeType() == "LiteralNode") {
      return visitLiteral(static_cast<LiteralNode*>(node));
  } else if (node->getNodeType() == "VariableNode") {
      return visitVariable(static_cast<VariableNode*>(node));
  } else if (node->getNodeType() == "UnaryNode") {
      return visitUnary(static_cast<UnaryNode*>(node));
  } else if (node->getNodeType() == "BinaryNode") {
      return visitBinary(static_cast<BinaryNode*>(node));
  } else if (node->getNodeType() == "CallNode") {
      return visitCall(static_cast<CallNode*>(node));
  } else if (node->getNodeType() == "ArrayAccessNode") {
      return visitArrayAccess(static_cast<ArrayAccessNode*>(node));
  } else if (node->getNodeType() == "MemberAccessNode") {
      return visitMemberAccess(static_cast<MemberAccessNode*>(node));
  } else if (node->getNodeType() == "ConditionalNode") {
      return visitConditional(static_cast<ConditionalNode*>(node));
  }
  
  // Unknown expression type
  errorHandler.error(0, 0, "Unknown expression type: " + node->getNodeType());
  return TypeInfo::createVoid();
}

TypeInfo SemanticAnalyzer::visitLiteral(LiteralNode* node) {
  switch (node->token.type) {
      case TokenType::INTEGER_LITERAL:
          return TypeInfo::createInt();
      case TokenType::FLOAT_LITERAL:
          return TypeInfo::createFloat();
      case TokenType::CHAR_LITERAL:
          return TypeInfo::createChar();
      case TokenType::STRING_LITERAL:
          // String literals are arrays of chars
          return TypeInfo::createArray(TypeInfo::createChar(), node->token.lexeme.length() - 2 + 1); // -2 for quotes, +1 for null terminator
      default:
          errorHandler.error(node->token.line, node->token.column, "Unknown literal type");
          return TypeInfo::createVoid();
  }
}

TypeInfo SemanticAnalyzer::visitVariable(VariableNode* node) {
  const std::string& name = node->name.lexeme;
  
  // Look up the variable in the symbol table
  const SymbolInfo* symbol = symbolTable.lookup(name);
  if (!symbol) {
      errorHandler.error(node->name.line, node->name.column, "Undefined variable '" + name + "'");
      return TypeInfo::createVoid();
  }
  
  return symbol->type;
}

TypeInfo SemanticAnalyzer::visitUnary(UnaryNode* node) {
  TypeInfo operandType = visitExpression(node->operand.get());
  
  switch (node->op.type) {
      case TokenType::OP_MINUS:
      case TokenType::OP_PLUS:
          // Unary plus and minus require numeric operand
          if (!operandType.isNumeric()) {
              errorHandler.error(node->op.line, node->op.column, 
                                "Unary operator " + node->op.lexeme + " requires numeric operand");
              return TypeInfo::createVoid();
          }
          return operandType;
          
      case TokenType::OP_EXCLAMATION:
          // Logical not requires scalar operand
          if (!operandType.isScalar()) {
              errorHandler.error(node->op.line, node->op.column, 
                                "Unary operator ! requires scalar operand");
              return TypeInfo::createVoid();
          }
          return TypeInfo::createInt(); // Boolean operations return int
          
      case TokenType::OP_TILDE:
          // Bitwise not requires integer operand
          if (!operandType.isInteger()) {
              errorHandler.error(node->op.line, node->op.column, 
                                "Unary operator ~ requires integer operand");
              return TypeInfo::createVoid();
          }
          return operandType;
          
      case TokenType::OP_STAR:
          // Dereferencing requires pointer operand
          if (operandType.kind != TypeInfo::Kind::POINTER) {
              errorHandler.error(node->op.line, node->op.column, 
                                "Cannot dereference non-pointer type");
              return TypeInfo::createVoid();
          }
          return *operandType.base;
          
      case TokenType::OP_AMPERSAND:
          // Taking address creates a pointer to the operand type
          return TypeInfo::createPointer(operandType);
          
      case TokenType::OP_PLUS_PLUS:
      case TokenType::OP_MINUS_MINUS:
          // Increment/decrement requires numeric or pointer operand
          if (!operandType.isNumeric() && operandType.kind != TypeInfo::Kind::POINTER) {
              errorHandler.error(node->op.line, node->op.column, 
                                "Unary operator " + node->op.lexeme + " requires numeric or pointer operand");
              return TypeInfo::createVoid();
          }
          return operandType;
          
      default:
          errorHandler.error(node->op.line, node->op.column, 
                            "Unknown unary operator: " + node->op.lexeme);
          return TypeInfo::createVoid();
  }
}

TypeInfo SemanticAnalyzer::visitBinary(BinaryNode* node) {
  TypeInfo leftType = visitExpression(node->left.get());
  TypeInfo rightType = visitExpression(node->right.get());
  
  switch (node->op.type) {
      case TokenType::OP_PLUS:
          // Pointer arithmetic: pointer + integer
          if (leftType.kind == TypeInfo::Kind::POINTER && rightType.isInteger()) {
              return leftType;
          }
          // Integer + pointer
          if (leftType.isInteger() && rightType.kind == TypeInfo::Kind::POINTER) {
              return rightType;
          }
          // Numeric + numeric
          if (leftType.isNumeric() && rightType.isNumeric()) {
              return getCommonType(leftType, rightType);
          }
          
          errorHandler.error(node->op.line, node->op.column, 
                            "Invalid operands to binary +");
          return TypeInfo::createVoid();
          
      case TokenType::OP_MINUS:
          // Pointer arithmetic: pointer - integer
          if (leftType.kind == TypeInfo::Kind::POINTER && rightType.isInteger()) {
              return leftType;
          }
          // Pointer - pointer (gives an integer)
          if (leftType.kind == TypeInfo::Kind::POINTER && rightType.kind == TypeInfo::Kind::POINTER) {
              return TypeInfo::createInt();
          }
          // Numeric - numeric
          if (leftType.isNumeric() && rightType.isNumeric()) {
              return getCommonType(leftType, rightType);
          }
          
          errorHandler.error(node->op.line, node->op.column, 
                            "Invalid operands to binary -");
          return TypeInfo::createVoid();
          
      case TokenType::OP_STAR:
      case TokenType::OP_SLASH:
      case TokenType::OP_PERCENT:
          // Numeric operations
          if (leftType.isNumeric() && rightType.isNumeric()) {
              return getCommonType(leftType, rightType);
          }
          
          errorHandler.error(node->op.line, node->op.column, 
                            "Invalid operands to binary " + node->op.lexeme);
          return TypeInfo::createVoid();
          
      case TokenType::OP_LESS:
      case TokenType::OP_LESS_EQUALS:
      case TokenType::OP_GREATER:
      case TokenType::OP_GREATER_EQUALS:
      case TokenType::OP_EQUALS_EQUALS:
      case TokenType::OP_NOT_EQUALS:
          // Comparison operators require compatible types
          if (!areTypesCompatible(leftType, rightType) && !areTypesCompatible(rightType, leftType)) {
              errorHandler.error(node->op.line, node->op.column, 
                                "Incompatible types for comparison");
              return TypeInfo::createVoid();
          }
          return TypeInfo::createInt(); // Comparisons return int (0 or 1)
          
      case TokenType::OP_AMPERSAND:
      case TokenType::OP_PIPE:
      case TokenType::OP_CARET:
      case TokenType::OP_SHL:
      case TokenType::OP_SHR:
          // Bitwise operators require integer operands
          if (!leftType.isInteger() || !rightType.isInteger()) {
              errorHandler.error(node->op.line, node->op.column, 
                                "Bitwise operators require integer operands");
              return TypeInfo::createVoid();
          }
          return getCommonType(leftType, rightType);
          
      case TokenType::OP_LOGICAL_AND:
      case TokenType::OP_LOGICAL_OR:
          // Logical operators require scalar operands
          if (!leftType.isScalar() || !rightType.isScalar()) {
              errorHandler.error(node->op.line, node->op.column, 
                                "Logical operators require scalar operands");
              return TypeInfo::createVoid();
          }
          return TypeInfo::createInt(); // Logical operations return int
          
      case TokenType::OP_EQUALS:
          // Assignment requires compatible types
          if (!areTypesCompatible(rightType, leftType)) {
              errorHandler.error(node->op.line, node->op.column, 
                                "Cannot assign incompatible type");
              return TypeInfo::createVoid();
          }
          return leftType;
          
      default:
          errorHandler.error(node->op.line, node->op.column, 
                            "Unknown binary operator: " + node->op.lexeme);
          return TypeInfo::createVoid();
  }
}

TypeInfo SemanticAnalyzer::visitCall(CallNode* node) {
  // Check that the callee is a function
  TypeInfo calleeType = visitExpression(node->callee.get());
  
  if (calleeType.kind != TypeInfo::Kind::FUNCTION) {
      errorHandler.error(0, 0, "Called object is not a function");
      return TypeInfo::createVoid();
  }
  
  // Check number of arguments
  if (node->arguments.size() != calleeType.parameters.size()) {
      errorHandler.error(0, 0, "Wrong number of arguments to function call");
      return TypeInfo::createVoid();
  }
  
  // Check argument types
  for (size_t i = 0; i < node->arguments.size(); i++) {
      TypeInfo argType = visitExpression(node->arguments[i].get());
      
      if (!areTypesCompatible(argType, calleeType.parameters[i])) {
          errorHandler.error(0, 0, "Argument type mismatch in function call");
          // Continue checking other arguments
      }
  }
  
  // Return the function's return type
  return *calleeType.base;
}

TypeInfo SemanticAnalyzer::visitArrayAccess(ArrayAccessNode* node) {
  TypeInfo arrayType = visitExpression(node->array.get());
  TypeInfo indexType = visitExpression(node->index.get());
  
  // Array access requires array or pointer base
  if (arrayType.kind != TypeInfo::Kind::ARRAY && arrayType.kind != TypeInfo::Kind::POINTER) {
      errorHandler.error(0, 0, "Subscripted value is not an array or pointer");
      return TypeInfo::createVoid();
  }
  
  // Index must be integer
  if (!indexType.isInteger()) {
      errorHandler.error(0, 0, "Array index must be an integer");
      return TypeInfo::createVoid();
  }
  
  // Return the element type
  return *arrayType.base;
}

TypeInfo SemanticAnalyzer::visitMemberAccess(MemberAccessNode* node) {
  TypeInfo objectType = visitExpression(node->object.get());
  
  // Member access requires struct type (or pointer to struct with -> operator)
  if (node->op.type == TokenType::OP_DOT) {
      if (objectType.kind != TypeInfo::Kind::STRUCT) {
          errorHandler.error(node->op.line, node->op.column, 
                            "Left operand of '.' must be a struct");
          return TypeInfo::createVoid();
      }
  } else if (node->op.type == TokenType::OP_ARROW) {
      if (objectType.kind != TypeInfo::Kind::POINTER || 
          (objectType.base && objectType.base->kind != TypeInfo::Kind::STRUCT)) {
          errorHandler.error(node->op.line, node->op.column, 
                            "Left operand of '->' must be a pointer to a struct");
          return TypeInfo::createVoid();
      }
  }
  
  // In a real compiler, we would look up the member in the struct
  // and return its type. For now, we'll just return int as a placeholder.
  errorHandler.warning(node->op.line, node->op.column, 
                     "Struct member access not fully implemented");
  return TypeInfo::createInt();
}

TypeInfo SemanticAnalyzer::visitConditional(ConditionalNode* node) {
  TypeInfo condType = visitExpression(node->condition.get());
  
  // Condition must be scalar
  if (!condType.isScalar()) {
      errorHandler.error(0, 0, "Conditional operator requires scalar condition");
      return TypeInfo::createVoid();
  }
  
  TypeInfo trueType = visitExpression(node->trueExpr.get());
  TypeInfo falseType = visitExpression(node->falseExpr.get());
  
  // Result types must be compatible
  if (areTypesCompatible(trueType, falseType)) {
      return trueType;
  } else if (areTypesCompatible(falseType, trueType)) {
      return falseType;
  } else {
      errorHandler.error(0, 0, "Incompatible types in conditional expression");
      return TypeInfo::createVoid();
  }
}

TypeInfo SemanticAnalyzer::getTypeFromTypeNode(TypeNode* node) {
  TypeInfo::Kind kind;
  int size = 0;
  
  // Determine base type
  if (node->name.type == TokenType::KW_VOID) {
      kind = TypeInfo::Kind::VOID;
      size = 0;
  } else if (node->name.type == TokenType::KW_CHAR) {
      kind = TypeInfo::Kind::CHAR;
      size = 1;
  } else if (node->name.type == TokenType::KW_INT) {
      kind = TypeInfo::Kind::INT;
      size = 4;
  } else if (node->name.type == TokenType::KW_FLOAT) {
      kind = TypeInfo::Kind::FLOAT;
      size = 4;
  } else if (node->name.type == TokenType::KW_DOUBLE) {
      kind = TypeInfo::Kind::DOUBLE;
      size = 8;
  } else {
      // Unknown type, treat as void
      errorHandler.error(node->name.line, node->name.column, 
                       "Unknown type: " + node->name.lexeme);
      kind = TypeInfo::Kind::VOID;
      size = 0;
  }
  
  // Create base type
  TypeInfo result(kind, node->isConst, node->isVolatile, size);
  
  // Handle pointers
  if (node->isPointer) {
      for (int i = 0; i < node->pointerLevel; i++) {
          result = TypeInfo::createPointer(result);
      }
  }
  
  return result;
}

bool SemanticAnalyzer::areTypesCompatible(const TypeInfo& source, const TypeInfo& target) {
  // Identical types are compatible
  if (source.kind == target.kind) {
      if (source.kind == TypeInfo::Kind::ARRAY || 
          source.kind == TypeInfo::Kind::POINTER || 
          source.kind == TypeInfo::Kind::FUNCTION) {
          // For compound types, base types must be compatible
          return areTypesCompatible(*source.base, *target.base);
      }
      return true;
  }
  
  // Integer promotion
  if (source.kind == TypeInfo::Kind::CHAR && target.kind == TypeInfo::Kind::INT) {
      return true;
  }
  
  // Float promotion
  if (source.kind == TypeInfo::Kind::FLOAT && target.kind == TypeInfo::Kind::DOUBLE) {
      return true;
  }
  
  // Integer to float
  if (source.isInteger() && target.isFloatingPoint()) {
      return true;
  }
  
  // Array decays to pointer
  if (source.kind == TypeInfo::Kind::ARRAY && target.kind == TypeInfo::Kind::POINTER) {
      return areTypesCompatible(*source.base, *target.base);
  }
  
  return false;
}

TypeInfo SemanticAnalyzer::getCommonType(const TypeInfo& a, const TypeInfo& b) {
  // If types are identical, return either one
  if (a.kind == b.kind) {
      return a;
  }
  
  // If one is double, result is double
  if (a.kind == TypeInfo::Kind::DOUBLE || b.kind == TypeInfo::Kind::DOUBLE) {
      return TypeInfo::createDouble();
  }
  
  // If one is float, result is float
  if (a.kind == TypeInfo::Kind::FLOAT || b.kind == TypeInfo::Kind::FLOAT) {
      return TypeInfo::createFloat();
  }
  
  // If both are integers, result is the larger one
  if (a.isInteger() && b.isInteger()) {
      return (a.size >= b.size) ? a : b;
  }
  
  // Default to a
  return a;
}

} // namespace ccc