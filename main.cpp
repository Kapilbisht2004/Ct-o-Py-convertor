#include <iostream>
#include <fstream>
#include "transpiler.h"
using namespace std;

// Helper to print indentation
void printIndent(int indent) {
    for (int i = 0; i < indent; ++i) cout << "  ";
}

// Forward declaration
void printAST(const shared_ptr<ASTNode> &node, int indent = 0);

// Generic node printer that handles all node types
void printAST(const shared_ptr<ASTNode> &node, int indent) {
    if (!node) {
        printIndent(indent);
        cout << "(nullptr)" << endl;
        return;
    }

    // Print node type and specific information based on node type
    if (auto p = dynamic_pointer_cast<ProgramNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        for (const auto &stmt : p->getStatements()) {
            printAST(stmt, indent + 1);
        }
    }
    else if (auto p = dynamic_pointer_cast<BlockNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        for (const auto &stmt : p->getStatements()) {
            printAST(stmt, indent + 1);
        }
    }
    else if (auto p = dynamic_pointer_cast<ExpressionStatementNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        printAST(p->getExpression(), indent + 1);
    }
    else if (auto p = dynamic_pointer_cast<IfNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        printIndent(indent + 1); cout << "Condition:" << endl;
        printAST(p->getCondition(), indent + 2);
        printIndent(indent + 1); cout << "ThenBranch:" << endl;
        printAST(p->getThenBranch(), indent + 2);
        if (p->getElseBranch()) {
            printIndent(indent + 1); cout << "ElseBranch:" << endl;
            printAST(p->getElseBranch(), indent + 2);
        }
    }
    else if (auto p = dynamic_pointer_cast<WhileNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        printIndent(indent + 1); cout << "Condition:" << endl;
        printAST(p->getCondition(), indent + 2);
        printIndent(indent + 1); cout << "Body:" << endl;
        printAST(p->getBody(), indent + 2);
    }
    else if (auto p = dynamic_pointer_cast<ForNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        if (p->getInitializer()) {
            printIndent(indent + 1); cout << "Initializer:" << endl;
            printAST(p->getInitializer(), indent + 2);
        }
        if (p->getCondition()) {
            printIndent(indent + 1); cout << "Condition:" << endl;
            printAST(p->getCondition(), indent + 2);
        }
        if (p->getIncrement()) {
            printIndent(indent + 1); cout << "Increment:" << endl;
            printAST(p->getIncrement(), indent + 2);
        }
        printIndent(indent + 1); cout << "Body:" << endl;
        printAST(p->getBody(), indent + 2);
    }
    else if (auto p = dynamic_pointer_cast<ReturnNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        if (p->getReturnValue()) {
            printIndent(indent + 1); cout << "Value:" << endl;
            printAST(p->getReturnValue(), indent + 2);
        }
    }
    else if (auto p = dynamic_pointer_cast<BreakNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
    }
    else if (auto p = dynamic_pointer_cast<ContinueNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
    }
    else if (auto p = dynamic_pointer_cast<VariableDeclarationNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << "): " << p->getDeclaredType() << " " << p->getName() << endl;
        if (p->getInitializer()) {
            printIndent(indent + 1); cout << "Initializer:" << endl;
            printAST(p->getInitializer(), indent + 2);
        }
    }
    else if (auto p = dynamic_pointer_cast<FunctionDeclarationNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << "): " << p->getDeclaredType() << " " << p->getName() << "(";
        const auto &paramNames = p->getParamNames();
        const auto &paramTypes = p->getParamTypes();
        for (size_t i = 0; i < paramNames.size(); ++i) {
            cout << paramTypes[i] << " " << paramNames[i] << (i < paramNames.size() - 1 ? ", " : "");
        }
        cout << ")" << endl;
        if (p->getBody()) {
            printIndent(indent + 1); cout << "Body:" << endl;
            printAST(p->getBody(), indent + 2);
        } else {
            printIndent(indent + 1); cout << "(Forward Declaration / No Body)" << endl;
        }
    }
    else if (auto p = dynamic_pointer_cast<AssignmentStatementNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        printAST(p->getAssignment(), indent + 1);
    }
    else if (auto p = dynamic_pointer_cast<AssignmentNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << "): " << p->getTargetName() << " =" << endl;
        printIndent(indent + 1); cout << "Value:" << endl;
        printAST(p->getValue(), indent + 2);
    }
    else if (auto p = dynamic_pointer_cast<BinaryExpressionNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << "): Operator '" << p->getOperator() << "'" << endl;
        printIndent(indent + 1); cout << "Left:" << endl;
        printAST(p->getLeft(), indent + 2);
        printIndent(indent + 1); cout << "Right:" << endl;
        printAST(p->getRight(), indent + 2);
    }
    else if (auto p = dynamic_pointer_cast<UnaryExpressionNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << "): Operator '" << p->getOperator() << "'" << endl;
        printIndent(indent + 1); cout << "Operand:" << endl;
        printAST(p->getOperand(), indent + 2);
    }
    else if (auto p = dynamic_pointer_cast<IdentifierNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << "): " << p->getName() << endl;
    }
    else if (auto p = dynamic_pointer_cast<FunctionCallNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << "): " << p->getFunctionName() << endl;
        const auto &args = p->getArguments();
        if (!args.empty()) {
            printIndent(indent + 1); cout << "Arguments:" << endl;
            for (const auto &arg : args) {
                printAST(arg, indent + 2);
            }
        }
    }
    else if (auto p = dynamic_pointer_cast<StringLiteralNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << "): \"" << p->getValue() << "\"" << endl;
    }
    else if (auto p = dynamic_pointer_cast<CharLiteralNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << "): '" << p->getValue() << "'" << endl;
    }
    else if (auto p = dynamic_pointer_cast<NumberNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << "): " << p->getValue() << endl;
    }
    else if (auto p = dynamic_pointer_cast<BooleanNode>(node)) {
        printIndent(indent);
        cout << "(" << p->type_name << "): " << (p->getValue() ? "true" : "false") << endl;
    }
    else {
        printIndent(indent);
        cout << "Unknown or unhandled ASTNode type: " << (node->type_name.empty() ? "(no type_name)" : node->type_name) << endl;
        // Generic child printing for unhandled complex nodes
        if (!node->getChildren().empty()) {
            printIndent(indent + 1); cout << "Generic Children:" << endl;
            for (const auto &child : node->getChildren()) {
                printAST(child, indent + 2);
            }
        }
    }
}

int main() {
    // === Step 1: Read code from stdin ===
    string line, source_code;
    while (getline(cin, line)) {
        source_code += line + "\n";
    }

    // === Step 2: Lexical Analysis ===
    Lexer lexer(source_code);
    vector<Token> tokens = lexer.tokenize();

    cout << "---TOKENS---\n";
    for (const auto& token : tokens) {
        cout << " " << token.value << " ---->("
             << tokenTypeToString(token.type) << ") line: "
             << token.line << ", col: " << token.col << endl;
    }

    // === Step 3: Parse tokens into AST ===
    Parser parser(tokens);
    shared_ptr<ProgramNode> ast_root = parser.parse();

    cout << "---AST---\n";
    if (ast_root) {
        printAST(ast_root);
    } else {
        cerr << "Parsing failed to produce an AST root." << endl;
        return 1;
    }

    // === Step 4: Transpile to Python ===
    Transpiler transpiler;
    string python_code = transpiler.transpile(ast_root);

    cout << "---PYTHON_CODE---\n";
    cout << python_code << endl;

    return 0;
}