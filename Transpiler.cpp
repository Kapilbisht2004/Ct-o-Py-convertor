
#include "Transpiler.h"
#include <sstream>
#include <typeinfo>
using namespace std;

Transpiler::Transpiler(shared_ptr<ProgramNode> root) : astRoot(move(root)) {}

string Transpiler::transpile() {
    return generateCode(astRoot);
}

string Transpiler::indent(int level) {
    return string(level * indentStr.size(), ' ');
}


string Transpiler::generateCode(const shared_ptr<ASTNode>& node, int indentLevel) {
    if (!node) return "";
    ostringstream out;

    const string& type = node->type_name;

    if (type == "ProgramNode" || type == "BlockNode") {
        for (const auto& stmt : node->getChildren()) {
            out << generateCode(stmt, indentLevel);
        }
    } else if (type == "VariableDeclarationNode") {
        auto var = dynamic_pointer_cast<VariableDeclarationNode>(node);
        out << indent(indentLevel) << var->getName();
        if (var->getInitializer()) {
            out << " = " << generateCode(var->getInitializer(), 0);
        }
        out << "\n";
    } else if (type == "AssignmentStatementNode") {
        auto assign = dynamic_pointer_cast<AssignmentStatementNode>(node);
        out << indent(indentLevel) << generateCode(assign->getAssignment(), 0) << "\n";
    } else if (type == "AssignmentNode") {
        auto a = dynamic_pointer_cast<AssignmentNode>(node);
        out << a->getTargetName() << " = " << generateCode(a->getValue(), 0);
    } else if (type == "ExpressionStatementNode") {
        auto exprStmt = dynamic_pointer_cast<ExpressionStatementNode>(node);
        out << indent(indentLevel) << generateCode(exprStmt->getExpression(), 0) << "\n";
    } else if (type == "BinaryExpressionNode") {
        auto bin = dynamic_pointer_cast<BinaryExpressionNode>(node);
        out << generateCode(bin->getLeft(), 0) << " " << bin->getOperator() << " " << generateCode(bin->getRight(), 0);
    } else if (type == "UnaryExpressionNode") {
        auto un = dynamic_pointer_cast<UnaryExpressionNode>(node);
        out << un->getOperator() << generateCode(un->getOperand(), 0);
    } else if (type == "IdentifierNode") {
        auto id = dynamic_pointer_cast<IdentifierNode>(node);
        out << id->getName();
    } else if (type == "NumberNode") {
        auto num = dynamic_pointer_cast<NumberNode>(node);
        out << num->getValue();
    } else if (type == "StringLiteralNode") {
        auto str = dynamic_pointer_cast<StringLiteralNode>(node);
        out << '"' << str->getValue() << '"';
    } else if (type == "CharLiteralNode") {
        auto ch = dynamic_pointer_cast<CharLiteralNode>(node);
        out << "'" << ch->getValue() << "'";
    } else if (type == "BooleanNode") {
        auto b = dynamic_pointer_cast<BooleanNode>(node);
        out << (b->getValue() ? "True" : "False");
    } else if (type == "IfNode") {
        auto ifNode = dynamic_pointer_cast<IfNode>(node);
        out << indent(indentLevel) << "if " << generateCode(ifNode->getCondition(), 0) << ":\n";
        out << generateCode(ifNode->getThenBranch(), indentLevel + 1);
        if (ifNode->getElseBranch()) {
            out << indent(indentLevel) << "else:\n";
            out << generateCode(ifNode->getElseBranch(), indentLevel + 1);
        }
    } else if (type == "WhileNode") {
        auto wh = dynamic_pointer_cast<WhileNode>(node);
        out << indent(indentLevel) << "while " << generateCode(wh->getCondition(), 0) << ":\n";
        out << generateCode(wh->getBody(), indentLevel + 1);
    } else if (type == "ForNode") {
        auto forNode = dynamic_pointer_cast<ForNode>(node);
        out << indent(indentLevel) << "# for-loop converted to while\n";
        if (forNode->getInitializer()) out << generateCode(forNode->getInitializer(), indentLevel);
        out << indent(indentLevel) << "while " << generateCode(forNode->getCondition(), 0) << ":\n";
        out << generateCode(forNode->getBody(), indentLevel + 1);
        if (forNode->getIncrement()) out << indent(indentLevel + 1) << generateCode(forNode->getIncrement(), 0) << "\n";
    } else if (type == "ReturnNode") {
        auto ret = dynamic_pointer_cast<ReturnNode>(node);
        out << indent(indentLevel) << "return";
        if (ret->getReturnValue()) {
            out << " " << generateCode(ret->getReturnValue(), 0);
        }
        out << "\n";
    } else if (type == "FunctionDeclarationNode") {
        auto func = dynamic_pointer_cast<FunctionDeclarationNode>(node);
        out << indent(indentLevel) << "def " << func->getName() << "(";
        const auto& params = func->getParamNames();
        for (size_t i = 0; i < params.size(); ++i) {
            out << params[i];
            if (i < params.size() - 1) out << ", ";
        }
        out << "):\n";
        if (func->getBody()) {
            out << generateCode(func->getBody(), indentLevel + 1);
        } else {
            out << indent(indentLevel + 1) << "pass\n";
        }
    } else if (type == "FunctionCallNode") {
        auto call = dynamic_pointer_cast<FunctionCallNode>(node);
        out << call->getFunctionName() << "(";
        const auto& args = call->getArguments();
        for (size_t i = 0; i < args.size(); ++i) {
            out << generateCode(args[i], 0);
            if (i < args.size() - 1) out << ", ";
        }
        out << ")";
    } else if (type == "BreakNode") {
        out << indent(indentLevel) << "break\n";
    } else if (type == "ContinueNode") {
        out << indent(indentLevel) << "continue\n";
    } else {
        out << indent(indentLevel) << "# Unhandled node: " << type << "\n";
    }

    return out.str();
}
