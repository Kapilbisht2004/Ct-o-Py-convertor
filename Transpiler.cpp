
#include "Transpiler.h"
#include <sstream>
#include <typeinfo>

Transpiler::Transpiler(std::shared_ptr<ProgramNode> root) : astRoot(std::move(root)) {}

std::string Transpiler::transpile() {
    return generateCode(astRoot);
}

std::string Transpiler::indent(int level) {
    return std::string(level * indentStr.size(), ' ');
}


std::string Transpiler::generateCode(const std::shared_ptr<ASTNode>& node, int indentLevel) {
    if (!node) return "";
    std::ostringstream out;

    const std::string& type = node->type_name;

    if (type == "ProgramNode" || type == "BlockNode") {
        for (const auto& stmt : node->getChildren()) {
            out << generateCode(stmt, indentLevel);
        }
    } else if (type == "VariableDeclarationNode") {
        auto var = std::dynamic_pointer_cast<VariableDeclarationNode>(node);
        out << indent(indentLevel) << var->getName();
        if (var->getInitializer()) {
            out << " = " << generateCode(var->getInitializer(), 0);
        }
        out << "\n";
    } else if (type == "AssignmentStatementNode") {
        auto assign = std::dynamic_pointer_cast<AssignmentStatementNode>(node);
        out << indent(indentLevel) << generateCode(assign->getAssignment(), 0) << "\n";
    } else if (type == "AssignmentNode") {
        auto a = std::dynamic_pointer_cast<AssignmentNode>(node);
        out << a->getTargetName() << " = " << generateCode(a->getValue(), 0);
    } else if (type == "ExpressionStatementNode") {
        auto exprStmt = std::dynamic_pointer_cast<ExpressionStatementNode>(node);
        out << indent(indentLevel) << generateCode(exprStmt->getExpression(), 0) << "\n";
    } else if (type == "BinaryExpressionNode") {
        auto bin = std::dynamic_pointer_cast<BinaryExpressionNode>(node);
        out << generateCode(bin->getLeft(), 0) << " " << bin->getOperator() << " " << generateCode(bin->getRight(), 0);
    } else if (type == "UnaryExpressionNode") {
        auto un = std::dynamic_pointer_cast<UnaryExpressionNode>(node);
        out << un->getOperator() << generateCode(un->getOperand(), 0);
    } else if (type == "IdentifierNode") {
        auto id = std::dynamic_pointer_cast<IdentifierNode>(node);
        out << id->getName();
    } else if (type == "NumberNode") {
        auto num = std::dynamic_pointer_cast<NumberNode>(node);
        out << num->getValue();
    } else if (type == "StringLiteralNode") {
        auto str = std::dynamic_pointer_cast<StringLiteralNode>(node);
        out << '"' << str->getValue() << '"';
    } else if (type == "CharLiteralNode") {
        auto ch = std::dynamic_pointer_cast<CharLiteralNode>(node);
        out << "'" << ch->getValue() << "'";
    } else if (type == "BooleanNode") {
        auto b = std::dynamic_pointer_cast<BooleanNode>(node);
        out << (b->getValue() ? "True" : "False");
    } else if (type == "IfNode") {
        auto ifNode = std::dynamic_pointer_cast<IfNode>(node);
        out << indent(indentLevel) << "if " << generateCode(ifNode->getCondition(), 0) << ":\n";
        out << generateCode(ifNode->getThenBranch(), indentLevel + 1);
        if (ifNode->getElseBranch()) {
            out << indent(indentLevel) << "else:\n";
            out << generateCode(ifNode->getElseBranch(), indentLevel + 1);
        }
    } else if (type == "WhileNode") {
        auto wh = std::dynamic_pointer_cast<WhileNode>(node);
        out << indent(indentLevel) << "while " << generateCode(wh->getCondition(), 0) << ":\n";
        out << generateCode(wh->getBody(), indentLevel + 1);
    } else if (type == "ForNode") {
        auto forNode = std::dynamic_pointer_cast<ForNode>(node);
        out << indent(indentLevel) << "# for-loop converted to while\n";
        if (forNode->getInitializer()) out << generateCode(forNode->getInitializer(), indentLevel);
        out << indent(indentLevel) << "while " << generateCode(forNode->getCondition(), 0) << ":\n";
        out << generateCode(forNode->getBody(), indentLevel + 1);
        if (forNode->getIncrement()) out << indent(indentLevel + 1) << generateCode(forNode->getIncrement(), 0) << "\n";
    } else if (type == "ReturnNode") {
        auto ret = std::dynamic_pointer_cast<ReturnNode>(node);
        out << indent(indentLevel) << "return";
        if (ret->getReturnValue()) {
            out << " " << generateCode(ret->getReturnValue(), 0);
        }
        out << "\n";
    } else if (type == "FunctionDeclarationNode") {
        auto func = std::dynamic_pointer_cast<FunctionDeclarationNode>(node);
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
        auto call = std::dynamic_pointer_cast<FunctionCallNode>(node);
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
