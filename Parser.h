#pragma once

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include "Lexer.h" // Assumed to provide Token, TokenType, and tokenTypeToString

// Forward declarations
class ExpressionNode;
class StatementNode;
class BlockNode;
class AssignmentNode;

// AST Node Classes
class ASTNode {
public:
    virtual ~ASTNode() = default;
    void addChild(std::shared_ptr<ASTNode> child) {
        if (child) { // Only add non-null children
            children.push_back(child);
        }
    }
    std::string type_name; // For easy identification/debugging

    const std::vector<std::shared_ptr<ASTNode>>& getChildren() const {
        return children;
    }

protected:
    std::vector<std::shared_ptr<ASTNode>> children;
};

class ExpressionNode : public ASTNode {
public:
    virtual ~ExpressionNode() = default;
};

class StatementNode : public ASTNode {
public:
    virtual ~StatementNode() = default;
};

class ProgramNode : public ASTNode {
public:
    ProgramNode() { type_name = "ProgramNode"; }
    virtual ~ProgramNode() = default;
    // Statements are accessed via getChildren(), which will be std::shared_ptr<StatementNode>
    std::vector<std::shared_ptr<StatementNode>> getStatements() const {
        std::vector<std::shared_ptr<StatementNode>> stmts;
        for (const auto& child : children) {
            stmts.push_back(std::dynamic_pointer_cast<StatementNode>(child));
        }
        return stmts;
    }
};

class BlockNode : public StatementNode {
public:
    BlockNode() { type_name = "BlockNode"; }
    virtual ~BlockNode() = default;
    // Statements are accessed via getChildren(), similar to ProgramNode
    std::vector<std::shared_ptr<StatementNode>> getStatements() const {
        std::vector<std::shared_ptr<StatementNode>> stmts;
        for (const auto& child : children) {
            stmts.push_back(std::dynamic_pointer_cast<StatementNode>(child));
        }
        return stmts;
    }
};

class ExpressionStatementNode : public StatementNode {
public:
    ExpressionStatementNode(std::shared_ptr<ExpressionNode> expr) {
        type_name = "ExpressionStatementNode";
        if (expr) addChild(expr);
    }
    virtual ~ExpressionStatementNode() = default;
    std::shared_ptr<ExpressionNode> getExpression() const {
        if (!children.empty()) {
            return std::dynamic_pointer_cast<ExpressionNode>(children[0]);
        }
        return nullptr;
    }
};

class IfNode : public StatementNode {
public:
    IfNode() { type_name = "IfNode"; }
    virtual ~IfNode() = default;

    void setCondition(std::shared_ptr<ExpressionNode> cond) { condition = cond; }
    void setThenBranch(std::shared_ptr<StatementNode> thenB) { thenBranch = thenB; }
    void setElseBranch(std::shared_ptr<StatementNode> elseB) { elseBranch = elseB; }

    std::shared_ptr<ExpressionNode> getCondition() const { return condition; }
    std::shared_ptr<StatementNode> getThenBranch() const { return thenBranch; }
    std::shared_ptr<StatementNode> getElseBranch() const { return elseBranch; } // Can be nullptr

private:
    std::shared_ptr<ExpressionNode> condition;
    std::shared_ptr<StatementNode> thenBranch;
    std::shared_ptr<StatementNode> elseBranch;
};

class WhileNode : public StatementNode {
public:
    WhileNode() { type_name = "WhileNode"; }
    virtual ~WhileNode() = default;

    void setCondition(std::shared_ptr<ExpressionNode> cond) { condition = cond; }
    void setBody(std::shared_ptr<StatementNode> b) { body = b; }

    std::shared_ptr<ExpressionNode> getCondition() const { return condition; }
    std::shared_ptr<StatementNode> getBody() const { return body; }

private:
    std::shared_ptr<ExpressionNode> condition;
    std::shared_ptr<StatementNode> body;
};

class ForNode : public StatementNode {
public:
    ForNode() { type_name = "ForNode"; }
    virtual ~ForNode() = default;

    void setInitializer(std::shared_ptr<StatementNode> init) { initializer = init; }
    void setCondition(std::shared_ptr<ExpressionNode> cond) { condition = cond; }
    void setIncrement(std::shared_ptr<ExpressionNode> incr) { increment = incr; }
    void setBody(std::shared_ptr<StatementNode> b) { body = b; }

    std::shared_ptr<StatementNode> getInitializer() const { return initializer; }
    std::shared_ptr<ExpressionNode> getCondition() const { return condition; }
    std::shared_ptr<ExpressionNode> getIncrement() const { return increment; }
    std::shared_ptr<StatementNode> getBody() const { return body; }

private:
    std::shared_ptr<StatementNode> initializer;
    std::shared_ptr<ExpressionNode> condition;
    std::shared_ptr<ExpressionNode> increment;
    std::shared_ptr<StatementNode> body;
};

class ReturnNode : public StatementNode {
public:
    ReturnNode() { type_name = "ReturnNode"; }
    virtual ~ReturnNode() = default;
    std::shared_ptr<ExpressionNode> getReturnValue() const {
        if (!children.empty()) {
            return std::dynamic_pointer_cast<ExpressionNode>(children[0]);
        }
        return nullptr;
    }
};

class BreakNode : public StatementNode {
public:
    BreakNode() { type_name = "BreakNode"; }
    virtual ~BreakNode() = default;
};

class ContinueNode : public StatementNode {
public:
    ContinueNode() { type_name = "ContinueNode"; }
    virtual ~ContinueNode() = default;
};

class DeclarationNode : public StatementNode {
public:
    DeclarationNode(const std::string& declName, const std::string& declType)
        : name(declName), type(declType) {}
    virtual ~DeclarationNode() = default;

    const std::string& getName() const { return name; }
    const std::string& getDeclaredType() const { return type; } // Type of var or return type of func

protected:
    std::string name;
    std::string type;
};

class VariableDeclarationNode : public DeclarationNode {
public:
    VariableDeclarationNode(const std::string& varName, const std::string& varType)
        : DeclarationNode(varName, varType) { type_name = "VariableDeclarationNode"; }
    virtual ~VariableDeclarationNode() = default;

    std::shared_ptr<ExpressionNode> getInitializer() const {
        if (!children.empty()) {
            return std::dynamic_pointer_cast<ExpressionNode>(children[0]);
        }
        return nullptr;
    }
};

class FunctionDeclarationNode : public DeclarationNode {
public:
    FunctionDeclarationNode(const std::string& funcName, const std::string& retType)
        : DeclarationNode(funcName, retType) { type_name = "FunctionDeclarationNode"; }
    virtual ~FunctionDeclarationNode() = default;

    void addParameter(const std::string& paramName, const std::string& paramType) {
        paramNames.push_back(paramName);
        paramTypes.push_back(paramType);
    }
    void setBody(std::shared_ptr<BlockNode> funcBody) { body = funcBody; }

    const std::vector<std::string>& getParamNames() const { return paramNames; }
    const std::vector<std::string>& getParamTypes() const { return paramTypes; }
    std::shared_ptr<BlockNode> getBody() const { return body; }

private:
    std::vector<std::string> paramNames;
    std::vector<std::string> paramTypes;
    std::shared_ptr<BlockNode> body; // Can be nullptr for forward declarations
};

class BinaryExpressionNode : public ExpressionNode {
public:
    BinaryExpressionNode(const std::string& op) : op_val(op) { type_name = "BinaryExpressionNode"; }
    virtual ~BinaryExpressionNode() = default;

    const std::string& getOperator() const { return op_val; }
    std::shared_ptr<ExpressionNode> getLeft() const {
        if (children.size() > 0) return std::dynamic_pointer_cast<ExpressionNode>(children[0]);
        return nullptr;
    }
    std::shared_ptr<ExpressionNode> getRight() const {
        if (children.size() > 1) return std::dynamic_pointer_cast<ExpressionNode>(children[1]);
        return nullptr;
    }
private:
    std::string op_val;
};

class UnaryExpressionNode : public ExpressionNode {
public:
    UnaryExpressionNode(const std::string& op) : op_val(op) { type_name = "UnaryExpressionNode"; }
    virtual ~UnaryExpressionNode() = default;

    const std::string& getOperator() const { return op_val; }
    std::shared_ptr<ExpressionNode> getOperand() const {
        if (!children.empty()) return std::dynamic_pointer_cast<ExpressionNode>(children[0]);
        return nullptr;
    }
private:
    std::string op_val;
};

class IdentifierNode : public ExpressionNode {
public:
    IdentifierNode(const std::string& idName) : name(idName) { type_name = "IdentifierNode"; }
    virtual ~IdentifierNode() = default;
    const std::string& getName() const { return name; }
private:
    std::string name;
};

class AssignmentNode : public ExpressionNode { // Represents the "target = value" part
public:
    AssignmentNode(const std::string& targetIdentifierName) : target_name(targetIdentifierName) {
        type_name = "AssignmentNode";
    }
    virtual ~AssignmentNode() = default;

    const std::string& getTargetName() const { return target_name; }
    std::shared_ptr<ExpressionNode> getValue() const { // Value being assigned is the first child
        if (!children.empty()) return std::dynamic_pointer_cast<ExpressionNode>(children[0]);
        return nullptr;
    }
private:
    std::string target_name;
};

class AssignmentStatementNode : public StatementNode { // Wraps an AssignmentNode to be a statement
public:
    AssignmentStatementNode(std::shared_ptr<AssignmentNode> assignExpr)
        : assignment_expr(assignExpr) { type_name = "AssignmentStatementNode"; }
    virtual ~AssignmentStatementNode() = default;
    std::shared_ptr<AssignmentNode> getAssignment() const { return assignment_expr; }
private:
    std::shared_ptr<AssignmentNode> assignment_expr;
};

class FunctionCallNode : public ExpressionNode {
public:
    FunctionCallNode(const std::string& funcName) : name(funcName) { type_name = "FunctionCallNode"; }
    virtual ~FunctionCallNode() = default;

    const std::string& getFunctionName() const { return name; }
    std::vector<std::shared_ptr<ExpressionNode>> getArguments() const {
        std::vector<std::shared_ptr<ExpressionNode>> args;
        for (const auto& child : children) {
            args.push_back(std::dynamic_pointer_cast<ExpressionNode>(child));
        }
        return args;
    }
private:
    std::string name;
};

class LiteralNode : public ExpressionNode { // Base for literals
public:
    virtual ~LiteralNode() = default;
};

class StringLiteralNode : public LiteralNode {
public:
    StringLiteralNode(const std::string& val) : value(val) { type_name = "StringLiteralNode"; }
    const std::string& getValue() const { return value; }
private:
    std::string value;
};

class CharLiteralNode : public LiteralNode {
public:
    CharLiteralNode(const std::string& val) : value(val) { type_name = "CharLiteralNode"; }
    const std::string& getValue() const { return value; } // Stores char as string for simplicity
private:
    std::string value;
};

class NumberNode : public LiteralNode {
public:
    NumberNode(const std::string& val) : value(val) { type_name = "NumberNode"; }
    const std::string& getValue() const { return value; } // Stores number as string
private:
    std::string value;
};

class BooleanNode : public LiteralNode {
public:
    BooleanNode(bool val) : value(val) { type_name = "BooleanNode"; }
    bool getValue() const { return value; }
private:
    bool value;
};


// Parser class
class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    std::shared_ptr<ProgramNode> parse(); // Changed return type to ProgramNode

private:
    std::vector<Token> tokens;
    size_t current;

    // Parsing methods for program structure
    std::shared_ptr<ProgramNode> parseProgram();
    std::shared_ptr<StatementNode> parseStatement();
    std::shared_ptr<ExpressionStatementNode> parseExpressionStatement();
    std::shared_ptr<BlockNode> parseBlock();
    std::shared_ptr<IfNode> parseIf();
    std::shared_ptr<WhileNode> parseWhile();
    std::shared_ptr<ForNode> parseFor();
    std::shared_ptr<ReturnNode> parseReturn();
    std::shared_ptr<BreakNode> parseBreak();
    std::shared_ptr<ContinueNode> parseContinue();
    std::shared_ptr<StatementNode> parseDeclaration(); // Can return VarDecl or FuncDecl
    std::shared_ptr<VariableDeclarationNode> parseVariableDeclaration(const std::string& typeHint = "", const std::string& identifierHint = "");
    std::shared_ptr<FunctionDeclarationNode> parseFunctionDeclaration(const std::string& returnType, const std::string& identifier);
    std::shared_ptr<AssignmentStatementNode> parseAssignmentStatement();

    // Expression parsing methods
    std::shared_ptr<ExpressionNode> parseExpression();
    std::shared_ptr<ExpressionNode> parseAssignmentExpression();
    std::shared_ptr<ExpressionNode> parseLogicalOr();
    std::shared_ptr<ExpressionNode> parseLogicalAnd();
    std::shared_ptr<ExpressionNode> parseEquality();
    std::shared_ptr<ExpressionNode> parseComparison();
    std::shared_ptr<ExpressionNode> parseTerm();
    std::shared_ptr<ExpressionNode> parseFactor();
    std::shared_ptr<ExpressionNode> parseUnary();
    std::shared_ptr<ExpressionNode> parseCall();     // Also handles primary before call
    std::shared_ptr<ExpressionNode> parsePrimary();

    std::shared_ptr<ExpressionNode> parseBinaryExpression(
        std::function<std::shared_ptr<ExpressionNode>()> parseSubExpr,
        const std::vector<std::string>& operators);

    // Token handling utility methods
    Token advance();
    Token peek(int offset = 0) const;
    Token previous() const;
    bool isAtEnd() const;
    bool match(TokenType type);
    bool match(TokenType type, const std::string& value);
    bool check(TokenType type) const;
    bool check(TokenType type, const std::string& value) const;
    Token consume(TokenType type, const std::string& message);
    void consume(TokenType type, const std::string& value, const std::string& message);
    void synchronize();
};