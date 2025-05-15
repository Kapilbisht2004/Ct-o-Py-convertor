#include "Transpiler.h"
#include <sstream>
#include <optional> // For optional return values

// Helper function (can be kept the same as the previous version)
// Tries to extract a simple numeric literal from an expression node.
// Returns std::nullopt if not a simple number.
std::optional<long long> getNumericLiteralValue(const shared_ptr<ASTNode>& exprNode) {
    if (exprNode && exprNode->type_name == "NumberNode") {
        auto num = dynamic_pointer_cast<NumberNode>(exprNode);
        if (num) {
            // Assuming NumberNode::getValue() returns a type convertible to long long.
            // Adjust if it returns string or requires different conversion.
            try {
                // If getValue returns std::string
                // return std::stoll(num->getValueString()); 
                // If getValue returns int/double
                return static_cast<long long>(num->getValue());
            } catch (const std::exception& e) {
                // Handle potential conversion errors if getValue() returns string
                return std::nullopt;
            }
        }
    }
    return std::nullopt;
}

// Helper to get the name from an IdentifierNode
std::optional<std::string> getIdentifierName(const shared_ptr<ASTNode>& node) {
    if (node && node->type_name == "IdentifierNode") {
        auto idNode = dynamic_pointer_cast<IdentifierNode>(node);
        if (idNode) return idNode->getName();
    }
    return std::nullopt;
}


string Transpiler::generateCode(const shared_ptr<ASTNode>& node, int indentLevel) {
    if (!node) {
        return "";
    }

    ostringstream out;
    const string& type = node->type_name;

    // ... (other node types remain the same) ...

    else if (type == "ForNode") {
        auto forNode = dynamic_pointer_cast<ForNode>(node);
        if (!forNode) { out << indent(indentLevel) << "# Error: Cast failed for ForNode\n"; return out.str(); }

        bool successfullyTranspiledToRange = false;
        string loopVarName;
        optional<long long> startValueOpt;
        optional<long long> endConditionValueOpt; // The value in the condition, e.g., 10 in "i < 10"
        optional<long long> stepValueOpt;
        string conditionOperator;


        // --- Attempt to extract elements for range() ---

        // 1. Analyze Initializer (e.g., "int i = 0" or "i = 0")
        shared_ptr<ASTNode> initNode = forNode->getInitializer();
        if (initNode) {
            shared_ptr<ASTNode> initValueExprNode;
            if (initNode->type_name == "VariableDeclarationNode") {
                auto varDecl = dynamic_pointer_cast<VariableDeclarationNode>(initNode);
                if (varDecl) {
                    loopVarName = varDecl->getName();
                    initValueExprNode = varDecl->getInitializer();
                }
            } else if (initNode->type_name == "AssignmentStatementNode") {
                auto assignStmt = dynamic_pointer_cast<AssignmentStatementNode>(initNode);
                if (assignStmt && assignStmt->getAssignment()) {
                    auto assign = dynamic_pointer_cast<AssignmentNode>(assignStmt->getAssignment());
                    if (assign) {
                        // We are assuming target of assignment is a simple identifier name
                        loopVarName = assign->getTargetName();
                        initValueExprNode = assign->getValue();
                    }
                }
            }
            if (!loopVarName.empty() && initValueExprNode) {
                startValueOpt = getNumericLiteralValue(initValueExprNode);
            }
        }

        // 2. Analyze Condition (e.g., "i < 10")
        shared_ptr<ASTNode> condNode = forNode->getCondition();
        if (condNode && condNode->type_name == "BinaryExpressionNode" && !loopVarName.empty()) {
            auto binCond = dynamic_pointer_cast<BinaryExpressionNode>(condNode);
            if (binCond) {
                auto leftOpNameOpt = getIdentifierName(binCond->getLeft());
                auto rightIsLiteral = getNumericLiteralValue(binCond->getRight());

                if (leftOpNameOpt && *leftOpNameOpt == loopVarName && rightIsLiteral) {
                    conditionOperator = binCond->getOperator();
                    endConditionValueOpt = rightIsLiteral;
                } else {
                    // Check for (literal < varName) - less common for canonical for-loops
                    auto rightOpNameOpt = getIdentifierName(binCond->getRight());
                    auto leftIsLiteral = getNumericLiteralValue(binCond->getLeft());
                    if (rightOpNameOpt && *rightOpNameOpt == loopVarName && leftIsLiteral){
                        // Need to "flip" the operator, e.g., 5 > i  is same as i < 5
                        string op = binCond->getOperator();
                        if(op == "<") conditionOperator = ">";
                        else if(op == "<=") conditionOperator = ">=";
                        else if(op == ">") conditionOperator = "<";
                        else if(op == ">=") conditionOperator = "<=";
                        else if(op == "==") conditionOperator = "=="; // '==' and '!=' remain the same logic-wise
                        else if(op == "!=") conditionOperator = "!=";
                        endConditionValueOpt = leftIsLiteral;
                    }
                }
            }
        }
        
        // 3. Analyze Increment/Decrement (e.g., "i++", "i = i + 1", "i += 1")
        shared_ptr<ASTNode> incrNode = forNode->getIncrement(); // This is often an Expression, not a Statement
        if (incrNode && !loopVarName.empty()) {
            // Pattern: i++ or ++i (UnaryExpressionNode)
            if (incrNode->type_name == "UnaryExpressionNode") {
                auto unaryIncr = dynamic_pointer_cast<UnaryExpressionNode>(incrNode);
                if (unaryIncr) {
                    auto operandNameOpt = getIdentifierName(unaryIncr->getOperand());
                    if (operandNameOpt && *operandNameOpt == loopVarName) {
                        if (unaryIncr->getOperator() == "++") stepValueOpt = 1;
                        else if (unaryIncr->getOperator() == "--") stepValueOpt = -1;
                    }
                }
            }
            // Pattern: i = i + N  or i = i - N (AssignmentNode, where value is BinaryExpression)
            else if (incrNode->type_name == "AssignmentNode") {
                 auto assignIncr = dynamic_pointer_cast<AssignmentNode>(incrNode);
                 if(assignIncr && assignIncr->getTargetName() == loopVarName) {
                    auto valueNode = assignIncr->getValue();
                    if (valueNode && valueNode->type_name == "BinaryExpressionNode") {
                        auto binVal = dynamic_pointer_cast<BinaryExpressionNode>(valueNode);
                        if (binVal) {
                            auto leftIsLoopVar = getIdentifierName(binVal->getLeft());
                            auto rightIsLiteral = getNumericLiteralValue(binVal->getRight());
                            if (leftIsLoopVar && *leftIsLoopVar == loopVarName && rightIsLiteral) {
                                if (binVal->getOperator() == "+") stepValueOpt = *rightIsLiteral;
                                else if (binVal->getOperator() == "-") stepValueOpt = -(*rightIsLiteral);
                            }
                        }
                    }
                 }
            }
            // Pattern: i += N or i -= N (Could be AssignmentNode with specific operators like "+=","-=")
            // Your current AST might not distinguish `i = i + 1` from `i += 1` at this level unless
            // AssignmentNode itself stores the operator. If `i += N` is an AssignmentNode with operator "+=":
            else if (incrNode->type_name == "AssignmentNode") { // Re-check, more general AssignmentNode like +=
                auto assignIncr = dynamic_pointer_cast<AssignmentNode>(incrNode);
                 // Let's assume AssignmentNode has getOperator() like BinaryExpressionNode
                 // if (assignIncr && assignIncr->getTargetName() == loopVarName && 
                 //    (assignIncr->getOperator() == "+=" || assignIncr->getOperator() == "-=")) {
                 //    auto literalValOpt = getNumericLiteralValue(assignIncr->getValue());
                 //    if (literalValOpt) {
                 //        if (assignIncr->getOperator() == "+=") stepValueOpt = *literalValOpt;
                 //        else if (assignIncr->getOperator() == "-=") stepValueOpt = -(*literalValOpt);
                 //    }
                 // }
                 // For now, this part is commented out as it relies on AssignmentNode structure details not explicitly given.
            }
        }

        // Default step to 1 if an initializer and condition were found, but no specific increment
        // (e.g. for loop missing the increment part but it implies step 1 for range)
        if (startValueOpt && endConditionValueOpt && !stepValueOpt) {
            // Heuristic: if we have a start and an end, and no explicit step, assume 1 or -1
            if (conditionOperator == "<" || conditionOperator == "<=") {
                 if (*endConditionValueOpt > *startValueOpt) stepValueOpt = 1;
            } else if (conditionOperator == ">" || conditionOperator == ">=") {
                 if (*endConditionValueOpt < *startValueOpt) stepValueOpt = -1;
            }
        }
        
        // --- Check if we successfully extracted a simple range pattern ---
        if (startValueOpt && endConditionValueOpt && stepValueOpt && !loopVarName.empty() && !conditionOperator.empty()) {
            long long start = *startValueOpt;
            long long end_cond_val = *endConditionValueOpt;
            long long step = *stepValueOpt;
            long long actual_range_end;

            bool valid_combination = false;
            if (step > 0) {
                if (conditionOperator == "<") { actual_range_end = end_cond_val; valid_combination = (start < actual_range_end); }
                else if (conditionOperator == "<=") { actual_range_end = end_cond_val + 1; valid_combination = (start < actual_range_end); }
            } else if (step < 0) {
                if (conditionOperator == ">") { actual_range_end = end_cond_val; valid_combination = (start > actual_range_end); }
                else if (conditionOperator == ">=") { actual_range_end = end_cond_val - 1; valid_combination = (start > actual_range_end); }
            }

            if (valid_combination) { // And ensure the loop would actually run at least once, or is a valid empty range
                out << indent(indentLevel) << "for " << loopVarName << " in range(";
                if (start == 0 && step == 1) {
                    out << actual_range_end;
                } else if (step == 1) {
                    out << start << ", " << actual_range_end;
                } else {
                    out << start << ", " << actual_range_end << ", " << step;
                }
                out << "):\n";
                out << generateCode(forNode->getBody(), indentLevel + 1);
                successfullyTranspiledToRange = true;
            }
        }
        

        // --- Fallback to while loop if not transpiled to range ---
        if (!successfullyTranspiledToRange) {
            if (forNode->getInitializer()) {
                out << generateCode(forNode->getInitializer(), indentLevel);
            }
            out << indent(indentLevel) << "while ";
            if (forNode->getCondition()) { // Condition is mandatory for a while loop
                 out << generateCode(forNode->getCondition(), 0);
            } else {
                 out << "True"; // Or error: C-style for without condition is often 'true'
            }
            out << ":\n";
            
            // Body first
            out << generateCode(forNode->getBody(), indentLevel + 1); 
            
            // Then increment, inside the loop and indented
            if (forNode->getIncrement()) {
                // Increment must be treated as a statement if it exists.
                // If forNode->getIncrement() is an ExpressionNode (like UnaryExpressionNode or AssignmentNode),
                // it might need to be wrapped in an ExpressionStatementNode implicitly or
                // the generateCode for these should produce just the expression part and we add ; \n.
                // Given your previous structure, an AssignmentNode for increment would need its AssignmentStatementNode wrapper
                // or `generateCode` should just make the expression.
                // Let's assume for now `generateCode(forNode->getIncrement(),0)` generates the expression string properly.
                out << indent(indentLevel + 1) << generateCode(forNode->getIncrement(), 0) << "\n";
            }
        }
    }
    // ... (rest of your generateCode method like ProgramNode, VariableDeclarationNode, etc.) ...
    else if (type == "ProgramNode") { // Ensure correct ordering of else-ifs
        auto programNode = dynamic_pointer_cast<ProgramNode>(node);
        if (!programNode) {
            out << indent(indentLevel) << "# Error: Cast failed for ProgramNode (type: " << type << ")\n";
            return out.str();
        }
        for (const auto& stmt : programNode->getChildren()) {
            out << generateCode(stmt, indentLevel);
        }
    } else if (type == "BlockNode") {
        auto blockNode = dynamic_pointer_cast<BlockNode>(node);
        if (!blockNode) {
            out << indent(indentLevel) << "# Error: Cast failed for BlockNode (type: " << type << ")\n";
            return out.str();
        }
        if (blockNode->getChildren().empty()) {
            out << indent(indentLevel) << "pass\n"; 
        } else {
            for (const auto& stmt : blockNode->getChildren()) {
                out << generateCode(stmt, indentLevel);
            }
        }
    } else if (type == "VariableDeclarationNode") {
        auto var = dynamic_pointer_cast<VariableDeclarationNode>(node);
        if (!var) { out << indent(indentLevel) << "# Error: Cast failed for VariableDeclarationNode\n"; return out.str(); }
        
        out << indent(indentLevel) << var->getName();
        if (var->getInitializer()) {
            out << " = " << generateCode(var->getInitializer(), 0); 
        } else {
            out << " = None"; 
        }
        out << "\n";
    } else if (type == "AssignmentStatementNode") {
        auto assignStmt = dynamic_pointer_cast<AssignmentStatementNode>(node);
        if (!assignStmt) { out << indent(indentLevel) << "# Error: Cast failed for AssignmentStatementNode\n"; return out.str(); }
        out << indent(indentLevel) << generateCode(assignStmt->getAssignment(), 0) << "\n";
    } else if (type == "AssignmentNode") { 
        auto a = dynamic_pointer_cast<AssignmentNode>(node);
        if (!a) { out << "# Error: Cast failed for AssignmentNode\n"; return out.str(); } 
        out << a->getTargetName() << " = " << generateCode(a->getValue(), 0);
    } else if (type == "ExpressionStatementNode") {
        auto exprStmt = dynamic_pointer_cast<ExpressionStatementNode>(node);
        if (!exprStmt) { out << indent(indentLevel) << "# Error: Cast failed for ExpressionStatementNode\n"; return out.str(); }
        out << indent(indentLevel) << generateCode(exprStmt->getExpression(), 0) << "\n";
    } else if (type == "BinaryExpressionNode") {
        auto bin = dynamic_pointer_cast<BinaryExpressionNode>(node);
        if (!bin) { out << "# Error: Cast failed for BinaryExpressionNode\n"; return out.str(); }
        out << generateCode(bin->getLeft(), 0) << " " << bin->getOperator() << " " << generateCode(bin->getRight(), 0);
    } else if (type == "UnaryExpressionNode") {
        auto un = dynamic_pointer_cast<UnaryExpressionNode>(node);
        if (!un) { out << "# Error: Cast failed for UnaryExpressionNode\n"; return out.str(); }
        out << un->getOperator() << generateCode(un->getOperand(), 0);
    } else if (type == "IdentifierNode") {
        auto id = dynamic_pointer_cast<IdentifierNode>(node);
        if (!id) { out << "# Error: Cast failed for IdentifierNode\n"; return out.str(); }
        out << id->getName();
    } else if (type == "NumberNode") {
        auto num = dynamic_pointer_cast<NumberNode>(node);
        if (!num) { out << "# Error: Cast failed for NumberNode\n"; return out.str(); }
        out << num->getValue(); 
    } else if (type == "StringLiteralNode") {
        auto str = dynamic_pointer_cast<StringLiteralNode>(node);
        if (!str) { out << "# Error: Cast failed for StringLiteralNode\n"; return out.str(); }
        out << '"' << str->getValue() << '"';
    } else if (type == "CharLiteralNode") {
        auto ch = dynamic_pointer_cast<CharLiteralNode>(node);
        if (!ch) { out << "# Error: Cast failed for CharLiteralNode\n"; return out.str(); }
        out << "'" << ch->getValue() << "'";
    } else if (type == "BooleanNode") {
        auto b = dynamic_pointer_cast<BooleanNode>(node);
        if (!b) { out << "# Error: Cast failed for BooleanNode\n"; return out.str(); }
        out << (b->getValue() ? "True" : "False"); 
    } else if (type == "IfNode") {
        auto ifNode = dynamic_pointer_cast<IfNode>(node);
        if (!ifNode) { out << indent(indentLevel) << "# Error: Cast failed for IfNode\n"; return out.str(); }
        out << indent(indentLevel) << "if " << generateCode(ifNode->getCondition(), 0) << ":\n";
        out << generateCode(ifNode->getThenBranch(), indentLevel + 1); 
        
        shared_ptr<ASTNode> elseBranch = ifNode->getElseBranch();
        if (elseBranch) {
            out << indent(indentLevel) << "else:\n";
            out << generateCode(elseBranch, indentLevel + 1); 
        }
    } else if (type == "WhileNode") {
        auto wh = dynamic_pointer_cast<WhileNode>(node);
        if (!wh) { out << indent(indentLevel) << "# Error: Cast failed for WhileNode\n"; return out.str(); }
        out << indent(indentLevel) << "while " << generateCode(wh->getCondition(), 0) << ":\n";
        out << generateCode(wh->getBody(), indentLevel + 1);
    } else if (type == "ReturnNode") {
        auto ret = dynamic_pointer_cast<ReturnNode>(node);
        if (!ret) { out << indent(indentLevel) << "# Error: Cast failed for ReturnNode\n"; return out.str(); }
        out << indent(indentLevel) << "return";
        if (ret->getReturnValue()) {
            out << " " << generateCode(ret->getReturnValue(), 0);
        }
        out << "\n";
    } else if (type == "FunctionDeclarationNode") {
        auto func = dynamic_pointer_cast<FunctionDeclarationNode>(node);
        if (!func) { out << indent(indentLevel) << "# Error: Cast failed for FunctionDeclarationNode\n"; return out.str(); }
        out << indent(indentLevel) << "def " << func->getName() << "(";
        const auto& params = func->getParamNames(); 
        for (size_t i = 0; i < params.size(); ++i) {
            out << params[i];
            if (i < params.size() - 1) out << ", ";
        }
        out << "):\n";
        if (func->getBody()) { 
            out << generateCode(func->getBody(), indentLevel + 1);
        } else {
            out << indent(indentLevel + 1) << "pass\n";
        }
    } else if (type == "FunctionCallNode") {
        auto call = dynamic_pointer_cast<FunctionCallNode>(node);
        if (!call) { out << "# Error: Cast failed for FunctionCallNode\n"; return out.str(); } 
        out << call->getFunctionName(); 
        out << "(";
        const auto& args = call->getArguments(); 
        for (size_t i = 0; i < args.size(); ++i) {
            out << generateCode(args[i], 0);
            if (i < args.size() - 1) out << ", ";
        }
        out << ")";
    } else if (type == "BreakNode") {
        out << indent(indentLevel) << "break\n";
    } else if (type == "ContinueNode") {
        out << indent(indentLevel) << "continue\n";
    } else {
        out << indent(indentLevel) << "# Unhandled AST node type: " << type << "\n";
    }

    return out.str();
}