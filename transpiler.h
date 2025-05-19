#pragma once

#include "Parser.h" // Includes all AST Node definition

using namespace std;

class Transpiler {
public:
    Transpiler();
    string transpile(shared_ptr<ProgramNode> program);

private:
    // Program
    string transpileProgram(shared_ptr<ProgramNode> program);

    // Statements
    string transpileStatement(shared_ptr<StatementNode> stmt, int current_indent_level = 0);
    string transpileAssignmentStatement(shared_ptr<AssignmentStatementNode> stmt);
    string transpileVariableDeclaration(shared_ptr<VariableDeclarationNode> decl);
    string transpileIfStatement(shared_ptr<IfNode> stmt, int current_indent_level);
    string transpileWhileStatement(shared_ptr<WhileNode> stmt, int current_indent_level);
    string transpileForStatement(shared_ptr<ForNode> stmt, int current_indent_level);
    string transpileExpressionStatement(shared_ptr<ExpressionStatementNode> stmt);
    string transpileReturnStatement(shared_ptr<ReturnNode> stmt);
    string transpileBlock(shared_ptr<BlockNode> block, int current_indent_level);
    string transpileFunctionDeclaration(shared_ptr<FunctionDeclarationNode> funcDecl);
    string transpilePrintStatement(shared_ptr<PrintNode> stmt);         // For 'print' keyword
    string transpilePrintfStatement(shared_ptr<PrintfNode> stmt);       // For 'printf'
    string transpileScanfStatement(shared_ptr<ScanfNode> stmt);         // For 'scanf'
    string transpileBreakStatement(shared_ptr<BreakNode> stmt);
    string transpileContinueStatement(shared_ptr<ContinueNode> stmt);


    // Expressions
    string transpileExpression(shared_ptr<ExpressionNode> expr);
    string transpileAssignmentNode(shared_ptr<AssignmentNode> assign); // Used by AssignmentStatement and ForLoop increment
    string transpileBinaryExpression(shared_ptr<BinaryExpressionNode> expr);
    string transpileUnaryExpression(shared_ptr<UnaryExpressionNode> expr);
    string transpileFunctionCallNode(shared_ptr<FunctionCallNode> expr);
    string transpileStringLiteralNode(shared_ptr<StringLiteralNode> expr);
    string transpileCharLiteralNode(shared_ptr<CharLiteralNode> expr);
    string transpileNumberNode(shared_ptr<NumberNode> expr);
    string transpileBooleanNode(shared_ptr<BooleanNode> expr);
    string transpileIdentifierNode(shared_ptr<IdentifierNode> expr);


    // Helper
    string indent(const string& code, int level, bool add_final_newline_if_missing = false);
    int m_current_indent_level; // To manage global indentation if needed (can be tricky)
                                // Simpler approach: pass indent level around. I'll use passed level.
};