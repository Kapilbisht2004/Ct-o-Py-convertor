// Transpiler.h
#pragma once

#include <memory>
#include <string>
#include "Parser.h" // Required for AST classes like ProgramNode, ASTNode, etc.

class Transpiler {
public:
    explicit Transpiler(std::shared_ptr<ProgramNode> root);
    
    // Main entry point: generates the Python code from the AST
    std::string transpile();

private:
    std::shared_ptr<ProgramNode> astRoot;
    std::string indentStr = "    "; // 4 spaces

    // Recursively generates Python code for the given AST node
    std::string generateCode(const std::shared_ptr<ASTNode>& node, int indent = 0);

    // Returns indentation string for current depth
    std::string indent(int level);
};
