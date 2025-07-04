#include "Parser.h"
#include <stdexcept>
#include <iostream> // For cerr

// This unescapeLiteralContent function assumes 's' is the PURE CONTENT
// (i.e., already stripped of any outer quotes)
// but MAY contain escape sequences like \n, \\, \', \"
string Parser::unescapeLiteralContent(const string &s)
{
    string res;
    res.reserve(s.length());
    for (size_t i = 0; i < s.length(); ++i)
    {
        if (s[i] == '\\' && i + 1 < s.length())
        {
            i++;
            switch (s[i])
            {
            case 'n':
                res += '\n';
                break;
            case 't':
                res += '\t';
                break;
            case 'r':
                res += '\r';
                break;
            case '\\':
                res += '\\';
                break;
            case '\'':
                res += '\'';
                break; // Escape for single quote within a char (if parser expects 'content')
            case '"':
                res += '"';
                break; // Escape for double quote within a string (if parser expects "content")
            case '0':
                res += '\0';
                break;
            default:
                // For unknown escapes, just pass them through as literal backslash + char
                res += '\\';
                res += s[i];
                break;
            }
        }
        else if (s[i] == '\\')
        {                // Dangling backslash at the end
            res += '\\'; // Treat as a literal backslash
        }
        else
        {
            res += s[i];
        }
    }
    return res;
}

Parser::Parser(const vector<Token> &tokens) : tokens(tokens), current(0) {}

shared_ptr<ProgramNode> Parser::parse()
{
    try
    {
        return parseProgram();
    }
    catch (const runtime_error &e)
    {
        cerr << "Parse error: " << e.what() << endl;
        if (current < tokens.size() && tokens[current].type != TokenType::EndOfFile)
        {
            cerr << "Error occurred near token: " << tokens[current].toString()
                 << " (type: " << tokenTypeToString(tokens[current].type)
                 << ", line: " << tokens[current].line << ")" << endl;
        }
        else if (!tokens.empty() && current > 0)
        {
            cerr << "Error might be after token: " << tokens[current - 1].toString()
                 << " (type: " << tokenTypeToString(tokens[current - 1].type)
                 << ", line: " << tokens[current - 1].line << ")" << endl;
        }
        return make_shared<ProgramNode>();
    }
}

shared_ptr<ProgramNode> Parser::parseProgram()
{
    auto programNode = make_shared<ProgramNode>();
    while (!isAtEnd())
    {
        try
        {
            shared_ptr<StatementNode> stmt = parseStatement();
            if (stmt)
            {
                programNode->addChild(stmt);
            }
        }
        catch (const runtime_error &e)
        {
            cerr << "Error parsing statement: " << e.what() << endl;
            if (current < tokens.size() && tokens[current].type != TokenType::EndOfFile)
            {
                cerr << "Error occurred near token: " << tokens[current].toString()
                     << " (type: " << tokenTypeToString(tokens[current].type)
                     << ", line: " << tokens[current].line << ")" << endl;
            }
            else if (!tokens.empty() && current > 0)
            {
                cerr << "Error might be after token: " << tokens[current - 1].toString()
                     << " (type: " << tokenTypeToString(tokens[current - 1].type)
                     << ", line: " << tokens[current - 1].line << ")" << endl;
            }
            synchronize();
            if (isAtEnd())
                break;
        }
    }
    return programNode;
}

shared_ptr<StatementNode> Parser::parseStatement()
{
    if (match(TokenType::Keyword, "if"))
        return parseIf();
    if (match(TokenType::Keyword, "while"))
        return parseWhile();
    if (match(TokenType::Keyword, "for"))
        return parseFor();
    if (match(TokenType::Keyword, "return"))
        return parseReturn();
    if (match(TokenType::Keyword, "break"))
        return parseBreak();
    if (match(TokenType::Keyword, "continue"))
        return parseContinue();
    if (match(TokenType::Symbol, "{"))
        return parseBlock();

    if (check(TokenType::Identifier, "printf") && peek(1).type == TokenType::Symbol && peek(1).value == "(")
    {
        return parsePrintfStatement();
    }
    if (check(TokenType::Identifier, "scanf") && peek(1).type == TokenType::Symbol && peek(1).value == "(")
    {
        return parseScanfStatement();
    }

    if (check(TokenType::Keyword, "int") || check(TokenType::Keyword, "float") ||
        check(TokenType::Keyword, "char") || check(TokenType::Keyword, "bool") ||
        check(TokenType::Keyword, "string") || check(TokenType::Keyword, "void"))
    {
        return parseDeclaration();
    }

    // This call will now handle expression statements, including those that are assignments.
    return parseExpressionStatement();
}

shared_ptr<PrintfNode> Parser::parsePrintfStatement()
{
    advance(); // Consume 'printf' identifier
    auto printfNode = make_shared<PrintfNode>();
    consume(TokenType::Symbol, "(", "Expected '(' after 'printf'.");

    // First argument must be a format string (StringLiteral)
    if (check(TokenType::StringLiteral))
    {
        // parsePrimary() handles StringLiteral correctly and returns StringLiteralNode
        auto formatStringExpr = parsePrimary();
        // The PrintfNode's getFormatStringExpression expects an ExpressionNode,
        // which StringLiteralNode is. A visitor would do the specific cast.
        printfNode->addChild(formatStringExpr);
    }
    else
    {
        // For stricter parsing if first arg MUST be a literal:
        throw runtime_error("Expected a string literal as the first argument to printf. Got: " + peek().toString());
    }

    while (match(TokenType::Symbol, ","))
    {
        printfNode->addChild(parseExpression());
    }

    consume(TokenType::Symbol, ")", "Expected ')' after printf arguments.");
    consume(TokenType::Symbol, ";", "Expected ';' after printf statement.");
    return printfNode;
}

shared_ptr<ScanfNode> Parser::parseScanfStatement()
{
    advance(); // Consume 'scanf' identifier
    auto scanfNode = make_shared<ScanfNode>();
    consume(TokenType::Symbol, "(", "Expected '(' after 'scanf'.");

    if (check(TokenType::StringLiteral))
    {
        auto formatStringExpr = parsePrimary();
        scanfNode->addChild(formatStringExpr);
    }
    else
    {
        throw runtime_error("Expected a string literal as the first argument to scanf. Got: " + peek().toString());
    }

    while (match(TokenType::Symbol, ","))
    {
        scanfNode->addChild(parseExpression()); // Arguments typically like &var
    }

    consume(TokenType::Symbol, ")", "Expected ')' after scanf arguments.");
    consume(TokenType::Symbol, ";", "Expected ';' after scanf statement.");
    return scanfNode;
}

shared_ptr<AssignmentStatementNode> Parser::parseAssignmentStatement()
{
    string identifierName = consume(TokenType::Identifier, "Expected identifier in assignment statement.").value;
    consume(TokenType::Operator, "=", "Expected '=' after identifier in assignment statement.");
    auto value = parseExpression();
    consume(TokenType::Symbol, ";", "Expected ';' after assignment statement.");
    // auto assignNode = make_shared<AssignmentNode>(identifierName);
    // assignNode->addChild(value);
    auto targetLValue = make_shared<IdentifierNode>(identifierName);    // Create an IdentifierNode for the left-hand side
    auto assignNode = make_shared<AssignmentNode>(targetLValue, value); // Use the (ExpressionNode, ExpressionNode) constructor
    return make_shared<AssignmentStatementNode>(assignNode);
}

shared_ptr<ExpressionStatementNode> Parser::parseExpressionStatement()
{
    auto expr = parseExpression();
    consume(TokenType::Symbol, ";", "Expected ';' after expression statement.");
    return make_shared<ExpressionStatementNode>(expr);
}

shared_ptr<BlockNode> Parser::parseBlock()
{
    auto blockNode = make_shared<BlockNode>();
    while (!check(TokenType::Symbol, "}") && !isAtEnd())
    {
        blockNode->addChild(parseStatement());
    }
    consume(TokenType::Symbol, "}", "Expected '}' after block.");
    return blockNode;
}

shared_ptr<IfNode> Parser::parseIf()
{
    auto ifNode = make_shared<IfNode>();
    consume(TokenType::Symbol, "(", "Expected '(' after 'if'.");
    ifNode->setCondition(parseExpression());
    consume(TokenType::Symbol, ")", "Expected ')' after if condition.");
    ifNode->setThenBranch(parseStatement());
    if (match(TokenType::Keyword, "else"))
    {
        ifNode->setElseBranch(parseStatement());
    }
    return ifNode;
}

shared_ptr<WhileNode> Parser::parseWhile()
{
    auto whileNode = make_shared<WhileNode>();
    consume(TokenType::Symbol, "(", "Expected '(' after 'while'.");
    whileNode->setCondition(parseExpression());
    consume(TokenType::Symbol, ")", "Expected ')' after while condition.");
    whileNode->setBody(parseStatement());
    return whileNode;
}

shared_ptr<ForNode> Parser::parseFor()
{
    // 'for' keyword was matched
    auto forNode = make_shared<ForNode>();
    consume(TokenType::Symbol, "(", "Expected '(' after 'for'.");
    // Initializer
    if (!check(TokenType::Symbol, ";"))
    {
        if (check(TokenType::Keyword, "int") ||
            check(TokenType::Keyword, "float"))
        {
            // Parse variable declaration but *without* consuming the type keyword yet,
            // as parseVariableDeclaration expects to do that if hints are empty.
            // Pass empty hints so parseVariableDeclaration consumes the type itself.
            forNode->setInitializer(parseVariableDeclaration("", ""));
        }
        else
        {
            forNode->setInitializer(parseExpressionStatement());
        }
    }
    else
    {
        consume(TokenType::Symbol, ";", "Expected ';' for empty initializer in for loop.");
        forNode->setInitializer(nullptr);
    }

    // Condition
    if (!check(TokenType::Symbol, ";"))
    {
        forNode->setCondition(parseExpression());
    } // else condition is nullptr, which is fine (infinite loop unless break)
    consume(TokenType::Symbol, ";", "Expected ';' after for condition.");

    // Increment
    if (!check(TokenType::Symbol, ")"))
    {
        forNode->setIncrement(parseExpression());
    } // else increment is nullptr
    consume(TokenType::Symbol, ")", "Expected ')' after for clauses.");

    forNode->setBody(parseStatement());
    return forNode;
}

shared_ptr<ReturnNode> Parser::parseReturn()
{
    auto returnNode = make_shared<ReturnNode>();
    if (!check(TokenType::Symbol, ";"))
    {
        returnNode->addChild(parseExpression());
    }
    consume(TokenType::Symbol, ";", "Expected ';' after return statement.");
    return returnNode;
}

shared_ptr<BreakNode> Parser::parseBreak()
{
    consume(TokenType::Symbol, ";", "Expected ';' after break statement.");
    return make_shared<BreakNode>();
}

shared_ptr<ContinueNode> Parser::parseContinue()
{
    consume(TokenType::Symbol, ";", "Expected ';' after continue statement.");
    return make_shared<ContinueNode>();
}

// shared_ptr<StatementNode> Parser::parseDeclaration()
// {
//     string typeStr = advance().value;
//     string identifierStr = consume(TokenType::Identifier, "Expected identifier after type in declaration.").value;
//     if (check(TokenType::Symbol, "("))
//     {
//         return parseFunctionDeclaration(typeStr, identifierStr);
//     }
//     else
//     {
//         return parseVariableDeclaration(typeStr, identifierStr);
//     }
// }
shared_ptr<StatementNode> Parser::parseDeclaration()
{
    string typeStr = advance().value; // e.g., "int"
    string identifierStr = consume(TokenType::Identifier, "Expected identifier after type in declaration.").value;

    if (check(TokenType::Symbol, "["))
    {                                      // Check for array declaration
        advance();                         // Consume '['
        auto sizeExpr = parseExpression(); // Parse the size (e.g., 10)
        consume(TokenType::Symbol, "]", "Expected ']' after array size in declaration.");

        // TODO: Add logic for multi-dimensional arrays here if desired (loop for more '[size]')
        // For now, single dimension

        auto arrayDeclNode = make_shared<ArrayDeclarationNode>(identifierStr, typeStr, sizeExpr);

        // Optional: Handle C-style initializers e.g. int arr[3] = {1, 2, 3};
        // This is a more complex parsing step. For now, we assume no explicit initializer list here.
        // If an '=' is found, you might parse an ArrayInitializerListNode.
        if (match(TokenType::Operator, "="))
        {
            // For now, we'll just log and consume until semicolon if initializers aren't supported.
            cerr << "Parser Warning (Line " << previous().line << "): Array initializer found for '"
                 << identifierStr << "' but full parsing for initializers is not yet implemented. Skipping initializer." << endl;
            while (!isAtEnd() && !check(TokenType::Symbol, ";"))
            {
                advance();
            }
            // Alternatively, attempt to parse it as a generic expression (might not be what you want)
            // or throw an error:
            // throw runtime_error("Array initializers e.g. '{1,2,3}' are not yet supported in declarations.");
        }

        consume(TokenType::Symbol, ";", "Expected ';' after array declaration.");
        return arrayDeclNode;
    }
    else if (check(TokenType::Symbol, "("))
    { // Existing function declaration check
        return parseFunctionDeclaration(typeStr, identifierStr);
    }
    else
    { // Existing scalar variable declaration
        // Reuse the logic from parseVariableDeclaration, but pass typeStr and identifierStr
        // or inline the relevant parts.
        // Assuming parseVariableDeclaration can take these as hints or arguments now:
        return parseVariableDeclaration(typeStr, identifierStr);
    }
}

shared_ptr<VariableDeclarationNode> Parser::parseVariableDeclaration(
    const string &typeHint, const string &identifierHint)
{

    string actualType = typeHint;
    string actualIdentifier = identifierHint;

    if (actualType.empty())
    {
        if (!(check(TokenType::Keyword, "int") || check(TokenType::Keyword, "float") ||
              check(TokenType::Keyword, "char") || check(TokenType::Keyword, "bool") ||
              check(TokenType::Keyword, "string") || check(TokenType::Keyword, "void")))
        {
            throw runtime_error("Expected type keyword for variable declaration, got " + peek().toString());
        }
        actualType = advance().value;
    }
    if (actualIdentifier.empty())
    {
        actualIdentifier = consume(TokenType::Identifier, "Expected identifier in variable declaration.").value;
    }

    auto varDeclNode = make_shared<VariableDeclarationNode>(actualIdentifier, actualType);
    if (match(TokenType::Operator, "="))
    {
        varDeclNode->addChild(parseExpression());
    }
    consume(TokenType::Symbol, ";", "Expected ';' after variable declaration.");
    return varDeclNode;
}

// REPLACE the old parseFunctionDeclaration with this one:
shared_ptr<FunctionDeclarationNode> Parser::parseFunctionDeclaration(
    const string &returnType, const string &identifier)
{
    auto funcDeclNode = make_shared<FunctionDeclarationNode>(identifier, returnType);
    consume(TokenType::Symbol, "(", "Expected '(' after function name for parameters.");

    if (!check(TokenType::Symbol, ")"))
    {
        do
        {
            // This is the new logic:
            Parameter currentParam; // Create a new Parameter object

            // 1. Parse type
            if (!(check(TokenType::Keyword, "int") || check(TokenType::Keyword, "float") ||
                  check(TokenType::Keyword, "char") || check(TokenType::Keyword, "bool") ||
                  check(TokenType::Keyword, "string") || check(TokenType::Keyword, "void")))
            {
                throw runtime_error("Expected type keyword for function parameter, got " + peek().toString());
            }
            currentParam.type = advance().value;

            // 2. Parse name
            currentParam.name = consume(TokenType::Identifier, "Expected parameter name.").value;

            // 3. NEW: Check for array syntax `[]`
            if (match(TokenType::Symbol, "["))
            {
                consume(TokenType::Symbol, "]", "Expected ']' after '[' in array parameter declaration.");
                currentParam.isArray = true; // Mark it as an array!
            }

            // 4. Add the completed parameter to the node
            funcDeclNode->addParameter(currentParam);

        } while (match(TokenType::Symbol, ","));
    }
    consume(TokenType::Symbol, ")", "Expected ')' after parameters or empty parameter list.");

    if (match(TokenType::Symbol, "{"))
    {
        funcDeclNode->setBody(parseBlock());
    }
    else
    {
        consume(TokenType::Symbol, ";", "Expected '{' for function body or ';' for function prototype.");
    }
    return funcDeclNode;
}

// Expression parsing
shared_ptr<ExpressionNode> Parser::parseExpression()
{
    return parseAssignmentExpression();
}

shared_ptr<ExpressionNode> Parser::parseAssignmentExpression()
{
    auto left_expr = parseLogicalOr(); // This can parse identifiers, array_subscripts, etc.

    if (match(TokenType::Operator, "="))
    {
        Token assign_op_token = previous();            // The '=' token itself
        auto right_expr = parseAssignmentExpression(); // Right-associativity for assignment

        // Check if left_expr is a valid L-value (can be assigned to)
        // For now, we'll accept IdentifierNode and ArraySubscriptNode
        if (dynamic_pointer_cast<IdentifierNode>(left_expr) ||
            dynamic_pointer_cast<ArraySubscriptNode>(left_expr)
            /* TODO: Add other valid L-value types here, e.g., MemberAccessNode for obj.field, UnaryExpressionNode for *ptr */
        )
        {
            // Create the new AssignmentNode that takes two ExpressionNode children
            return make_shared<AssignmentNode>(left_expr, right_expr);
        }
        else
        {
            // If left_expr is something like "5 = x" or "(a+b) = x", it's an error
            string left_type_name = "null";
            if (left_expr)
            {
                left_type_name = left_expr->type_name.empty() ? typeid(*left_expr).name() : left_expr->type_name;
            }
            throw runtime_error("Invalid assignment target. Left-hand side of '=' must be an identifier or assignable expression. Got: " +
                                left_type_name +
                                " near token: " + assign_op_token.toString());
        }
    }
    return left_expr; // If no '=', it's just the expression parsed by parseLogicalOr()
}

shared_ptr<ExpressionNode> Parser::parseLogicalOr()
{
    return parseBinaryExpression([this]()
                                 { return parseLogicalAnd(); }, {"||"});
}

shared_ptr<ExpressionNode> Parser::parseLogicalAnd()
{
    return parseBinaryExpression([this]()
                                 { return parseEquality(); }, {"&&"});
}

shared_ptr<ExpressionNode> Parser::parseEquality()
{
    return parseBinaryExpression([this]()
                                 { return parseComparison(); }, {"==", "!="});
}

shared_ptr<ExpressionNode> Parser::parseComparison()
{
    return parseBinaryExpression([this]()
                                 { return parseTerm(); }, {"<", ">", "<=", ">="});
}

shared_ptr<ExpressionNode> Parser::parseTerm()
{
    return parseBinaryExpression([this]()
                                 { return parseFactor(); }, {"+", "-"});
}

shared_ptr<ExpressionNode> Parser::parseFactor()
{
    return parseBinaryExpression([this]()
                                 { return parseUnary(); }, {"*", "/", "%"});
}

shared_ptr<ExpressionNode> Parser::parseUnary()
{
    if (check(TokenType::Operator, "!") ||
        check(TokenType::Operator, "-") ||
        check(TokenType::Operator, "&") ||
        check(TokenType::Operator, "++") || // <-- ADDED THIS
        check(TokenType::Operator, "--"))   // <-- ADDED THIS
    {                                       // Added '&' for address-of
        string op = advance().value;
        auto operand = parseUnary(); // Right-associative for unary operators
        auto unaryNode = make_shared<UnaryExpressionNode>(op);
        unaryNode->addChild(operand);
        return unaryNode;
    }
    return parseCall();
}

shared_ptr<ExpressionNode> Parser::parseCall()
{
    auto expr = parsePrimary(); // Parses identifier, literal, (grouped_expr), etc.

    while (true)
    {
        if (match(TokenType::Symbol, "("))
        { // Function call
            // ... (existing function call logic that sets expr = callNode)
            // ...
            if (auto identNode = dynamic_pointer_cast<IdentifierNode>(expr))
            {
                auto callNode = make_shared<FunctionCallNode>(identNode->getName());
                if (!check(TokenType::Symbol, ")"))
                {
                    do
                    {
                        callNode->addChild(parseExpression());
                    } while (match(TokenType::Symbol, ","));
                }
                consume(TokenType::Symbol, ")", "Expected ')' after function call arguments.");
                expr = callNode; // Update expr to be the call node
            }
            else
            {
                throw runtime_error("Expression before '(' is not a simple callable identifier for function call.");
            }
        }
        else if (match(TokenType::Symbol, "["))
        { // NEW: Array Subscript
            auto indexExpr = parseExpression();
            consume(TokenType::Symbol, "]", "Expected ']' after array index.");
            expr = make_shared<ArraySubscriptNode>(expr, indexExpr); // Update expr to be the subscript node
            // For multi-dimensional: loop here for more '[]'
        }
        else if (match(TokenType::Operator, "++") || match(TokenType::Operator, "--"))
        {
            // This is a postfix increment/decrement operator
            string op = previous().value; // Get the "++" or "--"
            // The operand is the expression we parsed *before* the operator
            auto postfix_unary_node = make_shared<UnaryExpressionNode>(op);
            postfix_unary_node->addChild(expr);
            // The whole thing (e.g., "i++") becomes the new expression
            expr = postfix_unary_node;
        }
        else
        {
            break; // No more postfix operators
        }
    }
    return expr;
}
// MODIFIED parsePrimary METHOD:
shared_ptr<ExpressionNode> Parser::parsePrimary()
{
    // Handle BooleanLiteral first if it's a distinct type from the lexer
    if (check(TokenType::BooleanLiteral))
    { // Assuming BooleanLiteral is a defined TokenType
        Token boolToken = advance();
        if (boolToken.value == "true")
            return make_shared<BooleanNode>(true);
        if (boolToken.value == "false")
            return make_shared<BooleanNode>(false);
        // Should not happen if lexer is correct for BooleanLiteral tokens
        throw runtime_error("Invalid boolean literal token value from lexer: " + boolToken.toString());
    }

    // Fallback or primary way if lexer uses Keywords for booleans
    if (match(TokenType::Keyword, "true"))
        return make_shared<BooleanNode>(true);
    if (match(TokenType::Keyword, "false"))
        return make_shared<BooleanNode>(false);

    if (check(TokenType::IntegerNumber) || check(TokenType::FloatNumber))
    {
        return make_shared<NumberNode>(advance().value);
    }

    if (match(TokenType::StringLiteral))
    {
        // ASSUMPTION from error: lexer provides token.value as the content WITHOUT surrounding quotes.
        string content_from_lexer = previous().value; // e.g., "hello" or "%d"
        return make_shared<StringLiteralNode>(unescapeLiteralContent(content_from_lexer));
    }

    if (match(TokenType::CharLiteral))
    {
        // ASSUMPTION from error: lexer provides token.value as the content WITHOUT surrounding quotes.
        string content_from_lexer = previous().value; // e.g., "a" or "n" (if lexer processes '\n' to "n")
                                                      // or actual '\n' char if lexer makes value a string with that.
                                                      // unescapeLiteralContent needs to handle this consistently.
                                                      // Let's assume content_from_lexer can be like "n" for input '\n'.
        string unescaped_content = unescapeLiteralContent(content_from_lexer);

        if (unescaped_content.length() != 1)
        {
            throw runtime_error("Character literal content must resolve to a single character after unescaping. Got: '" +
                                unescaped_content + "' from original token value: " + content_from_lexer);
        }
        return make_shared<CharLiteralNode>(unescaped_content);
    }

    if (match(TokenType::Identifier))
    {
        // Check if this identifier might be "printf" or "scanf" being used as an expression (less common, but possible)
        // This typically shouldn't happen here if printf/scanf are parsed as statements first in parseStatement()
        // But if they appear in an expression context where a primary is expected, they're just identifiers here.
        return make_shared<IdentifierNode>(previous().value);
    }

    if (match(TokenType::Symbol, "("))
    {
        auto expr = parseExpression();
        consume(TokenType::Symbol, ")", "Expected ')' after grouped expression.");
        return expr;
    }

    // If no primary expression matched
    throw runtime_error("Expected primary expression, but got " +
                        peek().toString() + " (type: " + tokenTypeToString(peek().type) +
                        ", line: " + to_string(peek().line) + ")");
}

shared_ptr<ExpressionNode> Parser::parseBinaryExpression(
    function<shared_ptr<ExpressionNode>()> parseOperand,
    const vector<string> &operators)
{
    auto left = parseOperand();
    while (true)
    {
        bool matchedThisIteration = false;
        for (const auto &opStr : operators)
        {
            // Operators can be TokenType::Operator (like +, *) or TokenType::Symbol (like && from C)
            if (check(TokenType::Operator, opStr) || check(TokenType::Symbol, opStr))
            {
                Token opToken = advance();
                auto right = parseOperand();
                auto binaryNode = make_shared<BinaryExpressionNode>(opToken.value);
                binaryNode->addChild(left);
                binaryNode->addChild(right);
                left = binaryNode;
                matchedThisIteration = true;
                break;
            }
        }
        if (!matchedThisIteration)
            break;
    }
    return left;
}

// Token handling utility methods
Token Parser::advance()
{
    if (!isAtEnd())
        current++;
    return previous();
}

Token Parser::peek(int offset) const
{
    long long target_idx_long = static_cast<long long>(current) + offset;
    if (target_idx_long < 0 || static_cast<size_t>(target_idx_long) >= tokens.size())
    {
        if (!tokens.empty() && tokens.back().type == TokenType::EndOfFile)
            return tokens.back(); // Return EOF
        throw std::out_of_range("Peek offset out of bounds for token stream (index: " +
                                to_string(target_idx_long) + ", size: " + to_string(tokens.size()) + ").");
    }
    return tokens[static_cast<size_t>(target_idx_long)];
}

Token Parser::previous() const
{
    if (current == 0)
        throw out_of_range("No previous token at the beginning of the stream.");
    return tokens[current - 1];
}

bool Parser::isAtEnd() const
{
    return current >= tokens.size() || tokens[current].type == TokenType::EndOfFile;
}

bool Parser::match(TokenType type)
{
    if (check(type))
    {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(TokenType type, const string &value)
{
    if (check(type, value))
    {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const
{
    if (isAtEnd())
        return false;
    return tokens[current].type == type;
}

bool Parser::check(TokenType type, const string &value) const
{
    if (isAtEnd())
        return false;
    return tokens[current].type == type && tokens[current].value == value;
}

Token Parser::consume(TokenType type, const string &message)
{
    if (check(type))
    {
        return advance();
    }
    string errorMsg = message + " Expected " + tokenTypeToString(type);
    if (!isAtEnd())
    {
        errorMsg += ", but got " + tokens[current].toString() +
                    " (type: " + tokenTypeToString(tokens[current].type) +
                    ", line: " + to_string(tokens[current].line) + ")";
    }
    else
    {
        errorMsg += ", but found end of file.";
    }
    errorMsg += " (at token index " + to_string(current) + ")";
    throw runtime_error(errorMsg);
}

void Parser::consume(TokenType type, const string &value, const string &message)
{
    if (check(type, value))
    {
        advance();
        return;
    }
    string errorMsg = message + " Expected '" + value + "' (type " + tokenTypeToString(type) + ")";
    if (!isAtEnd())
    {
        errorMsg += ", but got " + tokens[current].toString() +
                    " (type: " + tokenTypeToString(tokens[current].type) +
                    ", value: '" + tokens[current].value + "'" +
                    ", line: " + to_string(tokens[current].line) + ")";
    }
    else
    {
        errorMsg += ", but found end of file.";
    }
    errorMsg += " (at token index " + to_string(current) + ")";
    throw runtime_error(errorMsg);
}

void Parser::synchronize()
{
    if (isAtEnd())
        return;
    advance();
    while (!isAtEnd())
    {
        if (current > 0 && tokens[current - 1].type == TokenType::Symbol && tokens[current - 1].value == ";")
        {
            return;
        }
        switch (peek().type)
        {
        case TokenType::Keyword:
            if (peek().value == "if" || peek().value == "while" || peek().value == "for" ||
                peek().value == "return" || peek().value == "break" || peek().value == "continue" ||
                peek().value == "print" || peek().value == "int" || peek().value == "float" ||
                peek().value == "char" || peek().value == "bool" || peek().value == "string" ||
                peek().value == "void")
            {
                return;
            }
            break;
        case TokenType::Identifier: // Added Identifier check for printf/scanf like starts
            if ((peek().value == "printf" || peek().value == "scanf") &&
                (current + 1 < tokens.size() && tokens[current + 1].type == TokenType::Symbol && tokens[current + 1].value == "("))
            {
                return;
            }
            break;
        case TokenType::Symbol:
            if (peek().value == "{" || peek().value == "}")
            {
                return;
            }
            break;
        default:
            break;
        }
        advance();
    }
}