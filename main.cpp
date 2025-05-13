#include <iostream>
#include <string>
#include <vector>
#include <memory> // For std::shared_ptr, std::dynamic_pointer_cast
#include "Lexer.h"  // Assumed to provide Token, TokenType, tokenTypeToString, and Lexer class
#include "Parser.h" // Provides ASTNode derived classes and Parser class

// Helper to print indentation
void printIndent(int indent) {
    for (int i = 0; i < indent; ++i) {
        std::cout << "  ";
    }
}

// Forward declaration for printAST as some nodes might recursively call it for children
void printAST(const std::shared_ptr<ASTNode>& node, int indent = 0);

// Specialized print functions for clarity or if dynamic_cast is too verbose in a switch
void printProgramNode(const std::shared_ptr<ProgramNode>& node, int indent) {
    printIndent(indent);
    std::cout << "ProgramNode (" << node->type_name << ")" << std::endl;
    for (const auto& stmt : node->getStatements()) {
        printAST(stmt, indent + 1);
    }
}

void printBlockNode(const std::shared_ptr<BlockNode>& node, int indent) {
    printIndent(indent);
    std::cout << "BlockNode (" << node->type_name << ")" << std::endl;
    for (const auto& stmt : node->getStatements()) {
        printAST(stmt, indent + 1);
    }
}

void printExpressionStatementNode(const std::shared_ptr<ExpressionStatementNode>& node, int indent) {
    printIndent(indent);
    std::cout << "ExpressionStatementNode (" << node->type_name << ")" << std::endl;
    printAST(node->getExpression(), indent + 1);
}

void printIfNode(const std::shared_ptr<IfNode>& node, int indent) {
    printIndent(indent);
    std::cout << "IfNode (" << node->type_name << ")" << std::endl;
    printIndent(indent + 1); std::cout << "Condition:" << std::endl;
    printAST(node->getCondition(), indent + 2);
    printIndent(indent + 1); std::cout << "ThenBranch:" << std::endl;
    printAST(node->getThenBranch(), indent + 2);
    if (node->getElseBranch()) {
        printIndent(indent + 1); std::cout << "ElseBranch:" << std::endl;
        printAST(node->getElseBranch(), indent + 2);
    }
}

void printWhileNode(const std::shared_ptr<WhileNode>& node, int indent) {
    printIndent(indent);
    std::cout << "WhileNode (" << node->type_name << ")" << std::endl;
    printIndent(indent + 1); std::cout << "Condition:" << std::endl;
    printAST(node->getCondition(), indent + 2);
    printIndent(indent + 1); std::cout << "Body:" << std::endl;
    printAST(node->getBody(), indent + 2);
}

void printForNode(const std::shared_ptr<ForNode>& node, int indent) {
    printIndent(indent);
    std::cout << "ForNode (" << node->type_name << ")" << std::endl;
    if (node->getInitializer()) {
        printIndent(indent + 1); std::cout << "Initializer:" << std::endl;
        printAST(node->getInitializer(), indent + 2);
    }
    if (node->getCondition()) {
        printIndent(indent + 1); std::cout << "Condition:" << std::endl;
        printAST(node->getCondition(), indent + 2);
    }
    if (node->getIncrement()) {
        printIndent(indent + 1); std::cout << "Increment:" << std::endl;
        printAST(node->getIncrement(), indent + 2);
    }
    printIndent(indent + 1); std::cout << "Body:" << std::endl;
    printAST(node->getBody(), indent + 2);
}

void printReturnNode(const std::shared_ptr<ReturnNode>& node, int indent) {
    printIndent(indent);
    std::cout << "ReturnNode (" << node->type_name << ")" << std::endl;
    if (node->getReturnValue()) {
        printIndent(indent + 1); std::cout << "Value:" << std::endl;
        printAST(node->getReturnValue(), indent + 2);
    }
}

void printVariableDeclarationNode(const std::shared_ptr<VariableDeclarationNode>& node, int indent) {
    printIndent(indent);
    std::cout << "VariableDeclarationNode (" << node->type_name << "): "
              << node->getDeclaredType() << " " << node->getName() << std::endl;
    if (node->getInitializer()) {
        printIndent(indent + 1); std::cout << "Initializer:" << std::endl;
        printAST(node->getInitializer(), indent + 2);
    }
}

void printFunctionDeclarationNode(const std::shared_ptr<FunctionDeclarationNode>& node, int indent) {
    printIndent(indent);
    std::cout << "FunctionDeclarationNode (" << node->type_name << "): "
              << node->getDeclaredType() << " " << node->getName() << "(";
    const auto& paramNames = node->getParamNames();
    const auto& paramTypes = node->getParamTypes();
    for (size_t i = 0; i < paramNames.size(); ++i) {
        std::cout << paramTypes[i] << " " << paramNames[i] << (i < paramNames.size() - 1 ? ", " : "");
    }
    std::cout << ")" << std::endl;
    if (node->getBody()) {
        printIndent(indent + 1); std::cout << "Body:" << std::endl;
        printAST(node->getBody(), indent + 2);
    } else {
        printIndent(indent + 1); std::cout << "(Forward Declaration / No Body)" << std::endl;
    }
}

void printAssignmentStatementNode(const std::shared_ptr<AssignmentStatementNode>& node, int indent) {
    printIndent(indent);
    std::cout << "AssignmentStatementNode (" << node->type_name << ")" << std::endl;
    printAST(node->getAssignment(), indent + 1); // Print the inner AssignmentNode
}

void printAssignmentNode(const std::shared_ptr<AssignmentNode>& node, int indent) {
    printIndent(indent);
    std::cout << "AssignmentNode (" << node->type_name << "): " << node->getTargetName() << " =" << std::endl;
    printIndent(indent + 1); std::cout << "Value:" << std::endl;
    printAST(node->getValue(), indent + 2);
}

void printBinaryExpressionNode(const std::shared_ptr<BinaryExpressionNode>& node, int indent) {
    printIndent(indent);
    std::cout << "BinaryExpressionNode (" << node->type_name << "): Operator '" << node->getOperator() << "'" << std::endl;
    printIndent(indent + 1); std::cout << "Left:" << std::endl;
    printAST(node->getLeft(), indent + 2);
    printIndent(indent + 1); std::cout << "Right:" << std::endl;
    printAST(node->getRight(), indent + 2);
}

void printUnaryExpressionNode(const std::shared_ptr<UnaryExpressionNode>& node, int indent) {
    printIndent(indent);
    std::cout << "UnaryExpressionNode (" << node->type_name << "): Operator '" << node->getOperator() << "'" << std::endl;
    printIndent(indent + 1); std::cout << "Operand:" << std::endl;
    printAST(node->getOperand(), indent + 2);
}

void printIdentifierNode(const std::shared_ptr<IdentifierNode>& node, int indent) {
    printIndent(indent);
    std::cout << "IdentifierNode (" << node->type_name << "): " << node->getName() << std::endl;
}

void printFunctionCallNode(const std::shared_ptr<FunctionCallNode>& node, int indent) {
    printIndent(indent);
    std::cout << "FunctionCallNode (" << node->type_name << "): " << node->getFunctionName() << std::endl;
    const auto& args = node->getArguments();
    if (!args.empty()) {
        printIndent(indent + 1); std::cout << "Arguments:" << std::endl;
        for (const auto& arg : args) {
            printAST(arg, indent + 2);
        }
    }
}

void printStringLiteralNode(const std::shared_ptr<StringLiteralNode>& node, int indent) {
    printIndent(indent);
    std::cout << "StringLiteralNode (" << node->type_name << "): \"" << node->getValue() << "\"" << std::endl;
}

void printCharLiteralNode(const std::shared_ptr<CharLiteralNode>& node, int indent) {
    printIndent(indent);
    std::cout << "CharLiteralNode (" << node->type_name << "): '" << node->getValue() << "'" << std::endl;
}

void printNumberNode(const std::shared_ptr<NumberNode>& node, int indent) {
    printIndent(indent);
    std::cout << "NumberNode (" << node->type_name << "): " << node->getValue() << std::endl;
}

void printBooleanNode(const std::shared_ptr<BooleanNode>& node, int indent) {
    printIndent(indent);
    std::cout << "BooleanNode (" << node->type_name << "): " << (node->getValue() ? "true" : "false") << std::endl;
}

void printAST(const std::shared_ptr<ASTNode>& node, int indent) {
    if (!node) {
        printIndent(indent);
        std::cout << "(nullptr)" << std::endl;
        return;
    }

    // Using dynamic_pointer_cast to determine the type and call specialized printers
    // This is safer and more object-oriented than relying on node->type_name string comparisons directly for logic.
    if (auto p = std::dynamic_pointer_cast<ProgramNode>(node)) printProgramNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<BlockNode>(node)) printBlockNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<ExpressionStatementNode>(node)) printExpressionStatementNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<IfNode>(node)) printIfNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<WhileNode>(node)) printWhileNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<ForNode>(node)) printForNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<ReturnNode>(node)) printReturnNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<BreakNode>(node)) { printIndent(indent); std::cout << "BreakNode (" << p->type_name << ")" << std::endl; }
    else if (auto p = std::dynamic_pointer_cast<ContinueNode>(node)) { printIndent(indent); std::cout << "ContinueNode (" << p->type_name << ")" << std::endl; }
    else if (auto p = std::dynamic_pointer_cast<VariableDeclarationNode>(node)) printVariableDeclarationNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<FunctionDeclarationNode>(node)) printFunctionDeclarationNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<AssignmentStatementNode>(node)) printAssignmentStatementNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<AssignmentNode>(node)) printAssignmentNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<BinaryExpressionNode>(node)) printBinaryExpressionNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<UnaryExpressionNode>(node)) printUnaryExpressionNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<IdentifierNode>(node)) printIdentifierNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<FunctionCallNode>(node)) printFunctionCallNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<StringLiteralNode>(node)) printStringLiteralNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<CharLiteralNode>(node)) printCharLiteralNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<NumberNode>(node)) printNumberNode(p, indent);
    else if (auto p = std::dynamic_pointer_cast<BooleanNode>(node)) printBooleanNode(p, indent);
    else {
        printIndent(indent);
        std::cout << "Unknown or unhandled ASTNode type: " << (node->type_name.empty() ? "(no type_name)" : node->type_name) << std::endl;
        // Generic child printing for unhandled complex nodes
        if(!node->getChildren().empty()){
            printIndent(indent + 1);
            std::cout << "Generic Children:" << std::endl;
            for(const auto& child : node->getChildren()){
                printAST(child, indent + 2);
            }
        }
    }
}


int main() {
    std::string source_code = R"(
        int calculateSum(int a, int b) {
            int result;
            result = a + b;
            return result;
        }

        void main() {
            int x = 10;
            int y;
            y = 20;
            if (x > 5) {
                x = x + (y * 2);
                calculateSum(x, y); // Expression statement with function call
            } else {
                x = 0;
            }
            while (x < 15) {
                x = x + 1;
            }
            for (int i = 0; i < 3; i = i + 1) {
                y = y - 1;
            }
            return; // Return with no value (void context)
        }
    )";

    Lexer lexer(source_code);
    std::vector<Token> tokens = lexer.tokenize();

    std::cout << "=== Tokens ===\n";
    for (const auto& token : tokens) {
        std::cout << "Token: '" << token.value << "'"
                  << "\tType: " << tokenTypeToString(token.type) // Assumes tokenTypeToString is in Lexer.h
                  << "\t(Line: " << token.line
                  << ", Col: " << token.col << ")\n";
    }
    std::cout << std::endl;

    Parser parser(tokens);
    // parser.parse() now returns std::shared_ptr<ProgramNode>
    std::shared_ptr<ProgramNode> ast_root = parser.parse();

    std::cout << "=== AST ===\n";
    if (ast_root) {
        printAST(ast_root);
    } else {
        std::cout << "Parsing failed to produce an AST root." << std::endl;
    }

    return 0;
}