#include "transpiler.h"
#include "Parser.h" // Needed for full class definitions
#include <iostream>
#include <sstream>
#include <stdexcept>

// Constructor
Transpiler::Transpiler() {
    // Initialization if needed
}

// Utility: Indent given code by 4 spaces per level (default 1 level)
std::string Transpiler::indent(const std::string& code, int level = 1) {
    std::stringstream ss(code);
    std::string line;
    std::string indented;
    std::string indentStr(level * 4, ' '); // 4 spaces per level

    while (std::getline(ss, line)) {
        if (!line.empty())
            indented += indentStr + line + "\n";
        else
            indented += "\n";
    }
    return indented;
}

// Entry point
std::string Transpiler::transpile(std::shared_ptr<ProgramNode> program) {
    return transpileProgram(program);
}

// Transpile a ProgramNode
std::string Transpiler::transpileProgram(std::shared_ptr<ProgramNode> program) {
    std::string code;
    for (const auto& stmt : program->getStatements()) {
        code += transpileStatement(stmt);
    }
    return code;
}

// Dispatch to the correct statement transpiler
std::string Transpiler::transpileStatement(std::shared_ptr<StatementNode> stmt) {
    if (!stmt) return "";

    if (auto assignStmt = std::dynamic_pointer_cast<AssignmentStatementNode>(stmt)) {
        return transpileAssignment(assignStmt);
    }
    else if (auto varDecl = std::dynamic_pointer_cast<VariableDeclarationNode>(stmt)) {
        return transpileVariableDeclaration(varDecl);
    }
    else if (auto ifStmt = std::dynamic_pointer_cast<IfNode>(stmt)) {
        return transpileIfStatement(ifStmt);
    }
    else if (auto whileStmt = std::dynamic_pointer_cast<WhileNode>(stmt)) {
        return transpileWhileStatement(whileStmt);
    }
    else if (auto forStmt = std::dynamic_pointer_cast<ForNode>(stmt)) {
        return transpileForStatement(forStmt);
    }
    else if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatementNode>(stmt)) {
        return transpileExpression(exprStmt->getExpression()) + "\n";  // no semicolon, newline
    }
    else if (auto returnStmt = std::dynamic_pointer_cast<ReturnNode>(stmt)) {
        return transpileReturnStatement(returnStmt);
    }
    else if (auto blockStmt = std::dynamic_pointer_cast<BlockNode>(stmt)) {
        return transpileBlock(blockStmt);
    }
    else if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclarationNode>(stmt)) {
        return transpileFunctionDeclaration(funcDecl);
    }
    else {
        std::cerr << "Unknown statement node type: " << stmt->type_name << std::endl;
        throw std::runtime_error("Unsupported statement type in transpilation.");
    }
}

// Transpile Return Statement
std::string Transpiler::transpileReturnStatement(std::shared_ptr<ReturnNode> stmt) {
    if (!stmt->getReturnValue()) {
        return "return\n";
    }
    return "return " + transpileExpression(stmt->getReturnValue()) + "\n";
}

// Transpile Assignment Statement
std::string Transpiler::transpileAssignment(std::shared_ptr<AssignmentStatementNode> stmt) {
    std::string target = stmt->getAssignment()->getTargetName();
    std::string value = transpileExpression(stmt->getAssignment()->getValue());
    return target + " = " + value + "\n";
}

// Helper: Transpile Assignment Node (no semicolon)
std::string Transpiler::transpileAssignmentNode(std::shared_ptr<AssignmentNode> assign) {
    std::string target = assign->getTargetName();
    std::string value = transpileExpression(assign->getValue());
    return target + " = " + value;
}

// Transpile Variable Declaration (no type, no semicolon)
std::string Transpiler::transpileVariableDeclaration(std::shared_ptr<VariableDeclarationNode> decl) {
    std::string name = decl->getName();
    if (decl->getInitializer()) {
        std::string init = transpileExpression(decl->getInitializer());
        return name + " = " + init + "\n";
    } else {
        // Option 1: skip completely
        return "";
        // Option 2 (optional): assign None
        // return name + " = None\n";
    }
}


// Transpile If Statement
std::string Transpiler::transpileIfStatement(std::shared_ptr<IfNode> stmt) {
    std::string condition = transpileExpression(stmt->getCondition());

    std::string thenBranch;
    if (auto thenBlock = std::dynamic_pointer_cast<BlockNode>(stmt->getThenBranch())) {
        thenBranch = transpileBlock(thenBlock);
    } else {
        thenBranch = indent(transpileStatement(stmt->getThenBranch()));
    }

    std::string code = "if " + condition + ":\n" + thenBranch;

    if (stmt->getElseBranch()) {
        std::string elseBranch;
        if (auto elseBlock = std::dynamic_pointer_cast<BlockNode>(stmt->getElseBranch())) {
            elseBranch = transpileBlock(elseBlock);
        } else {
            elseBranch = indent(transpileStatement(stmt->getElseBranch()));
        }
        code += "else:\n" + elseBranch;
    }

    return code;
}

// Transpile While Statement
std::string Transpiler::transpileWhileStatement(std::shared_ptr<WhileNode> stmt) {
    std::string condition = transpileExpression(stmt->getCondition());

    std::string body;
    if (auto bodyBlock = std::dynamic_pointer_cast<BlockNode>(stmt->getBody())) {
        body = transpileBlock(bodyBlock);
    } else {
        body = indent(transpileStatement(stmt->getBody()));
    }

    return "while " + condition + ":\n" + body;
}

// Transpile For Statement (convert to Python style for-in-range)
std::string Transpiler::transpileForStatement(std::shared_ptr<ForNode> stmt) {
    std::string loopVar;
    std::string startVal = "0";
    std::string endVal;
    std::string stepVal = "1"; // Default step is 1

    // 1. Extract initializer (for loop variable and start)
    if (auto decl = std::dynamic_pointer_cast<VariableDeclarationNode>(stmt->getInitializer())) {
        loopVar = decl->getName();
        if (decl->getInitializer()) {
            startVal = transpileExpression(decl->getInitializer());
        }
    } else if (auto assignStmt = std::dynamic_pointer_cast<AssignmentStatementNode>(stmt->getInitializer())) {
        loopVar = assignStmt->getAssignment()->getTargetName();
        startVal = transpileExpression(assignStmt->getAssignment()->getValue());
    } else {
        throw std::runtime_error("Unsupported for-loop initializer type.");
    }

    // 2. Extract condition (assume form: i < endVal or i <= endVal)
    if (auto binaryCond = std::dynamic_pointer_cast<BinaryExpressionNode>(stmt->getCondition())) {
        std::string op = binaryCond->getOperator();
        endVal = transpileExpression(binaryCond->getRight());

        if (op == "<=") {
            endVal += " + 1"; // Inclusive
        } else if (op != "<") {
            throw std::runtime_error("Unsupported for-loop condition operator: " + op);
        }
    } else {
        throw std::runtime_error("Unsupported for-loop condition type.");
    }

    // 3. Detect if increment is additive/subtractive or complex
    bool isSimpleIncrement = false;
    std::string incrementCode;

    if (auto incrAssign = std::dynamic_pointer_cast<AssignmentNode>(stmt->getIncrement())) {
        auto binaryExpr = std::dynamic_pointer_cast<BinaryExpressionNode>(incrAssign->getValue());
        if (binaryExpr && 
           (binaryExpr->getOperator() == "+" || binaryExpr->getOperator() == "-") &&
            transpileExpression(binaryExpr->getLeft()) == loopVar) {
            isSimpleIncrement = true;
            if (binaryExpr->getOperator() == "+") {
                stepVal = transpileExpression(binaryExpr->getRight());
            } else { // operator == "-"
                stepVal = "-" + transpileExpression(binaryExpr->getRight());
            }
        } else {
            // Complex increment detected (e.g., multiplication)
            incrementCode = transpileAssignmentNode(incrAssign) + "\n";
        }
    } else {
        throw std::runtime_error("Unsupported for-loop increment type.");
    }

    // 4. Transpile body
    std::string body;
    if (auto bodyBlock = std::dynamic_pointer_cast<BlockNode>(stmt->getBody())) {
        body = transpileBlock(bodyBlock);
    } else {
        body = indent(transpileStatement(stmt->getBody()));
    }

    // 5. Construct Python code
    if (isSimpleIncrement) {
        // Use for-in-range loop
        return "for " + loopVar + " in range(" + startVal + ", " + endVal + ", " + stepVal + "):\n" + body;
    } else {
        // Use while loop for complex increments
        std::string code;
        code += loopVar + " = " + startVal + "\n";
        code += "while " + transpileExpression(stmt->getCondition()) + ":\n";
        code += body;
        code += indent(incrementCode, 1);
        return code;
    }
}


// Transpile Function Declaration (remove types, add colon + indent body)
std::string Transpiler::transpileFunctionDeclaration(std::shared_ptr<FunctionDeclarationNode> funcDecl) {
    std::string code = "def " + funcDecl->getName() + "(";

    const auto& paramNames = funcDecl->getParamNames();

    for (size_t i = 0; i < paramNames.size(); ++i) {
        code += paramNames[i];
        if (i != paramNames.size() - 1) code += ", ";
    }
    code += "):\n";

    auto body = funcDecl->getBody();
    if (body) {
        code += indent(transpileBlock(body));
    } else {
        code += indent("pass\n");
    }

    return code;
}

// Transpile Expression
std::string Transpiler::transpileExpression(std::shared_ptr<ExpressionNode> expr) {
    if (!expr) return "";
    if (auto binary = std::dynamic_pointer_cast<BinaryExpressionNode>(expr)) {
        return transpileBinaryExpression(binary);
    }
    if (auto unary = std::dynamic_pointer_cast<UnaryExpressionNode>(expr)) {
        return transpileUnaryExpression(unary);
    }
    if (auto ident = std::dynamic_pointer_cast<IdentifierNode>(expr)) {
        return ident->getName();
    }
    if (auto number = std::dynamic_pointer_cast<NumberNode>(expr)) {
        return number->getValue();
    }
    if (auto funcCall = std::dynamic_pointer_cast<FunctionCallNode>(expr)) {
        std::string result = funcCall->getFunctionName() + "(";
        for (size_t i = 0; i < funcCall->getArguments().size(); ++i) {
            result += transpileExpression(funcCall->getArguments()[i]);
            if (i != funcCall->getArguments().size() - 1) result += ", ";
        }
        result += ")";
        return result;
    }
    if (auto assign = std::dynamic_pointer_cast<AssignmentNode>(expr)) {
        return transpileAssignmentNode(assign);
    }

    std::cerr << "Unsupported expression type encountered: " << expr->type_name << std::endl;
    throw std::runtime_error("Unsupported expression type in transpilation.");
}

// Transpile Binary Expression
std::string Transpiler::transpileBinaryExpression(std::shared_ptr<BinaryExpressionNode> expr) {
    std::string left = transpileExpression(expr->getLeft());
    std::string right = transpileExpression(expr->getRight());
    return "(" + left + " " + expr->getOperator() + " " + right + ")";
}

// Transpile Unary Expression
std::string Transpiler::transpileUnaryExpression(std::shared_ptr<UnaryExpressionNode> expr) {
    return expr->getOperator() + transpileExpression(expr->getOperand());
}

// Transpile Block (no braces, just indent statements)
std::string Transpiler::transpileBlock(std::shared_ptr<BlockNode> block) {
    std::string code;
    for (const auto& stmt : block->getStatements()) {
        code += transpileStatement(stmt);
    }
    return indent(code);
}
