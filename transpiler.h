#pragma once

#include "Parser.h"
using namespace std;

class Transpiler {
public:
    Transpiler();
    string transpile(shared_ptr<ProgramNode> program);

private:
    // Program
    string transpileProgram(shared_ptr<ProgramNode> program);

    // Statements
    string transpileStatement(shared_ptr<StatementNode> stmt);
    string transpileAssignment(shared_ptr<AssignmentStatementNode> stmt);
    string transpileVariableDeclaration(shared_ptr<VariableDeclarationNode> decl);
    string transpileIfStatement(shared_ptr<IfNode> stmt);
    string transpileWhileStatement(shared_ptr<WhileNode> stmt);
    string transpileForStatement(shared_ptr<ForNode> stmt);
    string transpileAssignmentNode(shared_ptr<AssignmentNode> assign);
    string transpileExpressionStatement(shared_ptr<ExpressionStatementNode> stmt);
    string transpileReturnStatement(shared_ptr<ReturnNode> stmt);
    string transpileBlock(shared_ptr<BlockNode> block);
    string transpileFunctionDeclaration(shared_ptr<FunctionDeclarationNode> funcDecl);

    // Expressions
    string transpileExpression(shared_ptr<ExpressionNode> expr);
    string transpileBinaryExpression(shared_ptr<BinaryExpressionNode> expr);
    string transpileUnaryExpression(shared_ptr<UnaryExpressionNode> expr);

    // Helper
    string indent(const string& code, int level );
};