#ifndef CCC_CODEGEN_H
#define CCC_CODEGEN_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <stack>
#include "ast.h"
#include "error.h"
#include "coil/binary_format.h"
#include "coil/instruction_set.h"
#include "coil/type_system.h"

namespace ccc {

// Represents a variable during code generation
struct Variable {
  std::string name;
  uint16_t varId;  // COIL variable ID
  uint16_t type;   // COIL type
  
  Variable(const std::string& name, uint16_t varId, uint16_t type)
      : name(name), varId(varId), type(type) {}
};

// Code generator class
class CodeGenerator {
public:
  CodeGenerator(int optimizationLevel, ErrorHandler& errorHandler);
  
  // Generate COIL code from AST
  coil::CoilObject generate(ASTNode* root);
  
private:
  // State for code generation
  int optimizationLevel;
  ErrorHandler& errorHandler;
  coil::CoilObject coilObject;
  uint16_t textSectionIndex;
  uint16_t dataSectionIndex;
  uint16_t bssSectionIndex;
  
  // Variable tracking
  std::unordered_map<std::string, Variable> variables;
  uint16_t nextVarId;
  
  // Stack of active scopes for nested blocks
  std::stack<std::vector<std::string>> scopeStack;
  
  // Current function context
  std::string currentFunction;
  
  // Control flow labels
  int labelCounter;
  
  // Initialize COIL object (create sections, etc.)
  void initialize();
  
  // Generate various parts of the program
  void generateProgram(ProgramNode* node);
  void generateFunctionDeclaration(FunctionDeclarationNode* node);
  void generateVariableDeclaration(VariableDeclarationNode* node, bool isGlobal = false);
  void generateBlock(BlockNode* node);
  void generateStatement(StatementNode* node);
  void generateExpressionStatement(ExpressionStatementNode* node);
  void generateIfStatement(IfNode* node);
  void generateWhileStatement(WhileNode* node);
  void generateDoWhileStatement(DoWhileNode* node);
  void generateForStatement(ForNode* node);
  void generateReturnStatement(ReturnNode* node);
  void generateBreakStatement(BreakNode* node);
  void generateContinueStatement(ContinueNode* node);
  
  // Expression generation
  uint16_t generateExpression(ExpressionNode* node);
  uint16_t generateLiteral(LiteralNode* node);
  uint16_t generateVariable(VariableNode* node);
  uint16_t generateUnary(UnaryNode* node);
  uint16_t generateBinary(BinaryNode* node);
  uint16_t generateCall(CallNode* node);
  uint16_t generateArrayAccess(ArrayAccessNode* node);
  uint16_t generateMemberAccess(MemberAccessNode* node);
  uint16_t generateConditional(ConditionalNode* node);
  
  // Helper methods
  uint16_t getNextVarId() { return nextVarId++; }
  uint16_t createTempVar(uint16_t type);
  uint16_t translateType(TypeNode* typeNode);
  std::string generateLabel(const std::string& prefix);
  uint16_t addSymbol(const std::string& name, uint32_t attributes = 0, uint16_t sectionIndex = 0);
  void enterScope();
  void leaveScope();
  
  // Emit COIL instructions
  void emitInstruction(uint8_t opcode, const std::vector<coil::Operand>& operands);
  void emitScopeEnter();
  void emitScopeLeave();
  void emitVarDeclaration(uint16_t varId, uint16_t type, uint16_t initializer = 0);
  void emitLabel(const std::string& label);
  void emitJump(const std::string& label);
  void emitConditionalJump(const std::string& label, const std::string& condition);
};

} // namespace ccc

#endif // CCC_CODEGEN_H