#include "transpiler.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of
#include <cctype>    // For ::isspace

// Constructor
Transpiler::Transpiler() {}

// Utility: Indent given code by the number of 4-space groups specified by 'level_delta'.
// If 'code_block' is empty or only whitespace, and 'add_pass_if_empty' is true,
// it returns an indented "pass\n".
string Transpiler::indent(const string &code_block, int level_delta, bool add_pass_if_empty)
{
    if (level_delta <= 0)
    { // No change in indentation or outdent
        if (add_pass_if_empty)
        {
            string temp_block = code_block;
            temp_block.erase(std::remove_if(temp_block.begin(), temp_block.end(),
                                            [](unsigned char c)
                                            { return std::isspace(c); }),
                             temp_block.end());
            if (temp_block.empty())
                return "pass\n"; // pass is not prefixed by current level here, but relative
        }
        return code_block;
    }

    stringstream ss_in(code_block);
    string line;
    stringstream ss_out;
    string indentStr(level_delta * 4, ' ');

    bool first_line_outputted = false;
    bool content_actually_processed = false;

    string effectively_empty_check_str = code_block;
    effectively_empty_check_str.erase(std::remove_if(effectively_empty_check_str.begin(), effectively_empty_check_str.end(),
                                                     [](unsigned char c)
                                                     { return std::isspace(c); }),
                                      effectively_empty_check_str.end());

    if (effectively_empty_check_str.empty() && add_pass_if_empty)
    {
        return indentStr + "pass\n";
    }

    ss_in.clear();
    ss_in.str(code_block);

    while (getline(ss_in, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (first_line_outputted)
        { // Add newline before subsequent lines
            ss_out << "\n";
        }

        if (!line.empty())
        {
            ss_out << indentStr << line;
            content_actually_processed = true;
        }
        else if (first_line_outputted)
        {
            // If it's an empty line but not the first line overall, preserve it (as just a newline from above)
            // No indent string for completely empty lines.
        }
        first_line_outputted = true;
    }

    string result = ss_out.str();
    // Ensure the result (if not empty from processing) ends with a newline
    if (content_actually_processed && (result.empty() || result.back() != '\n'))
    {
        result += "\n";
    }
    else if (!content_actually_processed && add_pass_if_empty && !effectively_empty_check_str.empty())
    {
        // This case might be tricky: if input had content but became empty after processing (unlikely here)
        // OR if result is empty (e.g. code_block was just "\n")
        if (result.empty())
            result = indentStr + "pass\n";
    }

    return result;
}

// Entry point
string Transpiler::transpile(shared_ptr<ProgramNode> program)
{
    if (!program)
        return "Program AST is null\n";
    return transpileProgram(program);
}

// Transpile a ProgramNode
string Transpiler::transpileProgram(shared_ptr<ProgramNode> program)
{
    string code;
    for (const auto &stmt : program->getStatements())
    {
        // Top-level statements are at indent level 0.
        code += transpileStatement(stmt, 0);
    }
    return code;
}

// Dispatch to the correct statement transpiler.
// `base_indent_level` is the indentation level for the statement itself (if it's a leaf)
// or for the header of a control structure. Bodies will be `base_indent_level + 1`.
string Transpiler::transpileStatement(shared_ptr<StatementNode> stmt, int base_indent_level)
{
    if (!stmt)
        return "";
    string statement_code;

    if (auto assignStmt = dynamic_pointer_cast<AssignmentStatementNode>(stmt))
    {
        statement_code = transpileAssignmentStatement(assignStmt); // Returns "target = val\n" (unindented)
    }
    else if (auto varDecl = dynamic_pointer_cast<VariableDeclarationNode>(stmt))
    {
        statement_code = transpileVariableDeclaration(varDecl); // Returns "name = init\n" or "" (unindented)
    }
    else if (auto ifStmt = dynamic_pointer_cast<IfNode>(stmt))
    {
        // IfNode transpiler handles its own indentation based on base_indent_level
        return transpileIfStatement(ifStmt, base_indent_level);
    }
    else if (auto whileStmt = dynamic_pointer_cast<WhileNode>(stmt))
    {
        return transpileWhileStatement(whileStmt, base_indent_level);
    }
    else if (auto forStmt = dynamic_pointer_cast<ForNode>(stmt))
    {
        return transpileForStatement(forStmt, base_indent_level);
    }
    else if (auto exprStmt = dynamic_pointer_cast<ExpressionStatementNode>(stmt))
    {
        statement_code = transpileExpressionStatement(exprStmt); // Returns "expr\n" (unindented)
    }
    else if (auto returnStmt = dynamic_pointer_cast<ReturnNode>(stmt))
    {
        statement_code = transpileReturnStatement(returnStmt); // Returns "return ...\n" (unindented)
    }
    else if (auto blockStmt = dynamic_pointer_cast<BlockNode>(stmt))
    {
        // transpileBlock takes the indent level for ITS CONTENTS.
        return transpileBlock(blockStmt, base_indent_level + 1); // Standalone block's content is 1 level deeper
    }
    else if (auto funcDecl = dynamic_pointer_cast<FunctionDeclarationNode>(stmt))
    {
        return transpileFunctionDeclaration(funcDecl); // Handles its own indent for header and body
    }
    else if (auto printStmt = dynamic_pointer_cast<PrintNode>(stmt))
    {
        statement_code = transpilePrintStatement(printStmt); // "print(...)\n"
    }
    else if (auto printfStmt = dynamic_pointer_cast<PrintfNode>(stmt))
    {
        statement_code = transpilePrintfStatement(printfStmt); // "print(f\"...\")\n"
    }
    else if (auto scanfStmt = dynamic_pointer_cast<ScanfNode>(stmt))
    {
        statement_code = transpileScanfStatement(scanfStmt); // multiple lines, unindented
    }
    else if (auto breakStmt = dynamic_pointer_cast<BreakNode>(stmt))
    {
        statement_code = transpileBreakStatement(breakStmt); // "break\n"
    }
    else if (auto continueStmt = dynamic_pointer_cast<ContinueNode>(stmt))
    {
        statement_code = transpileContinueStatement(continueStmt); // "continue\n"
    }
    else
    {
        // ... error logging ...
        return indent("# UNHANDLED_STATEMENT_TYPE: " + (stmt ? stmt->type_name : "null") + "\n", base_indent_level);
    }

    // If we got here, it was a "leaf" statement (not if/while/for/def/block which return fully indented structures)
    // So we indent the raw `statement_code`.
    if (statement_code.empty())
        return ""; // e.g. from uninitialized var decl
    return indent(statement_code, base_indent_level);
}

// --- Leaf Statement Transpilers (return unindented Python lines, ending with \n) ---
string Transpiler::transpilePrintStatement(shared_ptr<PrintNode> stmt)
{ /* ... same, returns "print(...)\n" */
    if (stmt->getExpression())
        return "print(" + transpileExpression(stmt->getExpression()) + ")\n";
    return "print()\n";
}
string Transpiler::transpilePrintfStatement(shared_ptr<PrintfNode> stmt)
{ /* ... same, returns "print(f\"...\")\n" */
    auto formatStringNode = dynamic_pointer_cast<StringLiteralNode>(stmt->getFormatStringExpression());
    if (!formatStringNode)
        return "# Error: printf format string is not a string literal\n";
    string formatStr = formatStringNode->getValue();
    vector<string> pyArgs;
    for (const auto &argExpr : stmt->getArguments())
        pyArgs.push_back(transpileExpression(argExpr));
    size_t argIdx = 0;
    string f_string_content = "";
    for (size_t i = 0; i < formatStr.length(); ++i)
    {
        if (formatStr[i] == '%')
        { /* ... printf to f-string logic ... */
            if (i + 1 < formatStr.length() && formatStr[i + 1] == '%')
            {
                f_string_content += '%';
                i++;
            }
            else if (i + 1 < formatStr.length())
            {
                if (argIdx < pyArgs.size())
                {
                    f_string_content += "{" + pyArgs[argIdx] + "}";
                    argIdx++;
                    i++;
                }
                else
                {
                    f_string_content += '%'; /* or also skip specifier: i++; */
                }
            }
            else
            {
                f_string_content += '%';
            }
        }
        else if (formatStr[i] == '{')
            f_string_content += "{{";
        else if (formatStr[i] == '}')
            f_string_content += "}}";
        else
        {
            if (formatStr[i] == '\n')
                f_string_content += "\\n";
            else if (formatStr[i] == '\t')
                f_string_content += "\\t";
            else if (formatStr[i] == '\"')
                f_string_content += "\\\"";
            else
                f_string_content += formatStr[i];
        }
    }
    return "print(f\"" + f_string_content + "\")\n";
}
string Transpiler::transpileScanfStatement(shared_ptr<ScanfNode> stmt)
{ /* ... same, returns potentially multiple lines, unindented */
    auto formatStringNode = dynamic_pointer_cast<StringLiteralNode>(stmt->getFormatStringExpression());
    if (!formatStringNode)
        return "# Error: scanf format string is not a string literal\n";
    string formatStr = formatStringNode->getValue();
    string result_code = "";
    vector<string> targetVars;
    for (const auto &argExpr : stmt->getArguments())
    {
        if (auto unary = dynamic_pointer_cast<UnaryExpressionNode>(argExpr))
        {
            if (unary->getOperator() == "&")
            {
                if (auto ident = dynamic_pointer_cast<IdentifierNode>(unary->getOperand()))
                {
                    targetVars.push_back(ident->getName());
                    continue;
                }
            }
        }
        targetVars.push_back("#INVALID_SCANF_TARGET");
    }
    stringstream fs(formatStr);
    string spec_token;
    size_t var_idx = 0;
    bool multiple_inputs_on_line = formatStr.find(' ') != string::npos && targetVars.size() > 1;
    if (multiple_inputs_on_line)
        result_code += "_temp_inputs = input(\"# Enter values for: " + formatStr + "\\n\").split()\n";
    while (fs >> spec_token && var_idx < targetVars.size())
    {
        string current_target_var = targetVars[var_idx];
        string rhs = "";
        if (multiple_inputs_on_line)
            rhs = "_temp_inputs[" + to_string(var_idx) + "]";
        else
            rhs = "input(\"# Enter value for " + spec_token + " (" + current_target_var + "): \\n\")";
        if (spec_token == "%d")
            result_code += current_target_var + " = int(" + rhs + ")\n";
        else if (spec_token == "%f")
            result_code += current_target_var + " = float(" + rhs + ")\n";
        else if (spec_token == "%s")
            result_code += current_target_var + " = str(" + rhs + ")\n";
        else if (spec_token == "%c")
            result_code += current_target_var + " = (" + rhs + ")[0] if " + rhs + " else ''\n";
        else
            result_code += current_target_var + " = " + rhs + " # Unhandled scanf specifier " + spec_token + "\n";
        var_idx++;
    }
    if (var_idx < targetVars.size())
        result_code += "# Warning: Not all scanf target vars used.\n";
    return result_code;
}
string Transpiler::transpileReturnStatement(shared_ptr<ReturnNode> stmt)
{ /* ... same ... */
    if (!stmt->getReturnValue())
        return "return\n";
    return "return " + transpileExpression(stmt->getReturnValue()) + "\n";
}
string Transpiler::transpileAssignmentStatement(shared_ptr<AssignmentStatementNode> stmt)
{ /* ... same ... */
    return transpileAssignmentNode(stmt->getAssignment()) + "\n";
}
string Transpiler::transpileVariableDeclaration(shared_ptr<VariableDeclarationNode> decl)
{ /* ... same ... */
    string name = decl->getName();
    if (decl->getInitializer())
        return name + " = " + transpileExpression(decl->getInitializer()) + "\n";
    return "";
}
string Transpiler::transpileExpressionStatement(shared_ptr<ExpressionStatementNode> stmt)
{ /* ... same ... */
    return transpileExpression(stmt->getExpression()) + "\n";
}
string Transpiler::transpileBreakStatement(shared_ptr<BreakNode> stmt) { return "break\n"; }
string Transpiler::transpileContinueStatement(shared_ptr<ContinueNode> stmt) { return "continue\n"; }

// --- Control Structure and Block Transpilers ---
// These functions take a `base_indent_level` which is the level for THEIR OWN header (e.g. "if cond:").
// Their bodies/contents will be at `base_indent_level + 1`.

string Transpiler::transpileBlock(shared_ptr<BlockNode> block, int content_indent_level)
{
    string collected_code_for_block_content;
    if (block)
    {
        for (const auto &stmt_in_block : block->getStatements())
        {
            // Each statement inside this block will be rendered at `content_indent_level`
            collected_code_for_block_content += transpileStatement(stmt_in_block, content_indent_level);
        }
    }
    // If collected_code_for_block_content is empty (e.g. from empty BlockNode or all children were empty decls)
    // The `indent` utility should handle adding "pass" correctly IF it was called from If/While etc
    // that pass add_pass_if_empty = true.
    // Here, transpileBlock just returns the potentially multi-line, correctly indented string.
    // No further call to `indent()` needed on the result of transpileBlock.
    // The `pass` for an empty block should be generated here.
    if (collected_code_for_block_content.empty() || std::all_of(collected_code_for_block_content.begin(), collected_code_for_block_content.end(),
                                                                [](unsigned char c)
                                                                { return std::isspace(c); }))
    {
        return indent("pass\n", content_indent_level);
    }
    return collected_code_for_block_content;
}

string Transpiler::transpileIfStatement(shared_ptr<IfNode> stmt, int base_indent_level)
{
    string condition = transpileExpression(stmt->getCondition());
    string if_header = indent("if " + condition + ":\n", base_indent_level);

    string thenBranch_code = transpileStatement(stmt->getThenBranch(), base_indent_level + 1);

    string code = if_header + thenBranch_code;

    if (stmt->getElseBranch())
    {
        if (auto elseIf = dynamic_pointer_cast<IfNode>(stmt->getElseBranch()))
        {
            // elif header should be at the same level as the 'if'
            code += transpileIfStatement(elseIf, base_indent_level); // This prepends "el" internally later
            // Temporarily prepend "el"
            if (code.rfind("if ", 0) == 0 || code.find("\nif ") != string::npos)
            {                                   // Rough check if transpileIf already produced an "if "
                size_t pos = code.rfind("if "); // find last "if "
                if (pos != string::npos)
                    code.replace(pos, 2, "elif "); // Replace "if " with "elif "
            }
        }
        else
        {
            code += indent("else:\n", base_indent_level);
            code += transpileStatement(stmt->getElseBranch(), base_indent_level + 1);
        }
    }
    return code;
}

string Transpiler::transpileWhileStatement(shared_ptr<WhileNode> stmt, int base_indent_level)
{
    string condition = transpileExpression(stmt->getCondition());
    string while_header = indent("while " + condition + ":\n", base_indent_level);
    string body_code = transpileStatement(stmt->getBody(), base_indent_level + 1);
    return while_header + body_code;
}

string Transpiler::transpileForStatement(shared_ptr<ForNode> stmt, int base_indent_level)
{
    string result_code = "";
    if (stmt->getInitializer())
    {
        result_code += transpileStatement(stmt->getInitializer(), base_indent_level);
    }

    string condition_str_py = stmt->getCondition() ? transpileExpression(stmt->getCondition()) : "True";
    result_code += indent("while " + condition_str_py + ":\n", base_indent_level);

    string body_content = "";
    if (stmt->getBody())
    {
        body_content += transpileStatement(stmt->getBody(), base_indent_level + 1);
    }
    if (stmt->getIncrement())
    {
        // Increment needs to be treated as a statement at the end of the loop body
        string inc_expr_str = transpileExpression(stmt->getIncrement());
        body_content += indent(inc_expr_str + "\n", base_indent_level + 1);
    }

    // If body_content is empty after all this, ensure 'pass'
    if (body_content.empty() || std::all_of(body_content.begin(), body_content.end(), [](unsigned char c)
                                            { return std::isspace(c); }))
    {
        result_code += indent("pass\n", base_indent_level + 1);
    }
    else
    {
        result_code += body_content;
    }

    return result_code;
}

string Transpiler::transpileFunctionDeclaration(shared_ptr<FunctionDeclarationNode> funcDecl)
{
    // Function definition is at indent level 0 (or its context level if nested, but not handled here)
    string header = "def " + funcDecl->getName() + "(";
    const auto &paramNames = funcDecl->getParamNames();
    for (size_t i = 0; i < paramNames.size(); ++i)
    {
        header += paramNames[i];
        if (i < paramNames.size() - 1)
            header += ", ";
    }
    header += "):\n";
    string code = indent(header, 0); // Assuming top-level function is at base_indent 0

    auto bodyNode = funcDecl->getBody();
    if (bodyNode)
    {                                                // bodyNode is a BlockNode
        code += transpileStatement(bodyNode, 0 + 1); // Body contents start one level deeper
    }
    else
    {
        code += indent("pass\n", 0 + 1);
    }
    return code + "\n";
}

// --- Expression Transpilers (return Python expression strings, no newlines, no leading/trailing indent) ---
string Transpiler::transpileExpression(shared_ptr<ExpressionNode> expr)
{
    if (!expr)
        return "";
    if (auto binary = dynamic_pointer_cast<BinaryExpressionNode>(expr))
        return transpileBinaryExpression(binary);
    if (auto unary = dynamic_pointer_cast<UnaryExpressionNode>(expr))
        return transpileUnaryExpression(unary);
    if (auto ident = dynamic_pointer_cast<IdentifierNode>(expr))
        return transpileIdentifierNode(ident);
    if (auto number = dynamic_pointer_cast<NumberNode>(expr))
        return transpileNumberNode(number);
    if (auto strLiteral = dynamic_pointer_cast<StringLiteralNode>(expr))
        return transpileStringLiteralNode(strLiteral);
    if (auto charLiteral = dynamic_pointer_cast<CharLiteralNode>(expr))
        return transpileCharLiteralNode(charLiteral);
    if (auto boolLiteral = dynamic_pointer_cast<BooleanNode>(expr))
        return transpileBooleanNode(boolLiteral);
    if (auto funcCall = dynamic_pointer_cast<FunctionCallNode>(expr))
        return transpileFunctionCallNode(funcCall);
    if (auto assign = dynamic_pointer_cast<AssignmentNode>(expr))
        return transpileAssignmentNode(assign);
    cerr << "Transpiler Error: Unsupported expression type: " << (expr->type_name.empty() ? "Unknown" : expr->type_name) << endl;
    return "#UNSUPPORTED_EXPR_" + expr->type_name;
}
string Transpiler::transpileAssignmentNode(shared_ptr<AssignmentNode> assign)
{ /* ... same ... */
    return assign->getTargetName() + " = " + transpileExpression(assign->getValue());
}
string Transpiler::transpileIdentifierNode(shared_ptr<IdentifierNode> expr) { return expr->getName(); }
string Transpiler::transpileNumberNode(shared_ptr<NumberNode> expr) { return expr->getValue(); }
string Transpiler::transpileStringLiteralNode(shared_ptr<StringLiteralNode> expr)
{ /* ... same (with Python escaping) ... */
    string py_val = expr->getValue();
    stringstream ss;
    ss << "\"";
    for (char c : py_val)
    {
        switch (c)
        {
        case '"':
            ss << "\\\"";
            break;
        case '\\':
            ss << "\\\\";
            break;
        case '\n':
            ss << "\\n";
            break;
        case '\r':
            ss << "\\r";
            break;
        case '\t':
            ss << "\\t";
            break;
        default:
            ss << c;
            break;
        }
    }
    ss << "\"";
    return ss.str();
}
string Transpiler::transpileCharLiteralNode(shared_ptr<CharLiteralNode> expr)
{ /* ... same (with Python escaping) ... */
    string val = expr->getValue();
    if (val.length() != 1)
        return "'#ERR_CHAR'";
    char c = val[0];
    stringstream ss;
    ss << "'";
    switch (c)
    {
    case '\'':
        ss << "\\'";
        break;
    case '\\':
        ss << "\\\\";
        break;
    case '\n':
        ss << "\\n";
        break;
    case '\r':
        ss << "\\r";
        break;
    case '\t':
        ss << "\\t";
        break;
    default:
        ss << c;
        break;
    }
    ss << "'";
    return ss.str();
}
string Transpiler::transpileBooleanNode(shared_ptr<BooleanNode> expr) { return expr->getValue() ? "True" : "False"; }
string Transpiler::transpileFunctionCallNode(shared_ptr<FunctionCallNode> expr)
{ /* ... same ... */
    string result = expr->getFunctionName() + "(";
    const auto &args = expr->getArguments();
    for (size_t i = 0; i < args.size(); ++i)
    {
        result += transpileExpression(args[i]);
        if (i < args.size() - 1)
            result += ", ";
    }
    result += ")";
    return result;
}
string Transpiler::transpileBinaryExpression(shared_ptr<BinaryExpressionNode> expr)
{ /* ... same (with && || mapping) ... */
    string left = transpileExpression(expr->getLeft());
    string right = transpileExpression(expr->getRight());
    string op = expr->getOperator();
    if (op == "&&")
        op = "and";
    else if (op == "||")
        op = "or";
    return "(" + left + " " + op + " " + right + ")";
}
string Transpiler::transpileUnaryExpression(shared_ptr<UnaryExpressionNode> expr)
{ /* ... same (with ! and & mapping) ... */
    string op = expr->getOperator();
    string operand = transpileExpression(expr->getOperand());
    if (op == "!")
        op = "not ";
    if (op == "&")
        return operand;
    return op + operand;
}