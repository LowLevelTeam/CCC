#include "codegen.h"
#include <sstream>

namespace ccc {

CodeGenerator::CodeGenerator(int optimizationLevel, ErrorHandler& errorHandler)
    : optimizationLevel(optimizationLevel), errorHandler(errorHandler), 
      nextVarId(1), labelCounter(0) {
}

coil::CoilObject CodeGenerator::generate(ASTNode* root) {
    if (!root) {
        errorHandler.error(0, 0, "Empty AST");
        return coilObject;
    }
    
    // Initialize COIL object
    initialize();
    
    // Generate code based on the AST
    if (root->getNodeType() == "ProgramNode") {
        generateProgram(static_cast<ProgramNode*>(root));
    } else {
        errorHandler.error(0, 0, "Expected program node as root");
    }
    
    return coilObject;
}

void CodeGenerator::initialize() {
    // Create an empty COIL object
    coilObject = coil::CoilObject();
    
    // Set up the text section (code)
    coil::Symbol textSymbol;
    textSymbol.name = ".text";
    textSymbol.name_length = textSymbol.name.length();
    textSymbol.attributes = coil::SymbolFlags::GLOBAL;
    textSymbol.value = 0;
    textSymbol.section_index = 0;
    textSymbol.processor_type = 0x01; // CPU
    
    uint16_t textSymbolIndex = coilObject.addSymbol(textSymbol);
    
    coil::Section textSection;
    textSection.name_index = textSymbolIndex;
    textSection.attributes = coil::SectionFlags::EXECUTABLE | coil::SectionFlags::READABLE;
    textSection.offset = 0;
    textSection.size = 0;
    textSection.address = 0;
    textSection.alignment = 16;
    textSection.processor_type = 0x01; // CPU
    
    textSectionIndex = coilObject.addSection(textSection);
    
    // Set up the data section (initialized data)
    coil::Symbol dataSymbol;
    dataSymbol.name = ".data";
    dataSymbol.name_length = dataSymbol.name.length();
    dataSymbol.attributes = coil::SymbolFlags::GLOBAL;
    dataSymbol.value = 0;
    dataSymbol.section_index = 0;
    dataSymbol.processor_type = 0x01; // CPU
    
    uint16_t dataSymbolIndex = coilObject.addSymbol(dataSymbol);
    
    coil::Section dataSection;
    dataSection.name_index = dataSymbolIndex;
    dataSection.attributes = coil::SectionFlags::WRITABLE | coil::SectionFlags::READABLE | coil::SectionFlags::INITIALIZED;
    dataSection.offset = 0;
    dataSection.size = 0;
    dataSection.address = 0;
    dataSection.alignment = 16;
    dataSection.processor_type = 0x01; // CPU
    
    dataSectionIndex = coilObject.addSection(dataSection);
    
    // Set up the bss section (uninitialized data)
    coil::Symbol bssSymbol;
    bssSymbol.name = ".bss";
    bssSymbol.name_length = bssSymbol.name.length();
    bssSymbol.attributes = coil::SymbolFlags::GLOBAL;
    bssSymbol.value = 0;
    bssSymbol.section_index = 0;
    bssSymbol.processor_type = 0x01; // CPU
    
    uint16_t bssSymbolIndex = coilObject.addSymbol(bssSymbol);
    
    coil::Section bssSection;
    bssSection.name_index = bssSymbolIndex;
    bssSection.attributes = coil::SectionFlags::WRITABLE | coil::SectionFlags::READABLE | coil::SectionFlags::UNINITIALIZED;
    bssSection.offset = 0;
    bssSection.size = 0;
    bssSection.address = 0;
    bssSection.alignment = 16;
    bssSection.processor_type = 0x01; // CPU
    
    bssSectionIndex = coilObject.addSection(bssSection);
    
    // Initialize variable tracking
    variables.clear();
    nextVarId = 1;
    
    // Initialize scope stack
    while (!scopeStack.empty()) {
        scopeStack.pop();
    }
    
    // Initialize label counter
    labelCounter = 0;
    
    // Set up processor directive
    std::vector<coil::Operand> procOperands = {
        coil::Operand::createImmediate<uint8_t>(0x01) // CPU
    };
    emitInstruction(coil::Opcode::PROC, procOperands);
}

void CodeGenerator::generateProgram(ProgramNode* node) {
    // Process all declarations in the program
    for (const auto& declaration : node->declarations) {
        if (declaration->getNodeType() == "FunctionDeclarationNode") {
            generateFunctionDeclaration(static_cast<FunctionDeclarationNode*>(declaration.get()));
        } else if (declaration->getNodeType() == "VariableDeclarationNode") {
            generateVariableDeclaration(static_cast<VariableDeclarationNode*>(declaration.get()), true);
        }
        // Add other top-level declarations as needed
    }
}

void CodeGenerator::generateFunctionDeclaration(FunctionDeclarationNode* node) {
    // Set current function context
    currentFunction = node->name.lexeme;
    
    // Add function symbol
    uint16_t functionSymbol = addSymbol(node->name.lexeme, coil::SymbolFlags::GLOBAL | coil::SymbolFlags::FUNCTION, textSectionIndex);
    
    // Start defining the function
    emitLabel(node->name.lexeme);
    
    // Define function symbol with SYM instruction
    std::vector<coil::Operand> symOperands = {
        coil::Operand::createSymbol(functionSymbol)
    };
    emitInstruction(coil::Opcode::SYM, symOperands);
    
    // Generate function body if it exists
    if (node->body) {
        // Begin function scope
        enterScope();
        
        // Add parameters to the current scope
        for (size_t i = 0; i < node->parameters.size(); i++) {
            const auto& param = node->parameters[i];
            if (!param->name.lexeme.empty()) {
                uint16_t paramType = translateType(param->type.get());
                uint16_t paramVarId = getNextVarId();
                
                // Add parameter to variables map
                variables[param->name.lexeme] = Variable(param->name.lexeme, paramVarId, paramType);
                
                // Add to current scope for cleanup
                scopeStack.top().push_back(param->name.lexeme);
                
                // Emit variable declaration
                emitVarDeclaration(paramVarId, paramType);
                
                // Load parameter value from ABI
                std::vector<coil::Operand> movOperands = {
                    coil::Operand::createVariable(paramVarId),
                    coil::Operand::createImmediate<uint16_t>(coil::Type::ABICTL | coil::Type::PARAM),
                    coil::Operand::createImmediate<uint16_t>(static_cast<uint16_t>(i)) // Parameter index
                };
                emitInstruction(coil::Opcode::MOV, movOperands);
            }
        }
        
        // Generate code for the function body
        generateBlock(node->body.get());
        
        // If no explicit return statement in a non-void function, add one
        // In a real compiler, we'd check the function return type
        if (currentFunction == "main") {
            std::vector<coil::Operand> retOperands = {
                coil::Operand::createImmediate<uint16_t>(coil::Type::ABICTL | coil::Type::RET),
                coil::Operand::createImmediate<int32_t>(0)
            };
            emitInstruction(coil::Opcode::RET, retOperands);
        } else {
            // For non-main functions, just add a simple return
            std::vector<coil::Operand> retOperands;
            emitInstruction(coil::Opcode::RET, retOperands);
        }
        
        // End function scope
        leaveScope();
    }
    
    // Clear function context
    currentFunction = "";
}

void CodeGenerator::generateVariableDeclaration(VariableDeclarationNode* node, bool isGlobal) {
    // Get variable type
    uint16_t varType = translateType(node->type.get());
    
    if (isGlobal) {
        // Global variable - add to data or bss section
        uint16_t sectionIndex = node->initializer ? dataSectionIndex : bssSectionIndex;
        
        // Add variable symbol
        uint16_t symbolIndex = addSymbol(node->name.lexeme, coil::SymbolFlags::GLOBAL | coil::SymbolFlags::DATA, sectionIndex);
        
        // Global variables will be set up by the linker, so we're done for now
        // In a more complete implementation, we would add data directives for global variables
    } else {
        // Local variable - add to current scope
        uint16_t varId = getNextVarId();
        
        // Add variable to variables map
        variables[node->name.lexeme] = Variable(node->name.lexeme, varId, varType);
        
        // Add to current scope for cleanup
        scopeStack.top().push_back(node->name.lexeme);
        
        // Emit variable declaration
        if (node->initializer) {
            // Generate initializer expression
            uint16_t initVarId = generateExpression(node->initializer.get());
            
            // Emit variable declaration with initializer
            emitVarDeclaration(varId, varType, initVarId);
        } else {
            // Emit variable declaration without initializer
            emitVarDeclaration(varId, varType);
        }
    }
}

void CodeGenerator::generateBlock(BlockNode* node) {
    // Enter a new scope for the block
    emitScopeEnter();
    enterScope();
    
    // Generate code for each statement in the block
    for (const auto& stmt : node->statements) {
        generateStatement(stmt.get());
    }
    
    // Leave the block scope
    leaveScope();
    emitScopeLeave();
}

void CodeGenerator::generateStatement(StatementNode* node) {
    if (!node) return;
    
    if (node->getNodeType() == "BlockNode") {
        generateBlock(static_cast<BlockNode*>(node));
    } else if (node->getNodeType() == "ExpressionStatementNode") {
        generateExpressionStatement(static_cast<ExpressionStatementNode*>(node));
    } else if (node->getNodeType() == "VariableDeclarationNode") {
        generateVariableDeclaration(static_cast<VariableDeclarationNode*>(node));
    } else if (node->getNodeType() == "IfNode") {
        generateIfStatement(static_cast<IfNode*>(node));
    } else if (node->getNodeType() == "WhileNode") {
        generateWhileStatement(static_cast<WhileNode*>(node));
    } else if (node->getNodeType() == "DoWhileNode") {
        generateDoWhileStatement(static_cast<DoWhileNode*>(node));
    } else if (node->getNodeType() == "ForNode") {
        generateForStatement(static_cast<ForNode*>(node));
    } else if (node->getNodeType() == "ReturnNode") {
        generateReturnStatement(static_cast<ReturnNode*>(node));
    } else if (node->getNodeType() == "BreakNode") {
        generateBreakStatement(static_cast<BreakNode*>(node));
    } else if (node->getNodeType() == "ContinueNode") {
        generateContinueStatement(static_cast<ContinueNode*>(node));
    } else {
        errorHandler.error(0, 0, "Unknown statement type: " + node->getNodeType());
    }
}

void CodeGenerator::generateExpressionStatement(ExpressionStatementNode* node) {
    // Generate the expression - result is discarded in an expression statement
    generateExpression(node->expression.get());
}

void CodeGenerator::generateIfStatement(IfNode* node) {
    // Generate labels for branches
    std::string elseLabel = generateLabel("else");
    std::string endIfLabel = generateLabel("endif");
    
    // Generate condition expression
    uint16_t condVarId = generateExpression(node->condition.get());
    
    // Compare condition with 0 (false)
    std::vector<coil::Operand> cmpOperands = {
        coil::Operand::createVariable(condVarId),
        coil::Operand::createImmediate<int32_t>(0)
    };
    emitInstruction(coil::Opcode::CMP, cmpOperands);
    
    // Branch to else if condition is false
    std::vector<coil::Operand> branchOperands = {
        coil::Operand::createSymbol(addSymbol(elseLabel))
    };
    emitInstruction(coil::Opcode::BR, branchOperands);
    
    // Generate then branch
    generateStatement(node->thenBranch.get());
    
    // Jump to end (skip else branch)
    std::vector<coil::Operand> endBranchOperands = {
        coil::Operand::createSymbol(addSymbol(endIfLabel))
    };
    emitInstruction(coil::Opcode::BR, endBranchOperands);
    
    // Else branch
    emitLabel(elseLabel);
    if (node->elseBranch) {
        generateStatement(node->elseBranch.get());
    }
    
    // End of if statement
    emitLabel(endIfLabel);
}

void CodeGenerator::generateWhileStatement(WhileNode* node) {
    // Generate labels for loop control
    std::string startLabel = generateLabel("while_start");
    std::string endLabel = generateLabel("while_end");
    
    // Start of loop
    emitLabel(startLabel);
    
    // Generate condition expression
    uint16_t condVarId = generateExpression(node->condition.get());
    
    // Compare condition with 0 (false)
    std::vector<coil::Operand> cmpOperands = {
        coil::Operand::createVariable(condVarId),
        coil::Operand::createImmediate<int32_t>(0)
    };
    emitInstruction(coil::Opcode::CMP, cmpOperands);
    
    // Branch to end if condition is false
    std::vector<coil::Operand> branchOperands = {
        coil::Operand::createSymbol(addSymbol(endLabel))
    };
    emitInstruction(coil::Opcode::BR, branchOperands);
    
    // Generate loop body
    generateStatement(node->body.get());
    
    // Jump back to start
    std::vector<coil::Operand> loopBranchOperands = {
        coil::Operand::createSymbol(addSymbol(startLabel))
    };
    emitInstruction(coil::Opcode::BR, loopBranchOperands);
    
    // End of loop
    emitLabel(endLabel);
}

void CodeGenerator::generateDoWhileStatement(DoWhileNode* node) {
    // Generate labels for loop control
    std::string startLabel = generateLabel("dowhile_start");
    std::string conditionLabel = generateLabel("dowhile_condition");
    std::string endLabel = generateLabel("dowhile_end");
    
    // Start of loop
    emitLabel(startLabel);
    
    // Generate loop body
    generateStatement(node->body.get());
    
    // Condition check
    emitLabel(conditionLabel);
    uint16_t condVarId = generateExpression(node->condition.get());
    
    // Compare condition with 0 (false)
    std::vector<coil::Operand> cmpOperands = {
        coil::Operand::createVariable(condVarId),
        coil::Operand::createImmediate<int32_t>(0)
    };
    emitInstruction(coil::Opcode::CMP, cmpOperands);
    
    // Branch to end if condition is false
    std::vector<coil::Operand> branchOperands = {
        coil::Operand::createSymbol(addSymbol(endLabel))
    };
    emitInstruction(coil::Opcode::BR, branchOperands);
    
    // Jump back to start
    std::vector<coil::Operand> loopBranchOperands = {
        coil::Operand::createSymbol(addSymbol(startLabel))
    };
    emitInstruction(coil::Opcode::BR, loopBranchOperands);
    
    // End of loop
    emitLabel(endLabel);
}

void CodeGenerator::generateForStatement(ForNode* node) {
    // Generate labels for loop control
    std::string startLabel = generateLabel("for_start");
    std::string incrementLabel = generateLabel("for_increment");
    std::string endLabel = generateLabel("for_end");
    
    // Enter loop scope (for initializer variables)
    emitScopeEnter();
    enterScope();
    
    // Generate initializer
    if (node->initializer) {
        generateStatement(node->initializer.get());
    }
    
    // Start of loop
    emitLabel(startLabel);
    
    // Generate condition if present
    if (node->condition) {
        uint16_t condVarId = generateExpression(node->condition.get());
        
        // Compare condition with 0 (false)
        std::vector<coil::Operand> cmpOperands = {
            coil::Operand::createVariable(condVarId),
            coil::Operand::createImmediate<int32_t>(0)
        };
        emitInstruction(coil::Opcode::CMP, cmpOperands);
        
        // Branch to end if condition is false
        std::vector<coil::Operand> branchOperands = {
            coil::Operand::createSymbol(addSymbol(endLabel))
        };
        emitInstruction(coil::Opcode::BR, branchOperands);
    }
    
    // Generate loop body
    generateStatement(node->body.get());
    
    // Increment step
    emitLabel(incrementLabel);
    if (node->increment) {
        generateExpression(node->increment.get());
    }
    
    // Jump back to start
    std::vector<coil::Operand> loopBranchOperands = {
        coil::Operand::createSymbol(addSymbol(startLabel))
    };
    emitInstruction(coil::Opcode::BR, loopBranchOperands);
    
    // End of loop
    emitLabel(endLabel);
    
    // Leave loop scope
    leaveScope();
    emitScopeLeave();
}

void CodeGenerator::generateReturnStatement(ReturnNode* node) {
    // Generate return value if present
    if (node->value) {
        uint16_t returnVarId = generateExpression(node->value.get());
        
        // Return with value
        std::vector<coil::Operand> retOperands = {
            coil::Operand::createImmediate<uint16_t>(coil::Type::ABICTL | coil::Type::RET),
            coil::Operand::createVariable(returnVarId)
        };
        emitInstruction(coil::Opcode::RET, retOperands);
    } else {
        // Return without value
        std::vector<coil::Operand> retOperands = {
            coil::Operand::createImmediate<uint16_t>(coil::Type::ABICTL | coil::Type::RET)
        };
        emitInstruction(coil::Opcode::RET, retOperands);
    }
}

void CodeGenerator::generateBreakStatement(BreakNode* node) {
    // In a real compiler, we would keep track of loop/switch contexts
    // and jump to the appropriate end label
    errorHandler.warning(0, 0, "Break statement not fully implemented");
}

void CodeGenerator::generateContinueStatement(ContinueNode* node) {
    // In a real compiler, we would keep track of loop contexts
    // and jump to the appropriate continuation point
    errorHandler.warning(0, 0, "Continue statement not fully implemented");
}

uint16_t CodeGenerator::generateExpression(ExpressionNode* node) {
    if (!node) {
        errorHandler.error(0, 0, "Null expression");
        return 0;
    }
    
    if (node->getNodeType() == "LiteralNode") {
        return generateLiteral(static_cast<LiteralNode*>(node));
    } else if (node->getNodeType() == "VariableNode") {
        return generateVariable(static_cast<VariableNode*>(node));
    } else if (node->getNodeType() == "UnaryNode") {
        return generateUnary(static_cast<UnaryNode*>(node));
    } else if (node->getNodeType() == "BinaryNode") {
        return generateBinary(static_cast<BinaryNode*>(node));
    } else if (node->getNodeType() == "CallNode") {
        return generateCall(static_cast<CallNode*>(node));
    } else if (node->getNodeType() == "ArrayAccessNode") {
        return generateArrayAccess(static_cast<ArrayAccessNode*>(node));
    } else if (node->getNodeType() == "MemberAccessNode") {
        return generateMemberAccess(static_cast<MemberAccessNode*>(node));
    } else if (node->getNodeType() == "ConditionalNode") {
        return generateConditional(static_cast<ConditionalNode*>(node));
    } else {
        errorHandler.error(0, 0, "Unknown expression type: " + node->getNodeType());
        return 0;
    }
}

uint16_t CodeGenerator::generateLiteral(LiteralNode* node) {
    // Create a temporary variable for the literal
    uint16_t resultVarId = getNextVarId();
    
    switch (node->token.type) {
        case TokenType::INTEGER_LITERAL: {
            // Parse integer value
            int value = std::stoi(node->token.lexeme);
            
            // Declare a temporary variable
            emitVarDeclaration(resultVarId, coil::Type::INT32);
            
            // Move literal value to variable
            std::vector<coil::Operand> movOperands = {
                coil::Operand::createVariable(resultVarId),
                coil::Operand::createImmediate<int32_t>(value)
            };
            emitInstruction(coil::Opcode::MOV, movOperands);
            break;
        }
        case TokenType::FLOAT_LITERAL: {
            // Parse float value
            float value = std::stof(node->token.lexeme);
            
            // Declare a temporary variable
            emitVarDeclaration(resultVarId, coil::Type::FP32);
            
            // Move literal value to variable
            std::vector<coil::Operand> movOperands = {
                coil::Operand::createVariable(resultVarId),
                coil::Operand::createImmediate<float>(value)
            };
            emitInstruction(coil::Opcode::MOV, movOperands);
            break;
        }
        case TokenType::CHAR_LITERAL: {
            // Parse character value (handling escape sequences)
            char value = node->token.lexeme[1]; // Skip the opening quote
            if (value == '\\' && node->token.lexeme.length() > 2) {
                // Handle escape sequence
                char escape = node->token.lexeme[2];
                switch (escape) {
                    case 'n': value = '\n'; break;
                    case 't': value = '\t'; break;
                    case 'r': value = '\r'; break;
                    case '0': value = '\0'; break;
                    case '\\': value = '\\'; break;
                    case '\'': value = '\''; break;
                    case '\"': value = '\"'; break;
                    default: value = escape;
                }
            }
            
            // Declare a temporary variable
            emitVarDeclaration(resultVarId, coil::Type::INT8);
            
            // Move literal value to variable
            std::vector<coil::Operand> movOperands = {
                coil::Operand::createVariable(resultVarId),
                coil::Operand::createImmediate<int8_t>(value)
            };
            emitInstruction(coil::Opcode::MOV, movOperands);
            break;
        }
        case TokenType::STRING_LITERAL: {
            // Handle string literals (would be implemented with static data section)
            errorHandler.warning(0, 0, "String literals not fully implemented");
            
            // For now, just create a placeholder char pointer
            emitVarDeclaration(resultVarId, coil::Type::PTR);
            
            // Set to null for now
            std::vector<coil::Operand> movOperands = {
                coil::Operand::createVariable(resultVarId),
                coil::Operand::createImmediate<int32_t>(0)
            };
            emitInstruction(coil::Opcode::MOV, movOperands);
            break;
        }
        default:
            errorHandler.error(0, 0, "Unknown literal type");
            return 0;
    }
    
    return resultVarId;
}

uint16_t CodeGenerator::generateVariable(VariableNode* node) {
    // Look up the variable in our map
    const std::string& name = node->name.lexeme;
    auto it = variables.find(name);
    
    if (it == variables.end()) {
        errorHandler.error(node->name.line, node->name.column, "Undefined variable: " + name);
        return 0;
    }
    
    // For variable access, we just return the variable ID
    return it->second.varId;
}

uint16_t CodeGenerator::generateUnary(UnaryNode* node) {
    // Generate the operand
    uint16_t operandVarId = generateExpression(node->operand.get());
    
    // Create a temporary variable for the result
    uint16_t resultVarId = getNextVarId();
    
    // The type of the result depends on the operation
    uint16_t resultType = coil::Type::INT32; // Default
    
    // Determine the operation based on the operator
    switch (node->op.type) {
        case TokenType::OP_MINUS: {
            // Unary minus (negation)
            emitVarDeclaration(resultVarId, resultType);
            
            std::vector<coil::Operand> negOperands = {
                coil::Operand::createVariable(resultVarId),
                coil::Operand::createVariable(operandVarId)
            };
            emitInstruction(coil::Opcode::NEG, negOperands);
            break;
        }
        case TokenType::OP_PLUS: {
            // Unary plus (no-op)
            emitVarDeclaration(resultVarId, resultType);
            
            std::vector<coil::Operand> movOperands = {
                coil::Operand::createVariable(resultVarId),
                coil::Operand::createVariable(operandVarId)
            };
            emitInstruction(coil::Opcode::MOV, movOperands);
            break;
        }
        case TokenType::OP_EXCLAMATION: {
            // Logical NOT
            emitVarDeclaration(resultVarId, resultType);
            
            // COIL doesn't have a direct NOT instruction for booleans, so we do it with comparison
            std::vector<coil::Operand> cmpOperands = {
                coil::Operand::createVariable(operandVarId),
                coil::Operand::createImmediate<int32_t>(0)
            };
            emitInstruction(coil::Opcode::CMP, cmpOperands);
            
            // Move 1 to result if operand was 0 (opposite of boolean value)
            std::vector<coil::Operand> movOperands = {
                coil::Operand::createVariable(resultVarId),
                coil::Operand::createImmediate<int32_t>(1)
            };
            emitInstruction(coil::Opcode::MOV, movOperands);
            
            // TODO: This actually needs to be conditional using BR instruction
            break;
        }
        case TokenType::OP_TILDE: {
            // Bitwise NOT
            emitVarDeclaration(resultVarId, resultType);
            
            std::vector<coil::Operand> notOperands = {
                coil::Operand::createVariable(resultVarId),
                coil::Operand::createVariable(operandVarId)
            };
            emitInstruction(coil::Opcode::NOT, notOperands);
            break;
        }
        case TokenType::OP_PLUS_PLUS: {
            // Increment
            // Create a copy of the operand
            emitVarDeclaration(resultVarId, resultType);
            
            std::vector<coil::Operand> movOperands = {
                coil::Operand::createVariable(resultVarId),
                coil::Operand::createVariable(operandVarId)
            };
            emitInstruction(coil::Opcode::MOV, movOperands);
            
            // Increment the operand
            std::vector<coil::Operand> incOperands = {
                coil::Operand::createVariable(operandVarId)
            };
            emitInstruction(coil::Opcode::INC, incOperands);
            break;
        }
        case TokenType::OP_MINUS_MINUS: {
            // Decrement
            // Create a copy of the operand
            emitVarDeclaration(resultVarId, resultType);
            
            std::vector<coil::Operand> movOperands = {
                coil::Operand::createVariable(resultVarId),
                coil::Operand::createVariable(operandVarId)
            };
            emitInstruction(coil::Opcode::MOV, movOperands);
            
            // Decrement the operand
            std::vector<coil::Operand> decOperands = {
                coil::Operand::createVariable(operandVarId)
            };
            emitInstruction(coil::Opcode::DEC, decOperands);
            break;
        }
        case TokenType::OP_STAR: {
            // Dereference operator (*)
            // For now, just return the operand (which should be a pointer)
            // In a real compiler, we would generate a load instruction
            errorHandler.warning(0, 0, "Dereference operator not fully implemented");
            return operandVarId;
        }
        case TokenType::OP_AMPERSAND: {
            // Address-of operator (&)
            // For now, just return the operand
            // In a real compiler, we would generate an address computation
            errorHandler.warning(0, 0, "Address-of operator not fully implemented");
            return operandVarId;
        }
        default:
            errorHandler.error(node->op.line, node->op.column, 
                              "Unknown unary operator: " + node->op.lexeme);
            return 0;
    }
    
    return resultVarId;
}

uint16_t CodeGenerator::generateBinary(BinaryNode* node) {
    // Generate left and right operands
    uint16_t leftVarId = generateExpression(node->left.get());
    uint16_t rightVarId = generateExpression(node->right.get());
    
    // Create a temporary variable for the result
    uint16_t resultVarId = getNextVarId();
    
    // The type of the result depends on the operation
    uint16_t resultType = coil::Type::INT32; // Default
    
    // Determine the operation based on the operator
    switch (node->op.type) {
        case TokenType::OP_PLUS: {
            // Addition
            emitVarDeclaration(resultVarId, resultType);
            
            std::vector<coil::Operand> addOperands = {
                coil::Operand::createVariable(resultVarId),
                coil::Operand::createVariable(leftVarId),
                coil::Operand::createVariable(rightVarId)
            };
            emitInstruction(coil::Opcode::ADD, addOperands);
            break;
        }
        case TokenType::OP_MINUS: {
            // Subtraction
            emitVarDeclaration(resultVarId, resultType);
            
            std::vector<coil::Operand> subOperands = {
                coil::Operand::createVariable(resultVarId),
                coil::Operand::createVariable(leftVarId),
                coil::Operand::createVariable(rightVarId)
            };
            emitInstruction(coil::Opcode::SUB, subOperands);
            break;
        }
        case TokenType::OP_STAR: {
            // Multiplication
            emitVarDeclaration(resultVarId, resultType);
            
            std::vector<coil::Operand> mulOperands = {
                coil::Operand::createVariable(resultVarId),
                coil::Operand::createVariable(leftVarId),
                coil::Operand::createVariable(rightVarId)
            };
            emitInstruction(coil::Opcode::MUL, mulOperands);
            break;
        }
        case TokenType::OP_SLASH: {
            // Division
            emitVarDeclaration(resultVarId, resultType);
            
            std::vector<coil::Operand> divOperands = {
                coil::Operand::createVariable(resultVarId),
                coil::Operand::createVariable(leftVarId),
                coil::Operand::createVariable(rightVarId)
            };
            emitInstruction(coil::Opcode::DIV, divOperands);
            break;
        }
        case TokenType::OP_PERCENT: {
            // Modulo
            emitVarDeclaration(resultVarId, resultType);
            
            std::vector<coil::Operand> modOperands = {
                coil::Operand::createVariable(resultVarId),
                coil::Operand::createVariable(leftVarId),
                coil::Operand::createVariable(rightVarId)
            };
            emitInstruction(coil::Opcode::MOD, modOperands);
            break;
        }
        case TokenType::OP_EQUALS: {
            // Assignment
            // For assignment, we just copy the right value to the left variable
            std::vector<coil::Operand> movOperands = {
                coil::Operand::createVariable(leftVarId),
                coil::Operand::createVariable(rightVarId)
            };
            emitInstruction(coil::Opcode::MOV, movOperands);
            
            // Assignment expressions return the assigned value
            return leftVarId;
        }
        // Add other binary operators as needed (comparison, bitwise, logical, etc.)
        default:
            errorHandler.error(node->op.line, node->op.column, 
                              "Binary operator not implemented: " + node->op.lexeme);
            return 0;
    }
    
    return resultVarId;
}

uint16_t CodeGenerator::generateCall(CallNode* node) {
    // Generate the callee
    // For simplicity, assume callee is a variable (function name)
    std::string funcName;
    if (node->callee->getNodeType() == "VariableNode") {
        funcName = static_cast<VariableNode*>(node->callee.get())->name.lexeme;
    } else {
        errorHandler.error(0, 0, "Only simple function calls supported");
        return 0;
    }
    
    // Generate arguments
    std::vector<uint16_t> argVarIds;
    for (const auto& arg : node->arguments) {
        argVarIds.push_back(generateExpression(arg.get()));
    }
    
    // Create a variable for the return value
    uint16_t returnVarId = getNextVarId();
    emitVarDeclaration(returnVarId, coil::Type::INT32); // Assume int return type
    
    // Create the CALL instruction operands
    std::vector<coil::Operand> callOperands;
    
    // First operand is the function symbol
    uint16_t funcSymbol = addSymbol(funcName);
    callOperands.push_back(coil::Operand::createSymbol(funcSymbol));
    
    // ABI control parameter
    callOperands.push_back(coil::Operand::createImmediate<uint16_t>(coil::Type::ABICTL | coil::Type::PARAM));
    
    // Add all arguments
    for (uint16_t argId : argVarIds) {
        callOperands.push_back(coil::Operand::createVariable(argId));
    }
    
    // Emit the call instruction
    emitInstruction(coil::Opcode::CALL, callOperands);
    
    // Get the return value
    std::vector<coil::Operand> movOperands = {
        coil::Operand::createVariable(returnVarId),
        coil::Operand::createImmediate<uint16_t>(coil::Type::ABICTL | coil::Type::RET)
    };
    emitInstruction(coil::Opcode::MOV, movOperands);
    
    return returnVarId;
}

uint16_t CodeGenerator::generateArrayAccess(ArrayAccessNode* node) {
    // Generate array and index expressions
    uint16_t arrayVarId = generateExpression(node->array.get());
    uint16_t indexVarId = generateExpression(node->index.get());
    
    // Create a result variable
    uint16_t resultVarId = getNextVarId();
    emitVarDeclaration(resultVarId, coil::Type::INT32); // Assume int array elements
    
    // Emit the array access instruction
    std::vector<coil::Operand> indexOperands = {
        coil::Operand::createVariable(resultVarId),
        coil::Operand::createVariable(arrayVarId),
        coil::Operand::createVariable(indexVarId)
    };
    emitInstruction(coil::Opcode::INDEX, indexOperands);
    
    return resultVarId;
}

uint16_t CodeGenerator::generateMemberAccess(MemberAccessNode* node) {
    // Not implemented in this simple version
    errorHandler.warning(0, 0, "Member access not implemented");
    return 0;
}

uint16_t CodeGenerator::generateConditional(ConditionalNode* node) {
    // Generate condition
    uint16_t condVarId = generateExpression(node->condition.get());
    
    // Generate labels for the branches
    std::string falseLabel = generateLabel("cond_false");
    std::string endLabel = generateLabel("cond_end");
    
    // Create a result variable
    uint16_t resultVarId = getNextVarId();
    emitVarDeclaration(resultVarId, coil::Type::INT32); // Assume int result
    
    // Compare condition with zero
    std::vector<coil::Operand> cmpOperands = {
        coil::Operand::createVariable(condVarId),
        coil::Operand::createImmediate<int32_t>(0)
    };
    emitInstruction(coil::Opcode::CMP, cmpOperands);
    
    // Branch to false part if condition is false
    std::vector<coil::Operand> branchOperands = {
        coil::Operand::createSymbol(addSymbol(falseLabel))
    };
    emitInstruction(coil::Opcode::BR, branchOperands);
    
    // True part
    uint16_t trueVarId = generateExpression(node->trueExpr.get());
    std::vector<coil::Operand> trueMovOperands = {
        coil::Operand::createVariable(resultVarId),
        coil::Operand::createVariable(trueVarId)
    };
    emitInstruction(coil::Opcode::MOV, trueMovOperands);
    
    // Jump to end
    std::vector<coil::Operand> endBranchOperands = {
        coil::Operand::createSymbol(addSymbol(endLabel))
    };
    emitInstruction(coil::Opcode::BR, endBranchOperands);
    
    // False part
    emitLabel(falseLabel);
    uint16_t falseVarId = generateExpression(node->falseExpr.get());
    std::vector<coil::Operand> falseMovOperands = {
        coil::Operand::createVariable(resultVarId),
        coil::Operand::createVariable(falseVarId)
    };
    emitInstruction(coil::Opcode::MOV, falseMovOperands);
    
    // End of conditional
    emitLabel(endLabel);
    
    return resultVarId;
}

// Helper methods

uint16_t CodeGenerator::createTempVar(uint16_t type) {
    uint16_t varId = getNextVarId();
    emitVarDeclaration(varId, type);
    return varId;
}

uint16_t CodeGenerator::translateType(TypeNode* typeNode) {
    // Map AST type to COIL type
    if (typeNode->name.type == TokenType::KW_INT) {
        if (typeNode->isPointer) {
            return coil::Type::PTR;
        }
        return coil::Type::INT32;
    } else if (typeNode->name.type == TokenType::KW_CHAR) {
        if (typeNode->isPointer) {
            return coil::Type::PTR;
        }
        return coil::Type::INT8;
    } else if (typeNode->name.type == TokenType::KW_FLOAT) {
        return coil::Type::FP32;
    } else if (typeNode->name.type == TokenType::KW_DOUBLE) {
        return coil::Type::FP64;
    } else if (typeNode->name.type == TokenType::KW_VOID) {
        if (typeNode->isPointer) {
            return coil::Type::PTR;
        }
        return coil::Type::VOID;
    }
    
    // Default to int
    errorHandler.warning(typeNode->name.line, typeNode->name.column, 
                       "Unknown type '" + typeNode->name.lexeme + "', defaulting to int");
    return coil::Type::INT32;
}

std::string CodeGenerator::generateLabel(const std::string& prefix) {
    std::stringstream ss;
    ss << prefix << "_" << labelCounter++;
    return ss.str();
}

uint16_t CodeGenerator::addSymbol(const std::string& name, uint32_t attributes, uint16_t sectionIndex) {
    // Check if the symbol already exists
    uint16_t existingSymbol = coilObject.findSymbol(name);
    if (existingSymbol != UINT16_MAX) {
        return existingSymbol;
    }
    
    // Create a new symbol
    coil::Symbol symbol;
    symbol.name = name;
    symbol.name_length = name.length();
    symbol.attributes = attributes;
    symbol.value = 0;
    symbol.section_index = sectionIndex;
    symbol.processor_type = 0x01; // CPU
    
    return coilObject.addSymbol(symbol);
}

void CodeGenerator::enterScope() {
    // Push a new scope onto the stack
    scopeStack.push({});
}

void CodeGenerator::leaveScope() {
    // Clean up variables in the current scope
    if (!scopeStack.empty()) {
        for (const std::string& varName : scopeStack.top()) {
            variables.erase(varName);
        }
        scopeStack.pop();
    }
}

// Instruction emission methods

void CodeGenerator::emitInstruction(uint8_t opcode, const std::vector<coil::Operand>& operands) {
    // Create an instruction
    coil::Instruction instruction(opcode, operands);
    
    // Add it to the text section
    coilObject.addInstruction(textSectionIndex, instruction);
}

void CodeGenerator::emitScopeEnter() {
    // Emit SCOPEE instruction
    std::vector<coil::Operand> operands;
    emitInstruction(coil::Opcode::SCOPEE, operands);
}

void CodeGenerator::emitScopeLeave() {
    // Emit SCOPEL instruction
    std::vector<coil::Operand> operands;
    emitInstruction(coil::Opcode::SCOPEL, operands);
}

void CodeGenerator::emitVarDeclaration(uint16_t varId, uint16_t type, uint16_t initializer) {
    // Create VAR instruction operands
    std::vector<coil::Operand> varOperands = {
        coil::Operand::createVariable(varId),
        coil::Operand::createImmediate<uint16_t>(type)
    };
    
    // Add initializer if provided
    if (initializer != 0) {
        varOperands.push_back(coil::Operand::createVariable(initializer));
    }
    
    // Emit the VAR instruction
    emitInstruction(coil::Opcode::VAR, varOperands);
}

void CodeGenerator::emitLabel(const std::string& label) {
    // Find or create the symbol for this label
    uint16_t symbolIndex = addSymbol(label, 0, textSectionIndex);
    
    // Create the SYM instruction
    std::vector<coil::Operand> symOperands = {
        coil::Operand::createSymbol(symbolIndex)
    };
    
    // Emit the SYM instruction
    emitInstruction(coil::Opcode::SYM, symOperands);
}

void CodeGenerator::emitJump(const std::string& label) {
    // Find or create the symbol for the target label
    uint16_t symbolIndex = addSymbol(label);
    
    // Create the BR instruction
    std::vector<coil::Operand> brOperands = {
        coil::Operand::createSymbol(symbolIndex)
    };
    
    // Emit the BR instruction
    emitInstruction(coil::Opcode::BR, brOperands);
}

void CodeGenerator::emitConditionalJump(const std::string& label, const std::string& condition) {
    // Find or create the symbol for the target label
    uint16_t symbolIndex = addSymbol(label);
    
    // Create the BR instruction with condition
    std::vector<coil::Operand> brOperands = {
        coil::Operand::createSymbol(symbolIndex)
        // We'd add condition operands here in a full implementation
    };
    
    // Emit the BR instruction
    emitInstruction(coil::Opcode::BR, brOperands);
}

} // namespace ccc