#include "transpiler.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::all_of
#include <cctype>    // For ::isspace

// ADD THESE INCLUDES FOR THE TEMPORARY LEXER/PARSER IN transpileMacroBody
#include "Lexer.h"  // We already have MacroDefinition from transpiler.h, but good to be explicit for Lexer class
#include "Parser.h" // For Parser class
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

// IMPLEMENT THE NEW HELPER FUNCTION
string Transpiler::transpileMacroBodyToPythonExpression(const string &c_macro_body_source, const vector<string> &macro_params)
{
    if (c_macro_body_source.empty())
    {
        return "None"; // Or handle as error, or empty string if appropriate
    }

    // Create a temporary lexer and parser for the macro body.
    // IMPORTANT: This temporary lexer should ideally not process further #defines within the macro body
    // or use a version of the lexer that has #define processing disabled for this specific call.
    // For simplicity now, our current Lexer will try to process them if they exist.
    // A more robust solution might involve a Lexer constructor flag to disable preprocessor handling.
    Lexer tempLexer(c_macro_body_source);
    vector<Token> bodyTokens;
    try
    {
        bodyTokens = tempLexer.tokenize();
        // We expect the body to be an expression, so it should end with EOF.
        // Remove the EOF token if present, as parseExpression doesn't expect it at the end of its input.
        if (!bodyTokens.empty() && bodyTokens.back().type == TokenType::EndOfFile)
        {
            bodyTokens.pop_back();
        }
    }
    catch (const std::exception &e)
    {
        cerr << "Transpiler Error: Could not tokenize macro body '" << c_macro_body_source << "': " << e.what() << endl;
        return "#ERROR_TOKENIZING_MACRO_BODY";
    }

    // Filter out any macros defined *within* this macro body's tokenization.
    // This is a simple guard; complex nested preproc would need more.
    if (!tempLexer.getDefinedMacros().empty())
    {
        cerr << "Transpiler Warning: Nested #define found and ignored within macro body: " << c_macro_body_source << endl;
    }

    Parser tempParser(bodyTokens);
    shared_ptr<ExpressionNode> bodyExpr;
    try
    {
        if (bodyTokens.empty() && !c_macro_body_source.empty() &&
            std::all_of(c_macro_body_source.begin(), c_macro_body_source.end(), [](unsigned char c)
                        { return std::isspace(c); }))
        {
            // If body was just whitespace, it's effectively empty.
            return "None"; // Or "" if that's more suitable for an empty expression in Python.
        }
        if (bodyTokens.empty() && !c_macro_body_source.empty())
        {
            // If body tokens are empty but C source was not (just whitespace), it's an issue.
            // If C source was also empty, 'return "None"' above handled it.
            // This case could mean the lexer consumed everything as skippable.
            cerr << "Transpiler Error: Macro body '" << c_macro_body_source << "' resulted in no tokens for parsing as expression." << endl;
            return "#ERROR_EMPTY_TOKENS_FOR_MACRO_BODY";
        }
        if (bodyTokens.empty() && c_macro_body_source.empty())
        {
            return "None"; // Macro body was literally empty
        }

        bodyExpr = tempParser.parseExpression(); // This should parse up to where it thinks the expression ends.
                                                 // If there are trailing tokens, parseExpression might not consume them all.
                                                 // We need to ensure the parser can handle a stream that is *only* an expression.
    }
    catch (const std::runtime_error &e)
    {
        // Check if the error is about "Expected primary expression, but got EndOfFile"
        // This can happen if the macro body is empty or only whitespace after tokenization.
        string error_what = e.what();
        if (bodyTokens.empty() && error_what.find("Expected primary expression") != string::npos && error_what.find("EndOfFile") != string::npos)
        {
            // Macro body was effectively empty (e.g. just comments)
            return "None"; // Or an empty string, depending on how Python should treat empty macro bodies
        }
        cerr << "Transpiler Error: Could not parse macro body '" << c_macro_body_source << "' as expression: " << e.what() << endl;
        return "#ERROR_PARSING_MACRO_BODY";
    }

    if (!bodyExpr)
    {
        cerr << "Transpiler Error: Parsing macro body '" << c_macro_body_source << "' yielded null expression." << endl;
        return "#ERROR_NULL_EXPR_MACRO_BODY";
    }

    // The transpiled expression might use macro parameters.
    // For this simple version, C macro expansion is textual. Python functions will handle parameters.
    // No direct text substitution in this transpiler stage for macro parameters.
    return transpileExpression(bodyExpr);
}

// // Entry point
// string Transpiler::transpile(shared_ptr<ProgramNode> program)
// {
//     if (!program)
//         return "Program AST is null\n";
//     return transpileProgram(program);
// }

// MODIFY Transpiler::transpile
string Transpiler::transpile(shared_ptr<ProgramNode> program, const vector<MacroDefinition> &macros)
{
    if (!program)
    { // Should not happen if parser always returns a ProgramNode
        return "# Error: Program AST is null\n";
    }
    return transpileProgram(program, macros); // Pass macros along
}

// // Transpile a ProgramNode
// string Transpiler::transpileProgram(shared_ptr<ProgramNode> program)
// {
//     string code;
//     for (const auto &stmt : program->getStatements())
//     {
//         // Top-level statements are at indent level 0.
//         code += transpileStatement(stmt, 0);
//     }
//     return code;
// }
string Transpiler::transpileProgram(shared_ptr<ProgramNode> program, const vector<MacroDefinition> &macros)
{
    string py_code;

    // --- 1. Transpile Macro Definitions ---
    string transpiled_macros_code;
    for (const auto &macroDef : macros)
    {
        if (!macroDef.valid)
            continue; // Skip invalid macros

        if (macroDef.isFunctionLike)
        {
            string pyParamsStr;
            for (size_t i = 0; i < macroDef.parameters.size(); ++i)
            {
                pyParamsStr += macroDef.parameters[i];
                if (i < macroDef.parameters.size() - 1)
                    pyParamsStr += ", ";
            }
            transpiled_macros_code += "def " + macroDef.name + "(" + pyParamsStr + "):\n";

            string pyMacroBodyExpr = transpileMacroBodyToPythonExpression(macroDef.body, macroDef.parameters);
            // For function-like macros, we assume the body is an expression to be returned.
            transpiled_macros_code += indent("return " + pyMacroBodyExpr + "\n", 1);
        }
        else
        {
            // Object-like macro
            string pyMacroBodyExpr = transpileMacroBodyToPythonExpression(macroDef.body, {}); // No params
            transpiled_macros_code += macroDef.name + " = " + pyMacroBodyExpr + "\n";
        }
    }
    if (!transpiled_macros_code.empty())
    {
        transpiled_macros_code += "\n"; // Add a blank line after macro definitions
    }

    py_code += transpiled_macros_code;

    // --- 2. Transpile Program Statements ---
    string program_statements_code;
    for (const auto &stmt : program->getStatements())
    {
        // Top-level statements are at indent level 0.
        program_statements_code += transpileStatement(stmt, 0);
    }
    py_code += program_statements_code;

    return py_code;
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
        return transpileBlock(blockStmt, base_indent_level); // Standalone block's content is 1 level deeper
    }
    else if (auto funcDecl = dynamic_pointer_cast<FunctionDeclarationNode>(stmt))
    {
        return transpileFunctionDeclaration(funcDecl); // Handles its own indent for header and body
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
            result_code += current_target_var + " = " + rhs + "\n";
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
string Transpiler::transpileForStatement(shared_ptr<ForNode> forNode, int current_indent_level) {
    string code;

    // Extract initializer
    string loopVar;
    string startValue = "0";
    auto initializer = forNode->getInitializer();

    if (auto varDecl = dynamic_pointer_cast<VariableDeclarationNode>(initializer)) {
        loopVar = varDecl->getName();
        if (auto initExpr = varDecl->getInitializer()) {
            startValue = transpileExpression(initExpr);
        }
    } else if (auto assignStmt = dynamic_pointer_cast<AssignmentStatementNode>(initializer)) {
        auto assignExpr = assignStmt->getAssignment();
        loopVar = assignExpr->getTargetName();
        startValue = transpileExpression(assignExpr->getValue());
    } else {
        return code + indent("# Unsupported for-loop initializer\n", current_indent_level);
    }

    // Extract condition
    string stopValue;
    bool inclusive = false;
    bool isLessComparison = true;
    auto condition = forNode->getCondition();

    if (auto binaryCond = dynamic_pointer_cast<BinaryExpressionNode>(condition)) {
        string op = binaryCond->getOperator();
        auto left = binaryCond->getLeft();
        auto right = binaryCond->getRight();

        if (auto leftId = dynamic_pointer_cast<IdentifierNode>(left)) {
            if (leftId->getName() == loopVar) {
                stopValue = transpileExpression(right);
                if (op == "<") isLessComparison = true;
                else if (op == "<=") { isLessComparison = true; inclusive = true; }
                else if (op == ">") isLessComparison = false;
                else if (op == ">=") { isLessComparison = false; inclusive = true; }
                else return code + indent("# Unsupported for-loop condition operator\n", current_indent_level);
            } else return code + indent("# Unsupported for-loop condition structure\n", current_indent_level);
        } else return code + indent("# Unsupported for-loop condition structure\n", current_indent_level);
    } else return code + indent("# Unsupported for-loop condition type\n", current_indent_level);

    // Extract increment and check if range is safe
    int step = 1;
    auto increment = forNode->getIncrement();
    bool safeForRange = false;

    if (auto assignInc = dynamic_pointer_cast<AssignmentNode>(increment)) {
        if (assignInc->getTargetName() == loopVar) {
            if (auto binaryInc = dynamic_pointer_cast<BinaryExpressionNode>(assignInc->getValue())) {
                string op = binaryInc->getOperator();
                if ((op == "+" || op == "-") && dynamic_pointer_cast<NumberNode>(binaryInc->getRight())) {
                    int val = stoi(dynamic_pointer_cast<NumberNode>(binaryInc->getRight())->getValue());
                    step = (op == "+") ? val : -val;
                    safeForRange = true;
                }
            }
        }
    } else if (auto unaryInc = dynamic_pointer_cast<UnaryExpressionNode>(increment)) {
        if (auto operandId = dynamic_pointer_cast<IdentifierNode>(unaryInc->getOperand())) {
            if (operandId->getName() == loopVar) {
                string op = unaryInc->getOperator();
                if (op == "++") step = 1;
                else if (op == "--") step = -1;
                safeForRange = true;
            }
        }
    }

    // Adjust stop value for inclusive comparison
    if (inclusive) {
        if (step > 0) stopValue = "(" + stopValue + " + 1)";
        else stopValue = "(" + stopValue + " - 1)";
    }

    // Transpile using range() if safe
    if (safeForRange) {
        code += indent("for " + loopVar + " in range(" + startValue + ", " + stopValue + ", " + to_string(step) + "):\n", current_indent_level);
        auto body = forNode->getBody();
        if (body) {
            code += transpileStatement(body, current_indent_level + 1);
        } else {
            code += indent("pass\n", current_indent_level + 1);
        }
    } else {
        // fallback to while loop
        code += indent(loopVar + " = " + startValue + "\n", current_indent_level);
        code += indent("while " + transpileExpression(condition) + ":\n", current_indent_level);

        string bodyCode = "";
        if (auto body = forNode->getBody()) {
            bodyCode += transpileStatement(body, current_indent_level + 1);
        } else {
            bodyCode += indent("pass\n", current_indent_level + 1);
        }

        // Append manual increment at end of loop body
        string incLine = transpileExpression(increment) + "\n";
        bodyCode += indent(incLine, current_indent_level + 1);
        code += bodyCode;
    }

    return code;
}


string Transpiler::transpileFunctionDeclaration(shared_ptr<FunctionDeclarationNode> funcDecl)
{
    const int base_indent = 0;
    ostringstream header;
    header << "def " << funcDecl->getName() << "(";

    const auto &paramNames = funcDecl->getParamNames();
    for (size_t i = 0; i < paramNames.size(); ++i)
    {
        if (i > 0) header << ", ";
        header << paramNames[i];
    }
    header << "):\n";

    string code = indent(header.str(), base_indent);

    auto bodyNode = funcDecl->getBody();
    if (bodyNode)
    {
        code += transpileStatement(bodyNode, base_indent + 1);
    }
    else
    {
        code += indent("pass\n", base_indent + 1);
    }
    return code;
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