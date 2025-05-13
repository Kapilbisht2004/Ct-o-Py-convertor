#include "Parser.h"
#include <stdexcept>
#include <iostream> // For std::cerr

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), current(0) {}

// Changed return type to ProgramNode to match declaration
std::shared_ptr<ProgramNode> Parser::parse() {
    try {
        return parseProgram();
    } catch (const std::runtime_error& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
        // Depending on severity, you might want to return nullptr or an empty ProgramNode
        return std::make_shared<ProgramNode>(); // Return an empty program on major error
    }
}

std::shared_ptr<ProgramNode> Parser::parseProgram() {
    auto programNode = std::make_shared<ProgramNode>();
    while (!isAtEnd()) {
        try {
            programNode->addChild(parseStatement());
        } catch (const std::runtime_error& e) {
            std::cerr << "Error parsing statement: " << e.what() << std::endl;
            synchronize(); // Attempt to recover
        }
    }
    return programNode;
}

std::shared_ptr<StatementNode> Parser::parseStatement() {
    if (match(TokenType::Keyword, "if")) return parseIf();
    if (match(TokenType::Keyword, "while")) return parseWhile();
    if (match(TokenType::Keyword, "for")) return parseFor();
    if (match(TokenType::Keyword, "return")) return parseReturn();
    if (match(TokenType::Keyword, "break")) return parseBreak();
    if (match(TokenType::Keyword, "continue")) return parseContinue();
    if (match(TokenType::Symbol, "{")) return parseBlock();

    // Check for type keywords for declarations
    if (check(TokenType::Keyword, "int") || check(TokenType::Keyword, "char") ||
        check(TokenType::Keyword, "bool") || check(TokenType::Keyword, "string") ||
        check(TokenType::Keyword, "void")) {
        // Peek ahead to see if it's a function declaration (identifier followed by '(')
        // or variable declaration. parseDeclaration will handle this.
        return parseDeclaration();
    }

    // Check for assignment statement: Identifier = ... ;
    // This must come before general expression statement
    if (check(TokenType::Identifier) && peek(1).type == TokenType::Operator && peek(1).value == "=") {
        return parseAssignmentStatement();
    }
    
    return parseExpressionStatement();
}

std::shared_ptr<AssignmentStatementNode> Parser::parseAssignmentStatement() {
    // Assumes current token is Identifier, next is '='
    std::string identifier = consume(TokenType::Identifier, "Expected identifier in assignment statement.").value;
    consume(TokenType::Operator, "=", "Expected '=' in assignment statement."); // Consume '='

    auto value = parseExpression(); // Parse the right-hand side expression
    consume(TokenType::Symbol, ";", "Expected ';' after assignment statement.");

    auto assignNode = std::make_shared<AssignmentNode>(identifier);
    assignNode->addChild(value); // The assigned value is a child of AssignmentNode

    return std::make_shared<AssignmentStatementNode>(assignNode);
}


std::shared_ptr<ExpressionStatementNode> Parser::parseExpressionStatement() {
    auto expr = parseExpression();
    consume(TokenType::Symbol, ";", "Expected ';' after expression statement.");
    return std::make_shared<ExpressionStatementNode>(expr);
}

std::shared_ptr<BlockNode> Parser::parseBlock() {
    // Opening '{' was matched by parseStatement to enter here
    auto blockNode = std::make_shared<BlockNode>();
    while (!check(TokenType::Symbol, "}") && !isAtEnd()) {
        blockNode->addChild(parseStatement());
    }
    consume(TokenType::Symbol, "}", "Expected '}' after block.");
    return blockNode;
}

std::shared_ptr<IfNode> Parser::parseIf() {
    // 'if' keyword was matched by parseStatement
    auto ifNode = std::make_shared<IfNode>();
    consume(TokenType::Symbol, "(", "Expected '(' after 'if'.");
    ifNode->setCondition(parseExpression());
    consume(TokenType::Symbol, ")", "Expected ')' after if condition.");
    ifNode->setThenBranch(parseStatement());

    if (match(TokenType::Keyword, "else")) {
        ifNode->setElseBranch(parseStatement());
    }
    return ifNode;
}

std::shared_ptr<WhileNode> Parser::parseWhile() {
    // 'while' keyword was matched
    auto whileNode = std::make_shared<WhileNode>();
    consume(TokenType::Symbol, "(", "Expected '(' after 'while'.");
    whileNode->setCondition(parseExpression());
    consume(TokenType::Symbol, ")", "Expected ')' after while condition.");
    whileNode->setBody(parseStatement());
    return whileNode;
}

std::shared_ptr<ForNode> Parser::parseFor() {
    // 'for' keyword was matched
    auto forNode = std::make_shared<ForNode>();
    consume(TokenType::Symbol, "(", "Expected '(' after 'for'.");

    // Initializer
    if (!match(TokenType::Symbol, ";")) { // If not an empty initializer part
        if (check(TokenType::Keyword, "int") || check(TokenType::Keyword, "char") ||
            check(TokenType::Keyword, "bool") || check(TokenType::Keyword, "string")) {
            forNode->setInitializer(parseVariableDeclaration()); // Expects var decl ending in ';'
        } else {
            forNode->setInitializer(parseExpressionStatement()); // Expects expr ending in ';'
        }
    } else {
        // Semicolon was matched, initializer is null (or an "empty statement node" if needed)
         forNode->setInitializer(nullptr); // Or a specific EmptyStatementNode
    }


    // Condition
    if (!check(TokenType::Symbol, ";")) { // If there's a condition part
        forNode->setCondition(parseExpression());
    }
    consume(TokenType::Symbol, ";", "Expected ';' after for condition.");

    // Increment
    if (!check(TokenType::Symbol, ")")) { // If there's an increment part
        forNode->setIncrement(parseExpression()); // The expression itself, not an ExprStmt
    }
    consume(TokenType::Symbol, ")", "Expected ')' after for clauses.");

    forNode->setBody(parseStatement());
    return forNode;
}

std::shared_ptr<ReturnNode> Parser::parseReturn() {
    // 'return' keyword was matched
    auto returnNode = std::make_shared<ReturnNode>();
    if (!check(TokenType::Symbol, ";")) {
        returnNode->addChild(parseExpression());
    }
    consume(TokenType::Symbol, ";", "Expected ';' after return statement.");
    return returnNode;
}

std::shared_ptr<BreakNode> Parser::parseBreak() {
    // 'break' keyword was matched
    consume(TokenType::Symbol, ";", "Expected ';' after break statement.");
    return std::make_shared<BreakNode>();
}

std::shared_ptr<ContinueNode> Parser::parseContinue() {
    // 'continue' keyword was matched
    consume(TokenType::Symbol, ";", "Expected ';' after continue statement.");
    return std::make_shared<ContinueNode>();
}

std::shared_ptr<StatementNode> Parser::parseDeclaration() {
    // Type keyword (int, char, etc.) was checked by parseStatement
    // It's not consumed yet by parseStatement if it calls parseDeclaration directly.
    // However, the original logic might have assumed `advance()` then `consume(identifier)` *before* deciding
    // based on '('. Let's stick to that.

    std::string typeStr = advance().value; // Consume type keyword (e.g., "int")
    std::string identifierStr = consume(TokenType::Identifier, "Expected identifier after type in declaration.").value;

    if (check(TokenType::Symbol, "(")) {
        return parseFunctionDeclaration(typeStr, identifierStr);
    } else {
        return parseVariableDeclaration(typeStr, identifierStr);
    }
}

std::shared_ptr<VariableDeclarationNode> Parser::parseVariableDeclaration(
    const std::string& typeHint, const std::string& identifierHint) {
    
    std::string actualType = typeHint;
    std::string actualIdentifier = identifierHint;

    // If hints are empty, it means this was called from a context like 'for' loop init
    // where the type keyword is current, and identifier is next.
    if (actualType.empty()) {
        // Caller (e.g. parseFor) must ensure current token IS a type keyword
        // e.g., check(TokenType::Keyword, "int")
        actualType = advance().value; // Consume the type keyword
    }
    if (actualIdentifier.empty()) {
        actualIdentifier = consume(TokenType::Identifier, "Expected identifier in variable declaration.").value;
    }

    auto varDeclNode = std::make_shared<VariableDeclarationNode>(actualIdentifier, actualType);
    if (match(TokenType::Operator, "=")) {
        varDeclNode->addChild(parseExpression()); // Initializer is a child
    }
    consume(TokenType::Symbol, ";", "Expected ';' after variable declaration.");
    return varDeclNode;
}

std::shared_ptr<FunctionDeclarationNode> Parser::parseFunctionDeclaration(
    const std::string& returnType, const std::string& identifier) {
    // Type and identifier name are already consumed and passed as arguments.
    // Current token is '('.
    auto funcDeclNode = std::make_shared<FunctionDeclarationNode>(identifier, returnType);
    consume(TokenType::Symbol, "(", "Expected '(' after function name.");

    if (!check(TokenType::Symbol, ")")) {
        do {
            // Ensure type is a keyword, not just any identifier
            if (!(check(TokenType::Keyword, "int") || check(TokenType::Keyword, "char") ||
                  check(TokenType::Keyword, "bool") || check(TokenType::Keyword, "string") ||
                  check(TokenType::Keyword, "void"))) {
                throw std::runtime_error("Expected type keyword for parameter, got " + peek().toString());
            }
            std::string paramType = advance().value; // Consume param type
            std::string paramName = consume(TokenType::Identifier, "Expected parameter name.").value;
            funcDeclNode->addParameter(paramName, paramType);
        } while (match(TokenType::Symbol, ","));
    }
    consume(TokenType::Symbol, ")", "Expected ')' after parameters.");

    if (match(TokenType::Symbol, "{")) { // Function definition with body
        auto bodyBlock = std::make_shared<BlockNode>();
         while (!check(TokenType::Symbol, "}") && !isAtEnd()) {
            bodyBlock->addChild(parseStatement());
        }
        consume(TokenType::Symbol, "}", "Expected '}' after function body.");
        funcDeclNode->setBody(bodyBlock);
    } else {
        consume(TokenType::Symbol, ";", "Expected ';' after function declaration (for prototype or missing body).");
        // Body remains nullptr for prototype
    }
    return funcDeclNode;
}


// Expression parsing (Pratt parser style or precedence climbing)
std::shared_ptr<ExpressionNode> Parser::parseExpression() {
    return parseAssignmentExpression(); // Lowest precedence: assignment
}

std::shared_ptr<ExpressionNode> Parser::parseAssignmentExpression() {
    auto left = parseLogicalOr(); // Parse higher precedence stuff first

    if (match(TokenType::Operator, "=")) { // Assignment operator
        // Assignment is right-associative
        // Target must be an l-value (e.g., IdentifierNode)
        if (auto identNode = std::dynamic_pointer_cast<IdentifierNode>(left)) {
            auto value = parseAssignmentExpression(); // Recursively parse RHS
            auto assignNode = std::make_shared<AssignmentNode>(identNode->getName());
            assignNode->addChild(value);
            return assignNode;
        }
        // Could also handle other l-values like array access or member access here
        throw std::runtime_error("Invalid assignment target. Expected identifier, got " + left->type_name);
    }
    return left; // Not an assignment
}

std::shared_ptr<ExpressionNode> Parser::parseLogicalOr() {
    return parseBinaryExpression([this]() { return parseLogicalAnd(); }, {"||"});
}

std::shared_ptr<ExpressionNode> Parser::parseLogicalAnd() {
    return parseBinaryExpression([this]() { return parseEquality(); }, {"&&"});
}

std::shared_ptr<ExpressionNode> Parser::parseEquality() {
    return parseBinaryExpression([this]() { return parseComparison(); }, {"==", "!="});
}

std::shared_ptr<ExpressionNode> Parser::parseComparison() {
    return parseBinaryExpression([this]() { return parseTerm(); }, {"<", ">", "<=", ">="});
}

std::shared_ptr<ExpressionNode> Parser::parseTerm() {
    return parseBinaryExpression([this]() { return parseFactor(); }, {"+", "-"});
}

std::shared_ptr<ExpressionNode> Parser::parseFactor() {
    return parseBinaryExpression([this]() { return parseUnary(); }, {"*", "/", "%"});
}

std::shared_ptr<ExpressionNode> Parser::parseUnary() {
    if (match(TokenType::Operator, "!") || match(TokenType::Operator, "-")) {
        std::string op = previous().value;
        auto operand = parseUnary(); // Unary operators are often right-associative
        auto unaryNode = std::make_shared<UnaryExpressionNode>(op);
        unaryNode->addChild(operand);
        return unaryNode;
    }
    return parseCall(); // Or primary, if call is part of primary or a separate higher precedence
}

std::shared_ptr<ExpressionNode> Parser::parseCall() {
    auto expr = parsePrimary(); // Parse the "base" of the call (e.g., function name)

    while (true) {
        if (match(TokenType::Symbol, "(")) { // Function call
            if (auto identNode = std::dynamic_pointer_cast<IdentifierNode>(expr)) {
                auto callNode = std::make_shared<FunctionCallNode>(identNode->getName());
                if (!check(TokenType::Symbol, ")")) {
                    do {
                        callNode->addChild(parseExpression()); // Arguments are expressions
                    } while (match(TokenType::Symbol, ","));
                }
                consume(TokenType::Symbol, ")", "Expected ')' after function call arguments.");
                expr = callNode; // The result of the call is the new current expression
            } else {
                // e.g. (1+2)() is not a valid call
                throw std::runtime_error("Expression before '(' is not a callable identifier.");
            }
        } /* else if (match(TokenType::Symbol, "[")) { // Array access
            // ... handle array access ...
            // expr = ... make ArrayAccessNode ...
        } else if (match(TokenType::Symbol, ".")) { // Member access
            // ... handle member access ...
            // expr = ... make MemberAccessNode ...
        } */
        else {
            break; // No more call-like operators
        }
    }
    return expr;
}

std::shared_ptr<ExpressionNode> Parser::parsePrimary() {
    if (match(TokenType::Keyword, "true")) return std::make_shared<BooleanNode>(true);
    if (match(TokenType::Keyword, "false")) return std::make_shared<BooleanNode>(false);
    if (match(TokenType::IntegerNumber) || match(TokenType::FloatNumber)) return std::make_shared<NumberNode>(previous().value);
    if (match(TokenType::StringLiteral)) return std::make_shared<StringLiteralNode>(previous().value);
    if (match(TokenType::CharLiteral)) return std::make_shared<CharLiteralNode>(previous().value);
    if (match(TokenType::Identifier)) return std::make_shared<IdentifierNode>(previous().value);

    if (match(TokenType::Symbol, "(")) {
        auto expr = parseExpression();
        consume(TokenType::Symbol, ")", "Expected ')' after grouped expression.");
        return expr;
    }

    throw std::runtime_error("Expected primary expression, got " + peek().toString() + " (type: " + tokenTypeToString(peek().type) + ")");
}


std::shared_ptr<ExpressionNode> Parser::parseBinaryExpression(
    std::function<std::shared_ptr<ExpressionNode>()> parseOperand,
    const std::vector<std::string>& operators) {
    auto left = parseOperand();

    while (true) {
        bool matchedOperator = false;
        for (const auto& opStr : operators) {
            // Operators can be actual operators (==, !=, +, *) or symbols (&&, || if lexed as symbols)
            if (match(TokenType::Operator, opStr) || match(TokenType::Symbol, opStr)) {
                Token opToken = previous(); // The matched operator token
                auto right = parseOperand();
                auto binaryNode = std::make_shared<BinaryExpressionNode>(opToken.value);
                binaryNode->addChild(left);
                binaryNode->addChild(right);
                left = binaryNode; // For left-associativity
                matchedOperator = true;
                break; 
            }
        }
        if (!matchedOperator) break;
    }
    return left;
}


// Token handling utility methods
Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

Token Parser::peek(int offset) const {
    if (isAtEnd() && offset >= 0) return tokens.back(); // usually EOF token
    size_t target_idx = current + offset;
    if (target_idx < tokens.size()) { // target_idx >= 0 since current and offset can be 0
        return tokens[target_idx];
    }
    return tokens.back(); // Return last token (EOF) if out of bounds
}

Token Parser::previous() const {
    if (current == 0) throw std::out_of_range("No previous token at the beginning.");
    return tokens[current - 1];
}

bool Parser::isAtEnd() const {
    return current >= tokens.size() || tokens[current].type == TokenType::EndOfFile;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(TokenType type, const std::string& value) {
    if (check(type, value)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return tokens[current].type == type;
}

bool Parser::check(TokenType type, const std::string& value) const {
    if (isAtEnd()) return false;
    return tokens[current].type == type && tokens[current].value == value;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        return advance();
    }
    throw std::runtime_error(message + " Expected " + tokenTypeToString(type) +
                             ", but got " + peek().toString() +
                             " (type: " + tokenTypeToString(peek().type) + ") at token index " + std::to_string(current) + ".");
}

void Parser::consume(TokenType type, const std::string& value, const std::string& message) {
    if (check(type, value)) {
        advance();
        return;
    }
    throw std::runtime_error(message + " Expected '" + value + "' (type " + tokenTypeToString(type) +
                             "), but got " + peek().toString() +
                             " (type: " + tokenTypeToString(peek().type) + ") at token index " + std::to_string(current) + ".");
}

void Parser::synchronize() {
    advance(); // Consume the erroneous token

    while (!isAtEnd()) {
        if (previous().type == TokenType::Symbol && previous().value == ";") return; // End of a statement

        // Synchronize to the beginning of the next likely statement or declaration
        switch (peek().type) {
            case TokenType::Keyword:
                if (peek().value == "if" || peek().value == "while" || peek().value == "for" ||
                    peek().value == "return" || peek().value == "break" || peek().value == "continue" ||
                    peek().value == "int" || peek().value == "char" || peek().value == "bool" ||
                    peek().value == "string" || peek().value == "void") {
                    return;
                }
                break;
            case TokenType::Symbol:
                if (peek().value == "}") return; // End of a block
                break;
            // Potentially add other synchronization points
            default:
                break;
        }
        advance();
    }
}