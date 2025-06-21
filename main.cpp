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

    else if (auto p = dynamic_pointer_cast<ArrayDeclarationNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << "): " << p->getDeclaredType() << " " << p->getName();
        cout << "[";
        if (p->getSizeExpression())
        {
            // For brevity, if it's a simple number, print it, else just "expr"
            if (auto sizeNum = dynamic_pointer_cast<NumberNode>(p->getSizeExpression()))
            {
                cout << sizeNum->getValue();
            }
            else
            {
                cout << "expr"; // Placeholder for complex expression
            }
        }
        else
        {
            cout << "NO_SIZE_EXPR"; // Should ideally not happen if parser validates
        }
        cout << "]" << endl;
        // If VariableDeclarationNode (base) has an initializer member that ArrayDeclarationNode uses:
        if (p->getInitializer())
        { // Check if this method exists and is used
            printIndent(indent + 1);
            cout << "Initializer (from base):" << endl;
            printAST(p->getInitializer(), indent + 2);
        }
    } // --- ADD ArraySubscriptNode PRINTER ---
    else if (auto p = dynamic_pointer_cast<ArraySubscriptNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << ")" << endl;
        printIndent(indent + 1);
        cout << "Array Expression:" << endl;
        printAST(p->getArrayExpression(), indent + 2);
        printIndent(indent + 1);
        cout << "Index Expression:" << endl;
        printAST(p->getIndexExpression(), indent + 2);
    } 
    else if (auto p = dynamic_pointer_cast<VariableDeclarationNode>(node)) // This should come AFTER ArrayDeclarationNode if ArrayDecl inherits from VarDecl
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
    // REPLACE the old FunctionDeclarationNode block inside printAST with this one:
    else if (auto p = dynamic_pointer_cast<FunctionDeclarationNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << "): " << p->getDeclaredType() << " " << p->getName() << "(";

        // Use the new getParameters() method
        const auto &params = p->getParameters();
        for (size_t i = 0; i < params.size(); ++i)
        {
            // Print type and name
            cout << params[i].type << " " << params[i].name;
            // If it's an array, print the brackets!
            if (params[i].isArray)
            {
                cout << "[]";
            }
            // Add comma if not the last parameter
            if (i < params.size() - 1)
            {
                cout << ", ";
            }
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
        // AssignmentNode is an expression, its operator is implicitly '='
        cout << "(" << p->type_name << ") Operator '='" << endl;
        printIndent(indent + 1);
        cout << "LValue (Target):" << endl;
        printAST(p->getLValue(), indent + 2); // Assumes getLValue()
        printIndent(indent + 1);
        cout << "RValue (Value):" << endl;
        printAST(p->getRValue(), indent + 2); // Assumes getRValue()
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
    else if (auto p = dynamic_pointer_cast<ArrayDeclarationNode>(node))
    {
        printIndent(indent);
        cout << "(" << p->type_name << "): " << p->getDeclaredType() << " " << p->getName();
        cout << "[";
        if (p->getSizeExpression())
        {
            // For brevity, if it's a simple number, print it, else just "expr"
            if (auto sizeNum = dynamic_pointer_cast<NumberNode>(p->getSizeExpression()))
            {
                cout << sizeNum->getValue();
            }
            else
            {
                // Or you can recurse to print the full expression:
                // cout << endl; printAST(p->getSizeExpression(), indent + 1); printIndent(indent);
                cout << "expr"; // Placeholder for complex expression
            }
        }
    } 
        else // Fallback for unknown node types or types not explicitly handled above
        {
            printIndent(indent);
            string type_name_str = node->type_name.empty() ? typeid(*node).name() : node->type_name;
            cout << "Unknown or unhandled ASTNode type: " << type_name_str << endl;

            const auto &genericChildren = node->getChildren();
            // Check if it's a type whose children are explicitly handled by its own getter methods
            // This list helps avoid redundant printing of children.
            bool children_explicitly_handled =
                (dynamic_pointer_cast<ProgramNode>(node) != nullptr) ||
                (dynamic_pointer_cast<BlockNode>(node) != nullptr) ||
                (dynamic_pointer_cast<ExpressionStatementNode>(node) != nullptr) ||
                (dynamic_pointer_cast<PrintfNode>(node) != nullptr) ||
                (dynamic_pointer_cast<ScanfNode>(node) != nullptr) ||
                (dynamic_pointer_cast<IfNode>(node) != nullptr) ||
                (dynamic_pointer_cast<WhileNode>(node) != nullptr) ||
                (dynamic_pointer_cast<ForNode>(node) != nullptr) ||
                (dynamic_pointer_cast<ReturnNode>(node) != nullptr) ||
                (dynamic_pointer_cast<ArrayDeclarationNode>(node) != nullptr) || // Added to this check
                (dynamic_pointer_cast<ArraySubscriptNode>(node) != nullptr) ||   // Added to this check
                (dynamic_pointer_cast<VariableDeclarationNode>(node) != nullptr) ||
                (dynamic_pointer_cast<FunctionDeclarationNode>(node) != nullptr) ||
                (dynamic_pointer_cast<AssignmentNode>(node) != nullptr) || // Updated if structure changed
                (dynamic_pointer_cast<BinaryExpressionNode>(node) != nullptr) ||
                (dynamic_pointer_cast<UnaryExpressionNode>(node) != nullptr) ||
                (dynamic_pointer_cast<FunctionCallNode>(node) != nullptr);

            if (!genericChildren.empty() && !children_explicitly_handled)
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