#include <iostream>
#include <fstream>
#include <vector>       // For std::vector
#include <string>       // For std::string
#include <memory>       // For std::shared_ptr
#include "transpiler.h" // Contains Lexer, Parser, AST nodes, and Transpiler
// Ensure Lexer.h, Parser.h and their .cpp are correctly set up
// and "transpiler.h" correctly includes them or provides their definitions.

using namespace std;

// Helper to print indentation
void printIndent(int indent)
{
    for (int i = 0; i < indent; ++i)
        cout << "  ";
}

// Forward declaration
void printAST(const shared_ptr<ASTNode> &node, int indent = 0);

// Generic node printer that handles all node types
void printAST(const shared_ptr<ASTNode> &node, int indent)
{
    if (!node)
    {
        printIndent(indent);
        cout << "(nullptr)" << endl;
        return;
    }

    // Print node type and specific information based on node type
    if (auto p = dynamic_pointer_cast<ProgramNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        for (const auto &stmt : p->getStatements())
        {
            printAST(stmt, indent + 1);
        }
    }
    else if (auto p = dynamic_pointer_cast<BlockNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        for (const auto &stmt : p->getStatements())
        {
            printAST(stmt, indent + 1);
        }
    }
    else if (auto p = dynamic_pointer_cast<ExpressionStatementNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        printAST(p->getExpression(), indent + 1);
    }
    else if (auto p = dynamic_pointer_cast<PrintfNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        printIndent(indent + 1);
        cout << "FormatString:" << endl;
        printAST(p->getFormatStringExpression(), indent + 2);
        const auto &args = p->getArguments();
        if (!args.empty())
        {
            printIndent(indent + 1);
            cout << "Arguments:" << endl;
            for (const auto &arg : args)
            {
                printAST(arg, indent + 2);
            }
        }
    }
    else if (auto p = dynamic_pointer_cast<ScanfNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        printIndent(indent + 1);
        cout << "FormatString:" << endl;
        printAST(p->getFormatStringExpression(), indent + 2);
        const auto &args = p->getArguments();
        if (!args.empty())
        {
            printIndent(indent + 1);
            cout << "Arguments:" << endl;
            for (const auto &arg : args)
            {
                printAST(arg, indent + 2);
            }
        }
    }
    else if (auto p = dynamic_pointer_cast<IfNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        printIndent(indent + 1);
        cout << "Condition:" << endl;
        printAST(p->getCondition(), indent + 2);
        printIndent(indent + 1);
        cout << "ThenBranch:" << endl;
        printAST(p->getThenBranch(), indent + 2);
        if (p->getElseBranch())
        {
            printIndent(indent + 1);
            cout << "ElseBranch:" << endl;
            printAST(p->getElseBranch(), indent + 2);
        }
    }
    else if (auto p = dynamic_pointer_cast<WhileNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        printIndent(indent + 1);
        cout << "Condition:" << endl;
        printAST(p->getCondition(), indent + 2);
        printIndent(indent + 1);
        cout << "Body:" << endl;
        printAST(p->getBody(), indent + 2);
    }
    else if (auto p = dynamic_pointer_cast<ForNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        if (p->getInitializer())
        {
            printIndent(indent + 1);
            cout << "Initializer:" << endl;
            printAST(p->getInitializer(), indent + 2);
        }
        else
        {
            printIndent(indent + 1);
            cout << "Initializer: (empty)" << endl;
        }
        if (p->getCondition())
        {
            printIndent(indent + 1);
            cout << "Condition:" << endl;
            printAST(p->getCondition(), indent + 2);
        }
        else
        {
            printIndent(indent + 1);
            cout << "Condition: (empty)" << endl;
        }
        if (p->getIncrement())
        {
            printIndent(indent + 1);
            cout << "Increment:" << endl;
            printAST(p->getIncrement(), indent + 2);
        }
        else
        {
            printIndent(indent + 1);
            cout << "Increment: (empty)" << endl;
        }
        printIndent(indent + 1);
        cout << "Body:" << endl;
        printAST(p->getBody(), indent + 2);
    }
    else if (auto p = dynamic_pointer_cast<ReturnNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        if (p->getReturnValue())
        {
            printIndent(indent + 1);
            cout << "Value:" << endl;
            printAST(p->getReturnValue(), indent + 2);
        }
        else
        {
            printIndent(indent + 1);
            cout << "Value: (void)" << endl;
        }
    }
    else if (auto p = dynamic_pointer_cast<BreakNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
    }
    else if (auto p = dynamic_pointer_cast<ContinueNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
    }
    else if (auto p = dynamic_pointer_cast<VariableDeclarationNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << "): " << p->getDeclaredType() << " " << p->getName() << endl;
        if (p->getInitializer())
        {
            printIndent(indent + 1);
            cout << "Initializer:" << endl;
            printAST(p->getInitializer(), indent + 2);
        }
    }
    else if (auto p = dynamic_pointer_cast<FunctionDeclarationNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << "): " << p->getDeclaredType() << " " << p->getName() << "(";
        const auto paramNames = p->getParamNames();
        const auto paramTypes = p->getParamTypes();
        for (size_t i = 0; i < paramNames.size(); ++i)
        {
            cout << paramTypes[i] << " " << paramNames[i] << (i < paramNames.size() - 1 ? ", " : "");
        }
        cout << ")" << endl;
        if (p->getBody())
        {
            printIndent(indent + 1);
            cout << "Body:" << endl;
            printAST(p->getBody(), indent + 2);
        }
        else
        {
            printIndent(indent + 1);
            cout << "(Forward Declaration / No Body)" << endl;
        }
    }
    else if (auto p = dynamic_pointer_cast<AssignmentStatementNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        printAST(p->getAssignment(), indent + 1);
    }
    else if (auto p = dynamic_pointer_cast<AssignmentNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << "): Target '" << p->getTargetName() << "' =" << endl;
        printIndent(indent + 1);
        cout << "Value:" << endl;
        printAST(p->getValue(), indent + 2);
    }
    else if (auto p = dynamic_pointer_cast<BinaryExpressionNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << "): Operator '" << p->getOperator() << "'" << endl;
        printIndent(indent + 1);
        cout << "Left:" << endl;
        printAST(p->getLeft(), indent + 2);
        printIndent(indent + 1);
        cout << "Right:" << endl;
        printAST(p->getRight(), indent + 2);
    }
    else if (auto p = dynamic_pointer_cast<UnaryExpressionNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << "): Operator '" << p->getOperator() << "'" << endl;
        printIndent(indent + 1);
        cout << "Operand:" << endl;
        printAST(p->getOperand(), indent + 2);
    }
    else if (auto p = dynamic_pointer_cast<IdentifierNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << "): " << p->getName() << endl;
    }
    else if (auto p = dynamic_pointer_cast<FunctionCallNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << "): " << p->getFunctionName() << endl;
        const auto &args = p->getArguments();
        if (!args.empty())
        {
            printIndent(indent + 1);
            cout << "Arguments:" << endl;
            for (const auto &arg : args)
            {
                printAST(arg, indent + 2);
            }
        }
        else
        {
            printIndent(indent + 1);
            cout << "Arguments: (none)" << endl;
        }
    }
    else if (auto p = dynamic_pointer_cast<StringLiteralNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << "): \"" << p->getValue() << "\"" << endl;
    }
    else if (auto p = dynamic_pointer_cast<CharLiteralNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << "): '" << p->getValue() << "'" << endl;
    }
    else if (auto p = dynamic_pointer_cast<NumberNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << "): " << p->getValue() << endl;
    }
    else if (auto p = dynamic_pointer_cast<BooleanNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << "): " << (p->getValue() ? "true" : "false") << endl;
    }
    else
    { // Fallback for unknown node types or types not explicitly handled above
        printIndent(indent);
        cout << "Unknown or unhandled ASTNode type: " << (node->type_name.empty() ? "(no type_name field set)" : node->type_name) << endl;

        const auto &genericChildren = node->getChildren();
        if (!genericChildren.empty())
        {
            // The following booleans are used to avoid printing generic children
            // if they are already explicitly printed by specific handlers above.
            // This logic is to satisfy IntelliSense about converting shared_ptr to bool.
            bool isPrintfOrScanf = (dynamic_pointer_cast<PrintfNode>(node) != nullptr) ||
                                   (dynamic_pointer_cast<ScanfNode>(node) != nullptr);
            bool isExpressionStmt = (dynamic_pointer_cast<ExpressionStatementNode>(node) != nullptr); // Line 243
            bool isProgramOrBlock = (dynamic_pointer_cast<ProgramNode>(node) != nullptr) ||
                                    (dynamic_pointer_cast<BlockNode>(node) != nullptr);
            bool isReturnNode = (dynamic_pointer_cast<ReturnNode>(node) != nullptr);           // Line 245
            bool isVarDecl = (dynamic_pointer_cast<VariableDeclarationNode>(node) != nullptr); // Line 246
            bool isAssignment = (dynamic_pointer_cast<AssignmentNode>(node) != nullptr);       // Line 247
            bool isBinaryExpr = (dynamic_pointer_cast<BinaryExpressionNode>(node) != nullptr); // Line 248
            bool isUnaryExpr = (dynamic_pointer_cast<UnaryExpressionNode>(node) != nullptr);   // Line 249
            bool isFuncCall = (dynamic_pointer_cast<FunctionCallNode>(node) != nullptr);       // Line 250
            // Add any other node types that manage their primary 'children' through ASTNode::children directly
            // and are also explicitly handled above.

            // If the node type isn't one that we *know* prints its children via specific getters...
            if (!isPrintfOrScanf && !isExpressionStmt && !isProgramOrBlock &&
                !isReturnNode && !isVarDecl && !isAssignment &&
                !isBinaryExpr && !isUnaryExpr && !isFuncCall)
            {
                printIndent(indent + 1);
                cout << "Generic Children:" << endl;
                for (const auto &child : genericChildren)
                {
                    printAST(child, indent + 2);
                }
            }
        }
    }
}

int main()
{
    // === Step 1: Read code from stdin ===
    string line, source_code;
    char ch;
    while (cin.get(ch))
    {
        source_code += ch;
    }
    if (source_code.empty() && !cin.eof() && !cin.bad() && !cin.fail())
    {
        // Only show this specific error if not an expected empty input due to immediate EOF
        // And make sure stream state is not bad/fail (which could be why it's empty)
        //  if (isatty(fileno(stdin))) { // Heuristic: only print for interactive tty
        //      // This can be noisy for piped empty input
        //      // cerr << "No source code entered." << endl;
        //  }
    }
    else if (cin.bad() || (cin.fail() && !cin.eof()))
    {
        cerr << "Failed to read source code from stdin due to stream error." << endl;
        return 1;
    }

    // === Step 2: Lexical Analysis ===
    Lexer lexer(source_code);
    vector<Token> tokens;
    try
    {
        tokens = lexer.tokenize();
    }
    catch (const std::exception &e)
    {
        cerr << "Lexical Error: " << e.what() << endl;
        return 1;
    }
    // ADD THIS: Get defined macros
    const auto &definedMacros = lexer.getDefinedMacros();

    cout << "---TOKENS---" << endl;
    for (const auto &token : tokens)
    {
        cout << " " << token.value << " ---->("
             << tokenTypeToString(token.type) << ") line: "
             << token.line << ", col: " << token.col << endl;
    }

    cout << "---TOKENS---" << endl;
    for (const auto &token : tokens)
    {
        // It's good practice to check tokenTypeToString can handle all TokenType enum values
        cout << " " << token.value << " ---->("
             << tokenTypeToString(token.type) << ") line: "
             << token.line << ", col: " << token.col << endl;
    }

    cout << "\n---DEFINED MACROS---" << endl;
    if (definedMacros.empty())
    {
        cout << "(No macros defined or parsed)" << endl;
    }
    for (const auto &macro : definedMacros)
    {
        if (!macro.valid)
        {
            cout << "Invalid Macro (skipped): " << macro.name << " (defined on line " << macro.line << ")" << endl;
            continue;
        }
        cout << "Macro: " << macro.name;
        if (macro.isFunctionLike)
        {
            cout << "(";
            for (size_t i = 0; i < macro.parameters.size(); ++i)
            {
                cout << macro.parameters[i] << (i < macro.parameters.size() - 1 ? ", " : "");
            }
            cout << ")";
        }
        cout << " -> \"" << macro.body << "\" (Line: " << macro.line << ")" << endl;
    }
    // === Step 3: Parse tokens into AST ===
    Parser parser(tokens);
    shared_ptr<ProgramNode> ast_root = parser.parse(); // parser.parse() should not return nullptr based on its impl

    cout << "---AST---" << endl;
    // ast_root itself will be non-null.
    // We print it regardless; if parsing failed internally, ProgramNode might be empty
    // and parser would have printed errors to cerr.
    printAST(ast_root);

    // === Step 4: Transpile to Python ===
    Transpiler transpiler;
    string python_code;
    try
    {
        // MODIFY THIS: Pass definedMacros to transpiler
        python_code = transpiler.transpile(ast_root, definedMacros);
    }
    catch (const std::exception &e)
    {
        cerr << "Transpilation Error: " << e.what() << endl;
    }

    cout << "\n---PYTHON_CODE---" << endl;
    cout << python_code << endl;
    return 0;
}