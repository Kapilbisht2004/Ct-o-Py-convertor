#include "Transpiler.h"
#include <sstream>
// #include <iostream> // For std::cerr if needed for debugging, but returning error strings is cleaner

// Assuming Parser.h (included via Transpiler.h) provides definitions for:
// - ASTNode base class with a `std::string type_name` member.
// - All ASTNode derived classes (ProgramNode, BlockNode, VariableDeclarationNode, etc.).
// - Necessary getter methods on these nodes (getName(), getInitializer(), getChildren(), getOperator(), etc.).

Transpiler::Transpiler(shared_ptr<ProgramNode> root) : astRoot(move(root)) {}

string Transpiler::transpile() {
    if (!astRoot) {
        return "# Error: AST root is null.\n";
    }
    return generateCode(astRoot, 0); // Initial call with indentLevel 0 for the whole program
}

string Transpiler::indent(int level) {
    // Creates the indentation string by repeating indentStr `level` times.
    // This is robust for any indentStr (e.g., "    ", "  ", "\t").
    std::ostringstream oss;
    for (int i = 0; i < level; ++i) {
        oss << indentStr;
    }
    return oss.str();
}

string Transpiler::generateCode(const shared_ptr<ASTNode>& node, int indentLevel) {
    if (!node) {
        // This might occur if an optional child node is legitimately nullptr.
        // Returning an empty string is generally safe. For debugging, a comment could be added.
        // Example: return indent(indentLevel) + "# Warning: Null AST node encountered.\n";
        return ""; 
    }

    ostringstream out;
    const string& type = node->type_name; // Assumes ASTNode base class has this public member

    if (type == "ProgramNode") {
        auto programNode = dynamic_pointer_cast<ProgramNode>(node);
        if (!programNode) {
            out << indent(indentLevel) << "# Error: Cast failed for ProgramNode (type: " << type << ")\n";
            return out.str();
        }
        for (const auto& stmt : programNode->getChildren()) {
            out << generateCode(stmt, indentLevel); // Top-level statements in a program usually have indentLevel 0
        }
    } else if (type == "BlockNode") {
        auto blockNode = dynamic_pointer_cast<BlockNode>(node);
        if (!blockNode) {
            out << indent(indentLevel) << "# Error: Cast failed for BlockNode (type: " << type << ")\n";
            return out.str();
        }
        if (blockNode->getChildren().empty()) {
            out << indent(indentLevel) << "pass\n"; // Python requires 'pass' for empty blocks/suites
        } else {
            for (const auto& stmt : blockNode->getChildren()) {
                // Statements within a block maintain the block's indentLevel.
                // If a statement is itself a block-creating structure (like IfNode),
                // it will handle further indentation for its own children.
                out << generateCode(stmt, indentLevel);
            }
        }
    } else if (type == "VariableDeclarationNode") {
        auto var = dynamic_pointer_cast<VariableDeclarationNode>(node);
        if (!var) { out << indent(indentLevel) << "# Error: Cast failed for VariableDeclarationNode\n"; return out.str(); }
        
        out << indent(indentLevel) << var->getName();
        
        // Check if this variable has an initializer
        if (var->getInitializer()) {
            out << " = " << generateCode(var->getInitializer(), 0);
        } 
        // Only add None for variables with no initializer if you specifically need this
        else {
            out << " = None"; // Comment out or remove this line
        }
        out << "\n";
    } else if (type == "AssignmentStatementNode") {
        auto assignStmt = dynamic_pointer_cast<AssignmentStatementNode>(node);
        if (!assignStmt) { out << indent(indentLevel) << "# Error: Cast failed for AssignmentStatementNode\n"; return out.str(); }
        // The AssignmentNode itself doesn't handle indentation or newlines.
        // This statement wrapper does.
        out << indent(indentLevel) << generateCode(assignStmt->getAssignment(), 0) << "\n";
    } else if (type == "AssignmentNode") { // This is an expression component, not a standalone statement
        auto a = dynamic_pointer_cast<AssignmentNode>(node);
        if (!a) { out << "# Error: Cast failed for AssignmentNode\n"; return out.str(); } // No indent for expression parts
        out << a->getTargetName() << " = " << generateCode(a->getValue(), 0);
    } else if (type == "ExpressionStatementNode") {
        auto exprStmt = dynamic_pointer_cast<ExpressionStatementNode>(node);
        if (!exprStmt) { out << indent(indentLevel) << "# Error: Cast failed for ExpressionStatementNode\n"; return out.str(); }
        out << indent(indentLevel) << generateCode(exprStmt->getExpression(), 0) << "\n";
    } else if (type == "BinaryExpressionNode") {
        auto bin = dynamic_pointer_cast<BinaryExpressionNode>(node);
        if (!bin) { out << "# Error: Cast failed for BinaryExpressionNode\n"; return out.str(); }
        // For complex precedence rules, parentheses might be needed around generated sub-expressions.
        // This basic version doesn't add them explicitly.
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
        out << num->getValue(); // Assuming getValue() returns a type ostringstream can handle (e.g., int, double, string)
    } else if (type == "StringLiteralNode") {
        auto str = dynamic_pointer_cast<StringLiteralNode>(node);
        if (!str) { out << "# Error: Cast failed for StringLiteralNode\n"; return out.str(); }
        // TODO: Implement proper string escaping if getValue() returns raw string content.
        // For now, assumes getValue() returns content that is safe to directly embed in quotes.
        out << '"' << str->getValue() << '"';
    } else if (type == "CharLiteralNode") {
        auto ch = dynamic_pointer_cast<CharLiteralNode>(node);
        if (!ch) { out << "# Error: Cast failed for CharLiteralNode\n"; return out.str(); }
        // Python treats chars as strings of length 1.
        // TODO: Proper escaping for special chars like ' or \.
        out << "'" << ch->getValue() << "'";
    } else if (type == "BooleanNode") {
        auto b = dynamic_pointer_cast<BooleanNode>(node);
        if (!b) { out << "# Error: Cast failed for BooleanNode\n"; return out.str(); }
        out << (b->getValue() ? "True" : "False"); // Python boolean keywords
    } else if (type == "IfNode") {
        auto ifNode = dynamic_pointer_cast<IfNode>(node);
        if (!ifNode) { out << indent(indentLevel) << "# Error: Cast failed for IfNode\n"; return out.str(); }
        out << indent(indentLevel) << "if " << generateCode(ifNode->getCondition(), 0) << ":\n";
        out << generateCode(ifNode->getThenBranch(), indentLevel + 1); // ThenBranch is typically a BlockNode
        
        shared_ptr<ASTNode> elseBranch = ifNode->getElseBranch();
        if (elseBranch) {
            // Check if else branch is an IfNode for elif construct (optional extension)
            // For simple else:
            out << indent(indentLevel) << "else:\n";
            out << generateCode(elseBranch, indentLevel + 1); // ElseBranch is typically a BlockNode
        }
    } else if (type == "WhileNode") {
        auto wh = dynamic_pointer_cast<WhileNode>(node);
        if (!wh) { out << indent(indentLevel) << "# Error: Cast failed for WhileNode\n"; return out.str(); }
        out << indent(indentLevel) << "while " << generateCode(wh->getCondition(), 0) << ":\n";
        out << generateCode(wh->getBody(), indentLevel + 1); // Body is typically a BlockNode
    } else if (type == "ForNode") { // Translating C-style for to Python while
        auto forNode = dynamic_pointer_cast<ForNode>(node);
        if (!forNode) { out << indent(indentLevel) << "# Error: Cast failed for ForNode\n"; return out.str(); }

        if (forNode->getInitializer()) {
            // Initializer is expected to be a statement node (e.g., VarDecl, AssignmentStmt)
            // which will handle its own indentation and newline.
            out << generateCode(forNode->getInitializer(), indentLevel);
        }
        out << indent(indentLevel) << "while " << generateCode(forNode->getCondition(), 0) << ":\n";
        // Body needs to be indented one level more than the while loop's base indent
        out << generateCode(forNode->getBody(), indentLevel + 1);
        if (forNode->getIncrement()) {
            // Increment is treated as an expression statement, placed at the end of the loop body.
            out << indent(indentLevel + 1) << generateCode(forNode->getIncrement(), 0) << "\n";
        }
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
        const auto& params = func->getParamNames(); // Assumed to be vector<string>
        for (size_t i = 0; i < params.size(); ++i) {
            out << params[i];
            if (i < params.size() - 1) out << ", ";
        }
        out << "):\n";
        if (func->getBody()) { // getBody() usually returns a BlockNode
            out << generateCode(func->getBody(), indentLevel + 1);
        } else {
            // If the AST allows a function to be declared without a body block (getBody() is nullptr)
            out << indent(indentLevel + 1) << "pass\n";
        }
    } else if (type == "FunctionCallNode") {
        auto call = dynamic_pointer_cast<FunctionCallNode>(node);
        // No cast error message needs indentation if it's part of an expression
        if (!call) { out << "# Error: Cast failed for FunctionCallNode\n"; return out.str(); } 
        out << call->getFunctionName(); // Assumed string
        out << "(";
        const auto& args = call->getArguments(); // Assumed vector<shared_ptr<ASTNode>>
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