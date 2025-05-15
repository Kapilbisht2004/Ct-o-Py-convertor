#ifndef TRANSPILER_H
#define TRANSPILER_H

#include <string>
#include <memory>

// Forward declarations
class ProgramNode;
class StatementNode;
class AssignmentStatementNode;
class VariableDeclarationNode;
class IfNode;
class WhileNode;
class ExpressionNode;
class BinaryExpressionNode;
class UnaryExpressionNode;
class BlockNode;
class ForNode;
class AssignmentNode;
class ExpressionStatementNode;
class ReturnNode;
class IdentifierNode;
class NumberNode;
class FunctionCallNode;
class FunctionDeclarationNode;

class Transpiler {
public:
    Transpiler();

    std::string transpile(std::shared_ptr<ProgramNode> program);

private:
    // Program
    std::string transpileProgram(std::shared_ptr<ProgramNode> program);

    // Statements
    std::string transpileStatement(std::shared_ptr<StatementNode> stmt);
    std::string transpileAssignment(std::shared_ptr<AssignmentStatementNode> stmt);
    std::string transpileVariableDeclaration(std::shared_ptr<VariableDeclarationNode> decl);
    std::string transpileIfStatement(std::shared_ptr<IfNode> stmt);
    std::string transpileWhileStatement(std::shared_ptr<WhileNode> stmt);
    std::string transpileForStatement(std::shared_ptr<ForNode> stmt);
    std::string transpileAssignmentNode(std::shared_ptr<AssignmentNode> assign);
    std::string transpileExpressionStatement(std::shared_ptr<ExpressionStatementNode> stmt);
    std::string transpileReturnStatement(std::shared_ptr<ReturnNode> stmt);
    std::string transpileBlock(std::shared_ptr<BlockNode> block);
    std::string transpileFunctionDeclaration(std::shared_ptr<FunctionDeclarationNode> funcDecl);

    // Expressions
    std::string transpileExpression(std::shared_ptr<ExpressionNode> expr);
    std::string transpileBinaryExpression(std::shared_ptr<BinaryExpressionNode> expr);
    std::string transpileUnaryExpression(std::shared_ptr<UnaryExpressionNode> expr);

    // Helper
    std::string indent(const std::string& code, int level );
};

#endif // TRANSPILER_HS
