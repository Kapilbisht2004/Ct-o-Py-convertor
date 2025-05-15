#pragma once

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include "Lexer.h" // Assumed to provide Token, TokenType, and tokenTypeToString
using namespace std;

// Forward declarations
class ExpressionNode;
class StatementNode;
class BlockNode;
class AssignmentNode;


// AST Node Classes
class ASTNode {
public:
    virtual ~ASTNode() = default;
    void addChild(shared_ptr<ASTNode> child) {
        if (child) { // Only add non-null children
            children.push_back(child);
        }
    }
    string type_name; // For easy identification/debugging

    const vector<shared_ptr<ASTNode>>& getChildren() const {
        return children;
    }

protected:
    vector<shared_ptr<ASTNode>> children;
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
    // Statements are accessed via getChildren(), which will be shared_ptr<StatementNode>
    vector<shared_ptr<StatementNode>> getStatements() const {
        vector<shared_ptr<StatementNode>> stmts;
        for (const auto& child : children) {
            stmts.push_back(dynamic_pointer_cast<StatementNode>(child));
        }
        return stmts;
    }
};

class BlockNode : public StatementNode {
public:
    BlockNode() { type_name = "BlockNode"; }
    virtual ~BlockNode() = default;
    // Statements are accessed via getChildren(), similar to ProgramNode
    vector<shared_ptr<StatementNode>> getStatements() const {
        vector<shared_ptr<StatementNode>> stmts;
        for (const auto& child : children) {
            stmts.push_back(dynamic_pointer_cast<StatementNode>(child));
        }
        return stmts;
    }
};

class ExpressionStatementNode : public StatementNode {
public:
    ExpressionStatementNode(shared_ptr<ExpressionNode> expr) {
        type_name = "ExpressionStatementNode";
        if (expr) addChild(expr);
    }
    virtual ~ExpressionStatementNode() = default;
    shared_ptr<ExpressionNode> getExpression() const {
        if (!children.empty()) {
            return dynamic_pointer_cast<ExpressionNode>(children[0]);
        }
        return nullptr;
    }
};

class IfNode : public StatementNode {
public:
    IfNode() { type_name = "IfNode"; }
    virtual ~IfNode() = default;

    void setCondition(shared_ptr<ExpressionNode> cond) { condition = cond; }
    void setThenBranch(shared_ptr<StatementNode> thenB) { thenBranch = thenB; }
    void setElseBranch(shared_ptr<StatementNode> elseB) { elseBranch = elseB; }

    shared_ptr<ExpressionNode> getCondition() const { return condition; }
    shared_ptr<StatementNode> getThenBranch() const { return thenBranch; }
    shared_ptr<StatementNode> getElseBranch() const { return elseBranch; } // Can be nullptr

private:
    shared_ptr<ExpressionNode> condition;
    shared_ptr<StatementNode> thenBranch;
    shared_ptr<StatementNode> elseBranch;
};

class WhileNode : public StatementNode {
public:
    WhileNode() { type_name = "WhileNode"; }
    virtual ~WhileNode() = default;

    void setCondition(shared_ptr<ExpressionNode> cond) { condition = cond; }
    void setBody(shared_ptr<StatementNode> b) { body = b; }

    shared_ptr<ExpressionNode> getCondition() const { return condition; }
    shared_ptr<StatementNode> getBody() const { return body; }

private:
    shared_ptr<ExpressionNode> condition;
    shared_ptr<StatementNode> body;
};

class ForNode : public StatementNode {
public:
    ForNode() { type_name = "ForNode"; }
    virtual ~ForNode() = default;

    void setInitializer(shared_ptr<StatementNode> init) { initializer = init; }
    void setCondition(shared_ptr<ExpressionNode> cond) { condition = cond; }
    void setIncrement(shared_ptr<ExpressionNode> incr) { increment = incr; }
    void setBody(shared_ptr<StatementNode> b) { body = b; }

    shared_ptr<StatementNode> getInitializer() const { return initializer; }
    shared_ptr<ExpressionNode> getCondition() const { return condition; }
    shared_ptr<ExpressionNode> getIncrement() const { return increment; }
    shared_ptr<StatementNode> getBody() const { return body; }

private:
    shared_ptr<StatementNode> initializer;
    shared_ptr<ExpressionNode> condition;
    shared_ptr<ExpressionNode> increment;
    shared_ptr<StatementNode> body;
};

class ReturnNode : public StatementNode {
public:
    ReturnNode() { type_name = "ReturnNode"; }
    virtual ~ReturnNode() = default;
    shared_ptr<ExpressionNode> getReturnValue() const {
        if (!children.empty()) {
            return dynamic_pointer_cast<ExpressionNode>(children[0]);
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
    DeclarationNode(const string& declName, const string& declType)
        : name(declName), type(declType) {}
    virtual ~DeclarationNode() = default;

    const string& getName() const { return name; }
    const string& getDeclaredType() const { return type; } // Type of var or return type of func

protected:
    string name;
    string type;
};

class VariableDeclarationNode : public DeclarationNode {
public:
    VariableDeclarationNode(const string& varName, const string& varType)
        : DeclarationNode(varName, varType) { type_name = "VariableDeclarationNode"; }
    virtual ~VariableDeclarationNode() = default;

    shared_ptr<ExpressionNode> getInitializer() const {
        if (!children.empty()) {
            return dynamic_pointer_cast<ExpressionNode>(children[0]);
        }
        return nullptr;
    }
};

class FunctionDeclarationNode : public DeclarationNode {
public:
    FunctionDeclarationNode(const string& funcName, const string& retType)
        : DeclarationNode(funcName, retType) { type_name = "FunctionDeclarationNode"; }
    virtual ~FunctionDeclarationNode() = default;

    void addParameter(const string& paramName, const string& paramType) {
        paramNames.push_back(paramName);
        paramTypes.push_back(paramType);
    }
    void setBody(shared_ptr<BlockNode> funcBody) { body = funcBody; }

    const vector<string>& getParamNames() const { return paramNames; }
    const vector<string>& getParamTypes() const { return paramTypes; }
    shared_ptr<BlockNode> getBody() const { return body; }

private:
    vector<string> paramNames;
    vector<string> paramTypes;
    shared_ptr<BlockNode> body; // Can be nullptr for forward declarations
};

class BinaryExpressionNode : public ExpressionNode {
public:
    BinaryExpressionNode(const string& op) : op_val(op) { type_name = "BinaryExpressionNode"; }
    virtual ~BinaryExpressionNode() = default;

    const string& getOperator() const { return op_val; }
    shared_ptr<ExpressionNode> getLeft() const {
        if (children.size() > 0) return dynamic_pointer_cast<ExpressionNode>(children[0]);
        return nullptr;
    }
    shared_ptr<ExpressionNode> getRight() const {
        if (children.size() > 1) return dynamic_pointer_cast<ExpressionNode>(children[1]);
        return nullptr;
    }
private:
    string op_val;
};

class UnaryExpressionNode : public ExpressionNode {
public:
    UnaryExpressionNode(const string& op) : op_val(op) { type_name = "UnaryExpressionNode"; }
    virtual ~UnaryExpressionNode() = default;

    const string& getOperator() const { return op_val; }
    shared_ptr<ExpressionNode> getOperand() const {
        if (!children.empty()) return dynamic_pointer_cast<ExpressionNode>(children[0]);
        return nullptr;
    }
private:
    string op_val;
};

class IdentifierNode : public ExpressionNode {
public:
    IdentifierNode(const string& idName) : name(idName) { type_name = "IdentifierNode"; }
    virtual ~IdentifierNode() = default;
    const string& getName() const { return name; }
private:
    string name;
};

class AssignmentNode : public ExpressionNode { // Represents the "target = value" part
public:
    AssignmentNode(const string& targetIdentifierName) : target_name(targetIdentifierName) {
        type_name = "AssignmentNode";
    }
    virtual ~AssignmentNode() = default;

    const string& getTargetName() const { return target_name; }
    shared_ptr<ExpressionNode> getValue() const { // Value being assigned is the first child
        if (!children.empty()) return dynamic_pointer_cast<ExpressionNode>(children[0]);
        return nullptr;
    }
private:
    string target_name;
};

class AssignmentStatementNode : public StatementNode { // Wraps an AssignmentNode to be a statement
public:
    AssignmentStatementNode(shared_ptr<AssignmentNode> assignExpr)
        : assignment_expr(assignExpr) { type_name = "AssignmentStatementNode"; }
    virtual ~AssignmentStatementNode() = default;
    shared_ptr<AssignmentNode> getAssignment() const { return assignment_expr; }
private:
    shared_ptr<AssignmentNode> assignment_expr;
};

class FunctionCallNode : public ExpressionNode {
public:
    FunctionCallNode(const string& funcName) : name(funcName) { type_name = "FunctionCallNode"; }
    virtual ~FunctionCallNode() = default;

    const string& getFunctionName() const { return name; }
    vector<shared_ptr<ExpressionNode>> getArguments() const {
        vector<shared_ptr<ExpressionNode>> args;
        for (const auto& child : children) {
            args.push_back(dynamic_pointer_cast<ExpressionNode>(child));
        }
        return args;
    }
private:
    string name;
};

class LiteralNode : public ExpressionNode { // Base for literals
public:
    virtual ~LiteralNode() = default;
};

class StringLiteralNode : public LiteralNode {
public:
    StringLiteralNode(const string& val) : value(val) { type_name = "StringLiteralNode"; }
    const string& getValue() const { return value; }
private:
    string value;
};

class CharLiteralNode : public LiteralNode {
public:
    CharLiteralNode(const string& val) : value(val) { type_name = "CharLiteralNode"; }
    const string& getValue() const { return value; } // Stores char as string for simplicity
private:
    string value;
};

class NumberNode : public LiteralNode {
public:
    NumberNode(const string& val) : value(val) { type_name = "NumberNode"; }
    const string& getValue() const { return value; } // Stores number as string
private:
    string value;
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
    Parser(const vector<Token>& tokens);
    shared_ptr<ProgramNode> parse(); // Changed return type to ProgramNode

private:
    vector<Token> tokens;
    size_t current;

    // Parsing methods for program structure
    shared_ptr<ProgramNode> parseProgram();
    shared_ptr<StatementNode> parseStatement();
    shared_ptr<ExpressionStatementNode> parseExpressionStatement();
    shared_ptr<BlockNode> parseBlock();
    shared_ptr<IfNode> parseIf();
    shared_ptr<WhileNode> parseWhile();
    shared_ptr<ForNode> parseFor();
    shared_ptr<ReturnNode> parseReturn();
    shared_ptr<BreakNode> parseBreak();
    shared_ptr<ContinueNode> parseContinue();
    shared_ptr<StatementNode> parseDeclaration(); // Can return VarDecl or FuncDecl
    shared_ptr<VariableDeclarationNode> parseVariableDeclaration(const string& typeHint = "", const string& identifierHint = "");
    shared_ptr<FunctionDeclarationNode> parseFunctionDeclaration(const string& returnType, const string& identifier);
    shared_ptr<AssignmentStatementNode> parseAssignmentStatement();

    // Expression parsing methods
    shared_ptr<ExpressionNode> parseExpression();
    shared_ptr<ExpressionNode> parseAssignmentExpression();
    shared_ptr<ExpressionNode> parseLogicalOr();
    shared_ptr<ExpressionNode> parseLogicalAnd();
    shared_ptr<ExpressionNode> parseEquality();
    shared_ptr<ExpressionNode> parseComparison();
    shared_ptr<ExpressionNode> parseTerm();
    shared_ptr<ExpressionNode> parseFactor();
    shared_ptr<ExpressionNode> parseUnary();
    shared_ptr<ExpressionNode> parseCall();     // Also handles primary before call
    shared_ptr<ExpressionNode> parsePrimary();

    shared_ptr<ExpressionNode> parseBinaryExpression(
        function<shared_ptr<ExpressionNode>()> parseSubExpr,
        const vector<string>& operators);

    // Token handling utility methods
    Token advance();
    Token peek(int offset = 0) const;
    Token previous() const;
    bool isAtEnd() const;
    bool match(TokenType type);
    bool match(TokenType type, const string& value);
    bool check(TokenType type) const;
    bool check(TokenType type, const string& value) const;
    Token consume(TokenType type, const string& message);
    void consume(TokenType type, const string& value, const string& message);
    void synchronize();
};