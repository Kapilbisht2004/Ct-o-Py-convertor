#include <iostream>
#include <string>
#include <vector>
#include <memory> // For shared_ptr, dynamic_pointer_cast
#include "Lexer.h"  // Assumed to provide Token, TokenType, tokenTypeToString, and Lexer class
#include "Parser.h" // Provides ASTNode derived classes and Parser class
#include "Transpiler.h"
using namespace std;

// Helper to print indentation
void printIndent(int indent) {
    for (int i = 0; i < indent; ++i) {
        cout << "  ";
    }
}

// Forward declaration for printAST as some nodes might recursively call it for children
void printAST(const shared_ptr<ASTNode>& node, int indent = 0);

// Specialized print functions for clarity or if dynamic_cast is too verbose in a switch
void printProgramNode(const shared_ptr<ProgramNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << ")" << endl;
    for (const auto& stmt : node->getStatements()) {
        printAST(stmt, indent + 1);
    }
}

void printBlockNode(const shared_ptr<BlockNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << ")" << endl;
    for (const auto& stmt : node->getStatements()) {
        printAST(stmt, indent + 1);
    }
}

void printExpressionStatementNode(const shared_ptr<ExpressionStatementNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << ")" << endl;
    printAST(node->getExpression(), indent + 1);
}

void printIfNode(const shared_ptr<IfNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << ")" << endl;
    printIndent(indent + 1); cout << "Condition:" << endl;
    printAST(node->getCondition(), indent + 2);
    printIndent(indent + 1); cout << "ThenBranch:" << endl;
    printAST(node->getThenBranch(), indent + 2);
    if (node->getElseBranch()) {
        printIndent(indent + 1); cout << "ElseBranch:" << endl;
        printAST(node->getElseBranch(), indent + 2);
    }
}

void printWhileNode(const shared_ptr<WhileNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << ")" << endl;
    printIndent(indent + 1); cout << "Condition:" << endl;
    printAST(node->getCondition(), indent + 2);
    printIndent(indent + 1); cout << "Body:" << endl;
    printAST(node->getBody(), indent + 2);
}

void printForNode(const shared_ptr<ForNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << ")" << endl;
    if (node->getInitializer()) {
        printIndent(indent + 1); cout << "Initializer:" << endl;
        printAST(node->getInitializer(), indent + 2);
    }
    if (node->getCondition()) {
        printIndent(indent + 1); cout << "Condition:" << endl;
        printAST(node->getCondition(), indent + 2);
    }
    if (node->getIncrement()) {
        printIndent(indent + 1); cout << "Increment:" << endl;
        printAST(node->getIncrement(), indent + 2);
    }
    printIndent(indent + 1); cout << "Body:" << endl;
    printAST(node->getBody(), indent + 2);
}

void printReturnNode(const shared_ptr<ReturnNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << ")" << endl;
    if (node->getReturnValue()) {
        printIndent(indent + 1); cout << "Value:" << endl;
        printAST(node->getReturnValue(), indent + 2);
    }
}

void printVariableDeclarationNode(const shared_ptr<VariableDeclarationNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << "): "
              << node->getDeclaredType() << " " << node->getName() << endl;
    if (node->getInitializer()) {
        printIndent(indent + 1); cout << "Initializer:" << endl;
        printAST(node->getInitializer(), indent + 2);
    }
}

void printFunctionDeclarationNode(const shared_ptr<FunctionDeclarationNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << "): "
              << node->getDeclaredType() << " " << node->getName() << "(";
    const auto& paramNames = node->getParamNames();
    const auto& paramTypes = node->getParamTypes();
    for (size_t i = 0; i < paramNames.size(); ++i) {
        cout << paramTypes[i] << " " << paramNames[i] << (i < paramNames.size() - 1 ? ", " : "");
    }
    cout << ")" << endl;
    if (node->getBody()) {
        printIndent(indent + 1); cout << "Body:" << endl;
        printAST(node->getBody(), indent + 2);
    } else {
        printIndent(indent + 1); cout << "(Forward Declaration / No Body)" << endl;
    }
}

void printAssignmentStatementNode(const shared_ptr<AssignmentStatementNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << ")" << endl;
    printAST(node->getAssignment(), indent + 1); // Print the inner AssignmentNode
}

void printAssignmentNode(const shared_ptr<AssignmentNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << "): " << node->getTargetName() << " =" << endl;
    printIndent(indent + 1); cout << "Value:" << endl;
    printAST(node->getValue(), indent + 2);
}

void printBinaryExpressionNode(const shared_ptr<BinaryExpressionNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << "): Operator '" << node->getOperator() << "'" << endl;
    printIndent(indent + 1); cout << "Left:" << endl;
    printAST(node->getLeft(), indent + 2);
    printIndent(indent + 1); cout << "Right:" << endl;
    printAST(node->getRight(), indent + 2);
}

void printUnaryExpressionNode(const shared_ptr<UnaryExpressionNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << "): Operator '" << node->getOperator() << "'" << endl;
    printIndent(indent + 1); cout << "Operand:" << endl;
    printAST(node->getOperand(), indent + 2);
}

void printIdentifierNode(const shared_ptr<IdentifierNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << "): " << node->getName() << endl;
}

void printFunctionCallNode(const shared_ptr<FunctionCallNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << "): " << node->getFunctionName() << endl;
    const auto& args = node->getArguments();
    if (!args.empty()) {
        printIndent(indent + 1); cout << "Arguments:" << endl;
        for (const auto& arg : args) {
            printAST(arg, indent + 2);
        }
    }
}

void printStringLiteralNode(const shared_ptr<StringLiteralNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << "): \"" << node->getValue() << "\"" << endl;
}

void printCharLiteralNode(const shared_ptr<CharLiteralNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << "): '" << node->getValue() << "'" << endl;
}

void printNumberNode(const shared_ptr<NumberNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << "): " << node->getValue() << endl;
}

void printBooleanNode(const shared_ptr<BooleanNode>& node, int indent) {
    printIndent(indent);
    cout << "(" << node->type_name << "): " << (node->getValue() ? "true" : "false") << endl;
}

void printAST(const shared_ptr<ASTNode>& node, int indent) {
    if (!node) {
        printIndent(indent);
        cout << "(nullptr)" << endl;
        return;
    }

    // Using dynamic_pointer_cast to determine the type and call specialized printers
    // This is safer and more object-oriented than relying on node->type_name string comparisons directly for logic.
    if (auto p = dynamic_pointer_cast<ProgramNode>(node)) printProgramNode(p, indent);
    else if (auto p = dynamic_pointer_cast<BlockNode>(node)) printBlockNode(p, indent);
    else if (auto p = dynamic_pointer_cast<ExpressionStatementNode>(node)) printExpressionStatementNode(p, indent);
    else if (auto p = dynamic_pointer_cast<IfNode>(node)) printIfNode(p, indent);
    else if (auto p = dynamic_pointer_cast<WhileNode>(node)) printWhileNode(p, indent);
    else if (auto p = dynamic_pointer_cast<ForNode>(node)) printForNode(p, indent);
    else if (auto p = dynamic_pointer_cast<ReturnNode>(node)) printReturnNode(p, indent);
    else if (auto p = dynamic_pointer_cast<BreakNode>(node)) { printIndent(indent); cout << "BreakNode (" << p->type_name << ")" << endl; }
    else if (auto p = dynamic_pointer_cast<ContinueNode>(node)) { printIndent(indent); cout << "ContinueNode (" << p->type_name << ")" << endl; }
    else if (auto p = dynamic_pointer_cast<VariableDeclarationNode>(node)) printVariableDeclarationNode(p, indent);
    else if (auto p = dynamic_pointer_cast<FunctionDeclarationNode>(node)) printFunctionDeclarationNode(p, indent);
    else if (auto p = dynamic_pointer_cast<AssignmentStatementNode>(node)) printAssignmentStatementNode(p, indent);
    else if (auto p = dynamic_pointer_cast<AssignmentNode>(node)) printAssignmentNode(p, indent);
    else if (auto p = dynamic_pointer_cast<BinaryExpressionNode>(node)) printBinaryExpressionNode(p, indent);
    else if (auto p = dynamic_pointer_cast<UnaryExpressionNode>(node)) printUnaryExpressionNode(p, indent);
    else if (auto p = dynamic_pointer_cast<IdentifierNode>(node)) printIdentifierNode(p, indent);
    else if (auto p = dynamic_pointer_cast<FunctionCallNode>(node)) printFunctionCallNode(p, indent);
    else if (auto p = dynamic_pointer_cast<StringLiteralNode>(node)) printStringLiteralNode(p, indent);
    else if (auto p = dynamic_pointer_cast<CharLiteralNode>(node)) printCharLiteralNode(p, indent);
    else if (auto p = dynamic_pointer_cast<NumberNode>(node)) printNumberNode(p, indent);
    else if (auto p = dynamic_pointer_cast<BooleanNode>(node)) printBooleanNode(p, indent);
    else {
        printIndent(indent);
        cout << "Unknown or unhandled ASTNode type: " << (node->type_name.empty() ? "(no type_name)" : node->type_name) << endl;
        // Generic child printing for unhandled complex nodes
        if(!node->getChildren().empty()){
            printIndent(indent + 1);
            cout << "Generic Children:" << endl;
            for(const auto& child : node->getChildren()){
                printAST(child, indent + 2);
            }
        }
    }
}


int main() {
    string source_code = R"(
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
            return 0; // Return with no value (void context)
        }
    )";

    Lexer lexer(source_code);
    vector<Token> tokens = lexer.tokenize();

    cout << "=== Tokens ===\n";
    for (const auto& token : tokens) {
        cout << "Token: '" << token.value << "'"
                  << "\tType: " << tokenTypeToString(token.type) // Assumes tokenTypeToString is in Lexer.h
                  << "\t(Line: " << token.line
                  << ", Col: " << token.col << ")\n";
    }
    cout << endl;

    Parser parser(tokens);
    // parser.parse() now returns shared_ptr<ProgramNode>
    shared_ptr<ProgramNode> ast_root = parser.parse();

    cout << "=== AST ===\n";
    if (ast_root) {
        printAST(ast_root);
    } else {
        cout << "Parsing failed to produce an AST root." << endl;
    }

    cout << "\n=== Python Code ===\n";
    Transpiler transpiler(ast_root);
    string python_code = transpiler.transpile();
    cout << python_code;

    return 0;
}