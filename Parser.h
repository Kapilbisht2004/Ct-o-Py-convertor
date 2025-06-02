#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <string>
#include "Lexer.h" // Assumed to provide Token, TokenType, and tokenTypeToString
using namespace std;

// Forward declarations
class ExpressionNode;
class StatementNode;
class BlockNode;
// class AssignmentNode;
class PrintfNode; // New
class ScanfNode;  // New
class ArrayDeclarationNode;
class ArraySubscriptNode;

// AST Node Classes
class ASTNode
{
public:
    virtual ~ASTNode() = default;
    void addChild(shared_ptr<ASTNode> child)
    {
        if (child)
        { // Only add non-null children
            children.push_back(child);
        }
    }
    string type_name; // For easy identification/debugging

    const vector<shared_ptr<ASTNode>> &getChildren() const
    {
        return children;
    }

protected:
    vector<shared_ptr<ASTNode>> children;
};

class ExpressionNode : public ASTNode
{
public:
    virtual ~ExpressionNode() = default;
};

class StatementNode : public ASTNode
{
public:
    virtual ~StatementNode() = default;
};

class ProgramNode : public ASTNode
{
public:
    ProgramNode() { type_name = "ProgramNode"; }
    vector<shared_ptr<StatementNode>> getStatements() const
    {
        vector<shared_ptr<StatementNode>> stmts;
        for (const auto &child : children)
        {
            stmts.push_back(dynamic_pointer_cast<StatementNode>(child));
        }
        return stmts;
    }
};

class BlockNode : public StatementNode
{
public:
    BlockNode() { type_name = "BlockNode"; }
    vector<shared_ptr<StatementNode>> getStatements() const
    {
        vector<shared_ptr<StatementNode>> stmts;
        for (const auto &child : children)
        {
            stmts.push_back(dynamic_pointer_cast<StatementNode>(child));
        }
        return stmts;
    }
};

class ExpressionStatementNode : public StatementNode
{
public:
    ExpressionStatementNode(shared_ptr<ExpressionNode> expr)
    {
        type_name = "ExpressionStatementNode";
        if (expr)
            addChild(expr); // Expression is the first child
    }
    shared_ptr<ExpressionNode> getExpression() const
    {
        if (!children.empty())
        {
            return dynamic_pointer_cast<ExpressionNode>(children[0]);
        }
        return nullptr;
    }
};

// Node for C-style printf
class PrintfNode : public StatementNode
{
public:
    PrintfNode() { type_name = "PrintfNode"; }
    // Child 0: format string (should be StringLiteralNode)
    // Subsequent children: arguments (ExpressionNode)
    shared_ptr<ExpressionNode> getFormatStringExpression() const
    { // Changed to ExpressionNode
        if (!children.empty())
        {
            return dynamic_pointer_cast<ExpressionNode>(children[0]);
        }
        return nullptr;
    }
    vector<shared_ptr<ExpressionNode>> getArguments() const
    {
        vector<shared_ptr<ExpressionNode>> args;
        if (children.size() > 1)
        {
            for (size_t i = 1; i < children.size(); ++i)
            {
                args.push_back(dynamic_pointer_cast<ExpressionNode>(children[i]));
            }
        }
        return args;
    }
};

// Node for C-style scanf
class ScanfNode : public StatementNode
{
public:
    ScanfNode() { type_name = "ScanfNode"; }
    // Child 0: format string (should be StringLiteralNode)
    // Subsequent children: arguments (ExpressionNode, often Unary '&' Node)
    shared_ptr<ExpressionNode> getFormatStringExpression() const
    { // Changed to ExpressionNode
        if (!children.empty())
        {
            return dynamic_pointer_cast<ExpressionNode>(children[0]);
        }
        return nullptr;
    }
    vector<shared_ptr<ExpressionNode>> getArguments() const
    {
        vector<shared_ptr<ExpressionNode>> args;
        if (children.size() > 1)
        {
            for (size_t i = 1; i < children.size(); ++i)
            {
                args.push_back(dynamic_pointer_cast<ExpressionNode>(children[i]));
            }
        }
        return args;
    }
};

class IfNode : public StatementNode
{
public:
    IfNode() { type_name = "IfNode"; }
    void setCondition(shared_ptr<ExpressionNode> cond) { condition = cond; }
    void setThenBranch(shared_ptr<StatementNode> thenB) { thenBranch = thenB; }
    void setElseBranch(shared_ptr<StatementNode> elseB) { elseBranch = elseB; }

    shared_ptr<ExpressionNode> getCondition() const { return condition; }
    shared_ptr<StatementNode> getThenBranch() const { return thenBranch; }
    shared_ptr<StatementNode> getElseBranch() const { return elseBranch; }

private:
    shared_ptr<ExpressionNode> condition;
    shared_ptr<StatementNode> thenBranch;
    shared_ptr<StatementNode> elseBranch;
};

class WhileNode : public StatementNode
{
public:
    WhileNode() { type_name = "WhileNode"; }
    void setCondition(shared_ptr<ExpressionNode> cond) { condition = cond; }
    void setBody(shared_ptr<StatementNode> b) { body = b; }

    shared_ptr<ExpressionNode> getCondition() const { return condition; }
    shared_ptr<StatementNode> getBody() const { return body; }

private:
    shared_ptr<ExpressionNode> condition;
    shared_ptr<StatementNode> body;
};

class ForNode : public StatementNode
{
public:
    ForNode() { type_name = "ForNode"; }
    void setInitializer(shared_ptr<StatementNode> init) { initializer = init; } // Can be VarDecl or ExprStmt
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

class ReturnNode : public StatementNode
{
public:
    ReturnNode() { type_name = "ReturnNode"; }
    shared_ptr<ExpressionNode> getReturnValue() const
    {
        if (!children.empty())
        {
            return dynamic_pointer_cast<ExpressionNode>(children[0]);
        }
        return nullptr;
    }
};

class BreakNode : public StatementNode
{
public:
    BreakNode() { type_name = "BreakNode"; }
};

class ContinueNode : public StatementNode
{
public:
    ContinueNode() { type_name = "ContinueNode"; }
};

class DeclarationNode : public StatementNode
{
public:
    DeclarationNode(const string &declName, const string &declType, shared_ptr<ExpressionNode> initExpr = nullptr)
        : name(declName), type(declType), initialValue(initExpr) {}

    const string &getName() const { return name; }
    const string &getDeclaredType() const { return type; }
    shared_ptr<ExpressionNode> getInitialValue() const { return initialValue; } // ✅ New getter

protected:
    string name;
    string type;
    shared_ptr<ExpressionNode> initialValue; // ✅ New member
};

class VariableDeclarationNode : public DeclarationNode
{
public:
    VariableDeclarationNode(const string &varName, const string &varType)
        : DeclarationNode(varName, varType) { type_name = "VariableDeclarationNode"; }
    shared_ptr<ExpressionNode> getInitializer() const
    {
        if (!children.empty())
        {
            return dynamic_pointer_cast<ExpressionNode>(children[0]);
        }
        return nullptr;
    }
};

class FunctionDeclarationNode : public DeclarationNode
{
public:
    FunctionDeclarationNode(const string &funcName, const string &retType)
        : DeclarationNode(funcName, retType) { type_name = "FunctionDeclarationNode"; }
    void addParameter(const string &paramName, const string &paramType)
    {
        paramNames.push_back(paramName);
        paramTypes.push_back(paramType);
    }
    void setBody(shared_ptr<BlockNode> funcBody) { body = funcBody; }

    const vector<string> &getParamNames() const { return paramNames; }
    const vector<string> &getParamTypes() const { return paramTypes; }
    shared_ptr<BlockNode> getBody() const { return body; }

private:
    vector<string> paramNames;
    vector<string> paramTypes;
    shared_ptr<BlockNode> body;
};

class BinaryExpressionNode : public ExpressionNode
{
public:
    BinaryExpressionNode(const string &op) : op_val(op) { type_name = "BinaryExpressionNode"; }
    const string &getOperator() const { return op_val; }
    shared_ptr<ExpressionNode> getLeft() const
    {
        if (children.size() > 0)
            return dynamic_pointer_cast<ExpressionNode>(children[0]);
        return nullptr;
    }
    shared_ptr<ExpressionNode> getRight() const
    {
        if (children.size() > 1)
            return dynamic_pointer_cast<ExpressionNode>(children[1]);
        return nullptr;
    }

private:
    string op_val;
};

class UnaryExpressionNode : public ExpressionNode
{
public:
    UnaryExpressionNode(const string &op) : op_val(op) { type_name = "UnaryExpressionNode"; }
    const string &getOperator() const { return op_val; }
    shared_ptr<ExpressionNode> getOperand() const
    {
        if (!children.empty())
            return dynamic_pointer_cast<ExpressionNode>(children[0]);
        return nullptr;
    }

private:
    string op_val;
};

class IdentifierNode : public ExpressionNode
{
public:
    IdentifierNode(const string &idName) : name(idName) { type_name = "IdentifierNode"; }
    const string &getName() const { return name; }

private:
    string name;
};

// class AssignmentNode : public ExpressionNode
// {
// public:
//     AssignmentNode(const string &targetIdentifierName) : target_name(targetIdentifierName)
//     {
//         type_name = "AssignmentNode";
//     }
//     const string &getTargetName() const { return target_name; }
//     shared_ptr<ExpressionNode> getValue() const
//     {
//         if (!children.empty())
//             return dynamic_pointer_cast<ExpressionNode>(children[0]);
//         return nullptr;
//     }

// private:
//     string target_name;
// };
class AssignmentNode : public ExpressionNode
{
public:
    // Child 0: L-Value (target of assignment, e.g., IdentifierNode, ArraySubscriptNode)
    // Child 1: R-Value (value being assigned)
    AssignmentNode(shared_ptr<ExpressionNode> lval, shared_ptr<ExpressionNode> rval)
    {
        type_name = "AssignmentNode";
        if (lval)
            addChild(lval); // L-Value is child 0
        if (rval)
            addChild(rval); // R-Value is child 1
    }

    shared_ptr<ExpressionNode> getLValue() const
    {
        if (children.size() > 0)
        {
            return dynamic_pointer_cast<ExpressionNode>(children[0]);
        }
        return nullptr;
    }

    shared_ptr<ExpressionNode> getRValue() const
    {
        if (children.size() > 1)
        {
            return dynamic_pointer_cast<ExpressionNode>(children[1]);
        }
        return nullptr;
    }
    // REMOVE old constructor and members:
    // AssignmentNode(const string &targetIdentifierName) : target_name(targetIdentifierName) ...
    // const string &getTargetName() const { return target_name; }
    // private:
    // string target_name;
};
class AssignmentStatementNode : public StatementNode
{
public:
    AssignmentStatementNode(shared_ptr<AssignmentNode> assignExpr)
        : assignment_expr(assignExpr) { type_name = "AssignmentStatementNode"; }
    shared_ptr<AssignmentNode> getAssignment() const { return assignment_expr; }

private:
    shared_ptr<AssignmentNode> assignment_expr;
};

class FunctionCallNode : public ExpressionNode
{
public:
    FunctionCallNode(const string &funcName) : name(funcName) { type_name = "FunctionCallNode"; }
    const string &getFunctionName() const { return name; }
    vector<shared_ptr<ExpressionNode>> getArguments() const
    {
        vector<shared_ptr<ExpressionNode>> args;
        for (const auto &child : children)
        {
            args.push_back(dynamic_pointer_cast<ExpressionNode>(child));
        }
        return args;
    }

private:
    string name;
};

class LiteralNode : public ExpressionNode
{
public:
    virtual ~LiteralNode() = default;
};

class StringLiteralNode : public LiteralNode
{
public:
    StringLiteralNode(const string &val) : value(val) { type_name = "StringLiteralNode"; }
    const string &getValue() const { return value; }

private:
    string value;
};

class CharLiteralNode : public LiteralNode
{
public:
    CharLiteralNode(const string &val) : value(val) { type_name = "CharLiteralNode"; } // stores char as string of length 1
    const string &getValue() const { return value; }

private:
    string value;
};

class NumberNode : public LiteralNode
{
public:
    NumberNode(const string &val) : value(val) { type_name = "NumberNode"; }
    const string &getValue() const { return value; }

private:
    string value;
};

class BooleanNode : public LiteralNode
{
public:
    BooleanNode(bool val) : value(val) { type_name = "BooleanNode"; }
    bool getValue() const { return value; }

private:
    bool value;
};

class ArrayDeclarationNode : public VariableDeclarationNode
{ // Or public DeclarationNode
public:
    // Constructor: base type, name, and size expression
    ArrayDeclarationNode(const string &varName, const string &varType, shared_ptr<ExpressionNode> sizeExpr)
        : VariableDeclarationNode(varName, varType), size_expr(sizeExpr)
    {
        type_name = "ArrayDeclarationNode";
        // Optionally, add size_expr to children as well if your traversal relies on it
        // if (size_expr) addChild(size_expr); // Not strictly necessary if you have a getter
    }

    shared_ptr<ExpressionNode> getSizeExpression() const
    {
        return size_expr;
    }
    // If you plan to support C-style initializers like int arr[3] = {1,2,3};
    // you'd add members and methods to store/access these initializer expressions.
    // For now, we'll skip direct initializers in the declaration for simplicity.

private:
    shared_ptr<ExpressionNode> size_expr;
    // vector<shared_ptr<ExpressionNode>> initializers; // For later
};

class ArraySubscriptNode : public ExpressionNode
{
public:
    // Child 0: array expression (e.g., identifier)
    // Child 1: index expression
    ArraySubscriptNode(shared_ptr<ExpressionNode> arrExpr, shared_ptr<ExpressionNode> idxExpr)
    {
        type_name = "ArraySubscriptNode";
        if (arrExpr)
            addChild(arrExpr); // Store array expr as first child
        if (idxExpr)
            addChild(idxExpr); // Store index expr as second child
    }

    shared_ptr<ExpressionNode> getArrayExpression() const
    {
        if (children.size() > 0)
        {
            return dynamic_pointer_cast<ExpressionNode>(children[0]);
        }
        return nullptr;
    }

    shared_ptr<ExpressionNode> getIndexExpression() const
    {
        if (children.size() > 1)
        {
            return dynamic_pointer_cast<ExpressionNode>(children[1]);
        }
        return nullptr;
    }
};

// Parser class
class Parser
{
public:
    Parser(const vector<Token> &tokens);
    shared_ptr<ProgramNode> parse();
    shared_ptr<ExpressionNode> parseExpression();

private:
    vector<Token> tokens;
    size_t current;

    static string unescapeLiteralContent(const string &s);

    // Parsing methods for program structure
    shared_ptr<ProgramNode> parseProgram();
    shared_ptr<StatementNode> parseStatement();
    shared_ptr<ExpressionStatementNode> parseExpressionStatement();
    shared_ptr<BlockNode> parseBlock();
    shared_ptr<IfNode> parseIf();
    shared_ptr<WhileNode> parseWhile();
    shared_ptr<ForNode> parseFor();
    shared_ptr<StatementNode> parseForLoopInitializer(); // New for robust for-loop parsing
    shared_ptr<ReturnNode> parseReturn();
    shared_ptr<BreakNode> parseBreak();
    shared_ptr<ContinueNode> parseContinue();
    shared_ptr<PrintfNode> parsePrintfStatement(); // New
    shared_ptr<ScanfNode> parseScanfStatement();   // New
    shared_ptr<StatementNode> parseDeclaration();
    shared_ptr<VariableDeclarationNode> parseVariableDeclaration(const string &typeHint = "", const string &identifierHint = "");
    shared_ptr<FunctionDeclarationNode> parseFunctionDeclaration(const string &returnType, const string &identifier);
    shared_ptr<AssignmentStatementNode> parseAssignmentStatement();

    // Expression parsing methods
    shared_ptr<ExpressionNode> parseAssignmentExpression();
    shared_ptr<ExpressionNode> parseLogicalOr();
    shared_ptr<ExpressionNode> parseLogicalAnd();
    shared_ptr<ExpressionNode> parseEquality();
    shared_ptr<ExpressionNode> parseComparison();
    shared_ptr<ExpressionNode> parseTerm();
    shared_ptr<ExpressionNode> parseFactor();
    shared_ptr<ExpressionNode> parseUnary();
    shared_ptr<ExpressionNode> parseCall();
    shared_ptr<ExpressionNode> parsePrimary();

    shared_ptr<ExpressionNode> parseBinaryExpression(
        function<shared_ptr<ExpressionNode>()> parseSubExpr,
        const vector<string> &operators);

    // Token handling utility methods
    Token advance();
    Token peek(int offset = 0) const;
    Token previous() const;
    bool isAtEnd() const;
    bool match(TokenType type);
    bool match(TokenType type, const string &value);
    bool check(TokenType type) const;
    bool check(TokenType type, const string &value) const;
    Token consume(TokenType type, const string &message);
    void consume(TokenType type, const string &value, const string &message);
    void synchronize();
};