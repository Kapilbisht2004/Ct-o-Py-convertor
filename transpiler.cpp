#include "transpiler.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

// Constructor
Transpiler::Transpiler(){}

// Utility: Indent given code by 4 spaces per level (default 1 level)
string Transpiler::indent(const string& code, int level = 1) {
    stringstream ss(code);
    string line;
    string indented;
    string indentStr(level * 4, ' '); // 4 spaces per level

    while (getline(ss, line)) {
        if (!line.empty())
            indented += indentStr + line + "\n";
        else
            indented += "\n";
    }
    return indented;
}

// Entry point
string Transpiler::transpile(shared_ptr<ProgramNode> program) {
    return transpileProgram(program);
}

// Transpile a ProgramNode
string Transpiler::transpileProgram(shared_ptr<ProgramNode> program) {
    string code;
    for (const auto& stmt : program->getStatements()) {
        code += transpileStatement(stmt);
    }
    return code;
}

// Dispatch to the correct statement transpiler
string Transpiler::transpileStatement(shared_ptr<StatementNode> stmt) {
    if (!stmt) return "";

    if (auto assignStmt = dynamic_pointer_cast<AssignmentStatementNode>(stmt)) {
        return transpileAssignment(assignStmt);
    }
    else if (auto varDecl = dynamic_pointer_cast<VariableDeclarationNode>(stmt)) {
        return transpileVariableDeclaration(varDecl);
    }
    else if (auto ifStmt = dynamic_pointer_cast<IfNode>(stmt)) {
        return transpileIfStatement(ifStmt);
    }
    else if (auto whileStmt = dynamic_pointer_cast<WhileNode>(stmt)) {
        return transpileWhileStatement(whileStmt);
    }
    else if (auto forStmt = dynamic_pointer_cast<ForNode>(stmt)) {
        return transpileForStatement(forStmt);
    }
    else if (auto exprStmt = dynamic_pointer_cast<ExpressionStatementNode>(stmt)) {
        return transpileExpression(exprStmt->getExpression()) + "\n";  // no semicolon, newline
    }
    else if (auto returnStmt = dynamic_pointer_cast<ReturnNode>(stmt)) {
        return transpileReturnStatement(returnStmt);
    }
    else if (auto blockStmt = dynamic_pointer_cast<BlockNode>(stmt)) {
        return transpileBlock(blockStmt);
    }
    else if (auto funcDecl = dynamic_pointer_cast<FunctionDeclarationNode>(stmt)) {
        return transpileFunctionDeclaration(funcDecl);
    }
    else {
        cerr << "Unknown statement node type: " << stmt->type_name << endl;
        throw runtime_error("Unsupported statement type in transpilation.");
    }
}

// Transpile Return Statement
string Transpiler::transpileReturnStatement(shared_ptr<ReturnNode> stmt) {
    if (!stmt->getReturnValue()) {
        return "return\n";
    }
    return "return " + transpileExpression(stmt->getReturnValue()) + "\n";
}

// Transpile Assignment Statement
string Transpiler::transpileAssignment(shared_ptr<AssignmentStatementNode> stmt) {
    string target = stmt->getAssignment()->getTargetName();
    string value = transpileExpression(stmt->getAssignment()->getValue());
    return target + " = " + value + "\n";
}

// Helper: Transpile Assignment Node (no semicolon)
string Transpiler::transpileAssignmentNode(shared_ptr<AssignmentNode> assign) {
    string target = assign->getTargetName(); // e.g., "i"
    string value = transpileExpression(assign->getValue()); // e.g., "i + 1"
    return target + " = " + value; // "i = i + 1" (no newline)
}

// Transpile Variable Declaration (no type, no semicolon)
string Transpiler::transpileVariableDeclaration(shared_ptr<VariableDeclarationNode> decl) {
    string name = decl->getName();
    if (decl->getInitializer()) {
        string init = transpileExpression(decl->getInitializer());
        return name + " = " + init + "\n";
    } else {
        return "";// Skip uninitialized declarations (Python doesn't support them)
    }
}


// Transpile If Statement
string Transpiler::transpileIfStatement(shared_ptr<IfNode> stmt) {
    string condition = transpileExpression(stmt->getCondition());

    string thenBranch;
    if (auto thenBlock = dynamic_pointer_cast<BlockNode>(stmt->getThenBranch())) {
        thenBranch = transpileBlock(thenBlock);
    } else {
        thenBranch = indent(transpileStatement(stmt->getThenBranch()));
    }

    string code = "if " + condition + ":\n" + thenBranch;

    if (stmt->getElseBranch()) {
        string elseBranch;
        if (auto elseBlock = dynamic_pointer_cast<BlockNode>(stmt->getElseBranch())) {
            elseBranch = transpileBlock(elseBlock);
        } else {
            elseBranch = indent(transpileStatement(stmt->getElseBranch()));
        }
        code += "else:\n" + elseBranch;
    }

    return code;
}

// Transpile While Statement
string Transpiler::transpileWhileStatement(shared_ptr<WhileNode> stmt) {
    string condition = transpileExpression(stmt->getCondition());

    string body;
    if (auto bodyBlock = dynamic_pointer_cast<BlockNode>(stmt->getBody())) {
        body = transpileBlock(bodyBlock);
    } else {
        body = indent(transpileStatement(stmt->getBody()));
    }

    return "while " + condition + ":\n" + body;
}

// Transpile For Statement (convert to Python style for-in-range)
string Transpiler::transpileForStatement(shared_ptr<ForNode> stmt) {
    string loopVar;       // e.g., "i"
    string startVal = "0"; // Default start value
    string endVal;
    string stepVal = "1"; // Default step is 1

    // 1. Extract initializer (for loop variable and start)
    if (auto decl = dynamic_pointer_cast<VariableDeclarationNode>(stmt->getInitializer())) {
        loopVar = decl->getName(); // e.g., "int i = 0" → "i"        
        if (decl->getInitializer()) {
            startVal = transpileExpression(decl->getInitializer()); // e.g., "0"
        }
    } else if (auto assignStmt = dynamic_pointer_cast<AssignmentStatementNode>(stmt->getInitializer())) {
        loopVar = assignStmt->getAssignment()->getTargetName();  // e.g., "i = 0" → "i"
        startVal = transpileExpression(assignStmt->getAssignment()->getValue());
    } else {
        throw runtime_error("Unsupported for-loop initializer type.");
    }

    // 2. Extract condition (<,<=,>,>=)
    if (auto binaryCond = dynamic_pointer_cast<BinaryExpressionNode>(stmt->getCondition())) {
        string op = binaryCond->getOperator(); // e.g., "<", "<="
        endVal = transpileExpression(binaryCond->getRight());

        if (op == "<=") {
            endVal += " + 1"; // Inclusive
        } else if (op == ">=") {
            endVal += " - 1"; // Inclusive
        }else if (op != "<" && op != ">") {
            throw runtime_error("Unsupported for-loop condition operator: " + op);
        }
    } else {
        throw runtime_error("Unsupported for-loop condition type.");
    }

    // 3. Detect if increment is additive/subtractive or complex
    bool isSimpleIncrement = false;
    string incrementCode;

    if (auto incrAssign = dynamic_pointer_cast<AssignmentNode>(stmt->getIncrement())) {
        auto binaryExpr = dynamic_pointer_cast<BinaryExpressionNode>(incrAssign->getValue());
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
        throw runtime_error("Unsupported for-loop increment type.");
    }

    // 4. Transpile body
    string body;
    if (auto bodyBlock = dynamic_pointer_cast<BlockNode>(stmt->getBody())) {
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
        string code;
        code += loopVar + " = " + startVal + "\n";
        code += "while " + transpileExpression(stmt->getCondition()) + ":\n";
        code += body;
        code += indent(incrementCode, 1);
        return code;
    }
}


// Transpile Function Declaration (remove types, add colon + indent body)
string Transpiler::transpileFunctionDeclaration(shared_ptr<FunctionDeclarationNode> funcDecl) {
    string code = "def " + funcDecl->getName() + "(";

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
string Transpiler::transpileExpression(shared_ptr<ExpressionNode> expr) {
    if (!expr) return "";
    if (auto binary = dynamic_pointer_cast<BinaryExpressionNode>(expr)) {
        return transpileBinaryExpression(binary);
    }
    if (auto unary = dynamic_pointer_cast<UnaryExpressionNode>(expr)) {
        return transpileUnaryExpression(unary);
    }
    if (auto ident = dynamic_pointer_cast<IdentifierNode>(expr)) {
        return ident->getName();
    }
    if (auto number = dynamic_pointer_cast<NumberNode>(expr)) {
        return number->getValue();
    }
    if (auto funcCall = dynamic_pointer_cast<FunctionCallNode>(expr)) {
        string result = funcCall->getFunctionName() + "(";
        for (size_t i = 0; i < funcCall->getArguments().size(); ++i) {
            result += transpileExpression(funcCall->getArguments()[i]);
            if (i != funcCall->getArguments().size() - 1) result += ", ";
        }
        result += ")";
        return result;
    }
    if (auto assign = dynamic_pointer_cast<AssignmentNode>(expr)) {
        return transpileAssignmentNode(assign);
    }

    cerr << "Unsupported expression type encountered: " << expr->type_name << endl;
    throw runtime_error("Unsupported expression type in transpilation.");
}

// Transpile Binary Expression
string Transpiler::transpileBinaryExpression(shared_ptr<BinaryExpressionNode> expr) {
    string left = transpileExpression(expr->getLeft());
    string right = transpileExpression(expr->getRight());
    return "(" + left + " " + expr->getOperator() + " " + right + ")";
}

// Transpile Unary Expression
string Transpiler::transpileUnaryExpression(shared_ptr<UnaryExpressionNode> expr) {
    return expr->getOperator() + transpileExpression(expr->getOperand());
}

// Transpile Block (no braces, just indent statements)
string Transpiler::transpileBlock(shared_ptr<BlockNode> block) {
    string code;
    for (const auto& stmt : block->getStatements()) {
        code += transpileStatement(stmt);
    }
    return indent(code);
}
