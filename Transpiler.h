// Transpiler.h
#pragma once

#include <memory>
#include <string>
#include "Parser.h" // Required for AST classes like ProgramNode, ASTNode, etc.
using namespace std;

class Transpiler {
public:
    explicit Transpiler(shared_ptr<ProgramNode> root);
    
    // Main entry point: generates the Python code from the AST
    string transpile();

private:
    shared_ptr<ProgramNode> astRoot;
    string indentStr = "    "; // 4 spaces

    // Recursively generates Python code for the given AST node
    string generateCode(const shared_ptr<ASTNode>& node, int indent = 0);

    // Returns indentation string for current depth
    string indent(int level);
};
