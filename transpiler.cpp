#include "transpiler.h"
#include <iostream>
#include <sstream>
#include <string>
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

// MODIFY Transpiler::transpile
string Transpiler::transpile(shared_ptr<ProgramNode> program, const vector<MacroDefinition> &macros)
{
    if (!program)
    { // Should not happen if parser always returns a ProgramNode
        return "# Error: Program AST is null\n";
    }
    return transpileProgram(program, macros); // Pass macros along
}

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
{
    auto formatStringNode = dynamic_pointer_cast<StringLiteralNode>(stmt->getFormatStringExpression());
    if (!formatStringNode)
        return "# Error: scanf format string is not a string literal\n";
    string formatStr = formatStringNode->getValue();
    string result_code = "";

    vector<string> py_target_vars_str; // Stores the Python string representation of the target L-Value

    for (const auto &argExpr : stmt->getArguments())
    {
        if (auto unaryNode = dynamic_pointer_cast<UnaryExpressionNode>(argExpr))
        {
            if (unaryNode->getOperator() == "&")
            {
                // The operand of '&' is the actual L-Value (e.g., IdentifierNode, ArraySubscriptNode)
                // Transpile this L-Value expression to get its Python string representation.
                py_target_vars_str.push_back(transpileExpression(unaryNode->getOperand()));
                continue;
            }
        }
        // Fallback: if not a recognized &expression.
        // This could be an error for complex expressions not meant as simple scanf targets.
        // For robustness, we can try to transpile it, but it might lead to invalid Python.
        cerr << "Transpiler Warning: Scanf argument '" << transpileExpression(argExpr)
             << "' is not a simple address-of expression. Transpiling as is for target." << endl;
        py_target_vars_str.push_back(transpileExpression(argExpr)); // Attempt to transpile it
    }

    stringstream fs(formatStr);
    string spec_token;
    size_t var_idx = 0;
    // A more robust check for multiple inputs: count non-whitespace "segments" in formatStr
    // or simply check if there's more than one target variable.
    bool multiple_inputs_on_line = (formatStr.find_first_of(" \t\n") != string::npos && py_target_vars_str.size() > 1) ||
                                   (py_target_vars_str.size() > 1 && formatStr.find_first_of(" \t\n") == string::npos && formatStr.find('%') != formatStr.rfind('%'));

    if (multiple_inputs_on_line && !py_target_vars_str.empty())
    {
        // Create a temporary prompt string
        string temp_prompt = "Enter values for format '";
        for (char fc : formatStr)
        {
            if (fc == '\n')
                temp_prompt += "\\n";
            else
                temp_prompt += fc;
        }
        temp_prompt += "'";

        result_code += "_temp_inputs = input(\"" + temp_prompt + "\\n\").split()\n";
    }

    size_t spec_count = 0; // Count actual format specifiers encountered
    string temp_format_str = formatStr;
    size_t pos = 0;
    while ((pos = temp_format_str.find('%', pos)) != string::npos)
    {
        if (pos + 1 < temp_format_str.length() && temp_format_str[pos + 1] != '%')
        {
            spec_count++;
        }
        pos++; // Move past the current '%'
    }

    while (fs >> spec_token && var_idx < py_target_vars_str.size() && var_idx < spec_count)
    {
        // Ensure spec_token actually starts with % and is a valid specifier
        if (spec_token.rfind('%', 0) != 0)
        { // if spec_token doesn't start with %
            // This might happen if formatStr has text between specifiers, e.g. "val1: %d val2: %d"
            // We need a more robust way to extract only specifiers.
            // For now, let's assume simple space-separated specifiers or just one.
            // This part of scanf parsing is complex.
            // A simple fix for now: if not starting with %, skip it or find next %
            string actual_spec;
            size_t p_pos = spec_token.find('%');
            if (p_pos != string::npos)
            {
                actual_spec = spec_token.substr(p_pos);
            }
            else
            {
                // No specifier found in this token, maybe it's just text. Skip or log.
                continue; // This might skip intended variables if format string is complex
            }
            spec_token = actual_spec;
        }

        string current_target_var_str = py_target_vars_str[var_idx];
        string rhs;

        if (multiple_inputs_on_line)
        {
            rhs = "_temp_inputs[" + to_string(var_idx) + "]";
        }
        else
        {
            // Prompt for single input
            string prompt_message = "Enter value for " + spec_token + " (" + current_target_var_str + "): ";
            rhs = "input(\"" + prompt_message + "\\n\")"; // Added newline for better prompt
        }

        if (spec_token == "%d")
            result_code += current_target_var_str + " = int(" + rhs + ")\n";
        else if (spec_token == "%f")
            result_code += current_target_var_str + " = float(" + rhs + ")\n";
        else if (spec_token == "%s")
            result_code += current_target_var_str + " = " + rhs + "\n";
        else if (spec_token == "%c")
            result_code += current_target_var_str + " = (" + rhs + ")[0] if " + rhs + " else ''\n";
        else
            result_code += current_target_var_str + " = " + rhs + " # Unhandled scanf specifier " + spec_token + "\n";
        var_idx++;
    }

    if (var_idx < py_target_vars_str.size())
    {
        result_code += "# Warning: Not all scanf target variables were assigned due to too few format specifiers processed.\n";
    }
    if (var_idx < spec_count && multiple_inputs_on_line)
    { // If more specifiers were expected than inputs handled
        result_code += "# Warning: More format specifiers than provided input slots processed for _temp_inputs.\n";
    }
    return result_code;
}
string Transpiler::transpileReturnStatement(shared_ptr<ReturnNode> stmt)
{ /* ... same ... */
    if (!stmt->getReturnValue())
        return "return\n";
    return "return " + transpileExpression(stmt->getReturnValue()) + "\n";
}
// string Transpiler::transpileAssignmentStatement(shared_ptr<AssignmentStatementNode> stmt)
// { /* ... same ... */
//     return transpileAssignmentNode(stmt->getAssignment()) + "\n";
// }
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

// string Transpiler::transpileIfStatement(shared_ptr<IfNode> stmt, int base_indent_level)
// {
//     string condition = transpileExpression(stmt->getCondition());
//     string if_header = indent("if " + condition + ":\n", base_indent_level);

//     string thenBranch_code = transpileStatement(stmt->getThenBranch(), base_indent_level + 1);

//     string code = if_header + thenBranch_code;

//     if (stmt->getElseBranch())
//     {
//         if (auto elseIf = dynamic_pointer_cast<IfNode>(stmt->getElseBranch()))
//         {
//             // elif header should be at the same level as the 'if'
//             code += transpileIfStatement(elseIf, base_indent_level); // This prepends "el" internally later
//             // Temporarily prepend "el"
//             if (code.rfind("if ", 0) == 0 || code.find("\nif ") != string::npos)
//             {                                   // Rough check if transpileIf already produced an "if "
//                 size_t pos = code.rfind("if "); // find last "if "
//                 if (pos != string::npos)
//                     code.replace(pos, 2, "elif "); // Replace "if " with "elif "
//             }
//         }
//         else
//         {
//             code += indent("else:\n", base_indent_level);
//             code += transpileStatement(stmt->getElseBranch(), base_indent_level + 1);
//         }
//     }
//     return code;
// }

// PASTE THIS NEW CODE IN ITS PLACE
string Transpiler::transpileIfStatement(shared_ptr<IfNode> stmt, int base_indent_level)
{
    // 1. Transpile the initial 'if' part
    string condition = transpileExpression(stmt->getCondition());
    string code = indent("if " + condition + ":\n", base_indent_level);
    code += transpileStatement(stmt->getThenBranch(), base_indent_level + 1);

    // 2. Start processing the chain of 'else' branches
    shared_ptr<StatementNode> current_else_branch = stmt->getElseBranch();

    // Loop through the chain of 'else if's
    while (current_else_branch)
    {
        // Try to cast the current else branch to an IfNode.
        // If successful, it's an 'else if' construct.
        if (auto else_if_node = dynamic_pointer_cast<IfNode>(current_else_branch))
        {
            // It's an 'else if', so generate an 'elif'.
            string elif_condition = transpileExpression(else_if_node->getCondition());
            code += indent("elif " + elif_condition + ":\n", base_indent_level);
            code += transpileStatement(else_if_node->getThenBranch(), base_indent_level + 1);

            // Move to the next link in the chain for the next loop iteration.
            current_else_branch = else_if_node->getElseBranch();
        }
        else
        {
            // It's a final 'else' block (not another 'if').
            code += indent("else:\n", base_indent_level);
            code += transpileStatement(current_else_branch, base_indent_level + 1);

            // Break the loop since we've handled the final 'else'.
            current_else_branch = nullptr;
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
string Transpiler::transpileForStatement(shared_ptr<ForNode> forNode, int current_indent_level)
{
    string code;
    string loopVar;
    string startValue = "0"; // Default if no explicit start
    auto initializer = forNode->getInitializer();
    string init_code_for_while_fallback; // Code for initializer if using while loop

    // Handle Initializer
    if (auto varDecl = dynamic_pointer_cast<VariableDeclarationNode>(initializer))
    {
        loopVar = varDecl->getName();
        if (auto initExpr = varDecl->getInitializer())
        {
            startValue = transpileExpression(initExpr);
        }
        init_code_for_while_fallback = transpileVariableDeclaration(varDecl); // For while loop
    }
    else if (auto exprStmt = dynamic_pointer_cast<ExpressionStatementNode>(initializer))
    {
        init_code_for_while_fallback = transpileExpressionStatement(exprStmt); // For while loop
        if (auto assignNode = dynamic_pointer_cast<AssignmentNode>(exprStmt->getExpression()))
        {
            if (auto identLVal = dynamic_pointer_cast<IdentifierNode>(assignNode->getLValue()))
            {
                loopVar = identLVal->getName();
                startValue = transpileExpression(assignNode->getRValue());
            }
            else
            {
                // Initializer assignment target is not a simple identifier, fallback to while
                loopVar.clear(); // Cannot optimize to range
            }
        }
        else
        {
            // Initializer is an expression statement but not a simple assignment.
            loopVar.clear(); // Cannot optimize to range
        }
    }
    else if (initializer)
    { // Some other statement type? Unlikely for valid C for-loop init.
        return indent("# Unsupported for-loop initializer type: " + initializer->type_name + "\n", current_indent_level);
    }
    // If loopVar is still empty after processing initializer, we must use while loop fallback.

    // Handle Condition
    string stopValue;                 // Only for range optimization
    bool inclusive_for_range = false; // Only for range optimization
    auto condition_expr_node = forNode->getCondition();
    string condition_py_expr_for_while = "True"; // Default for while if no C condition

    if (condition_expr_node)
    {
        condition_py_expr_for_while = transpileExpression(condition_expr_node);
        if (!loopVar.empty())
        { // Attempt range optimization only if loopVar is identified
            if (auto binaryCond = dynamic_pointer_cast<BinaryExpressionNode>(condition_expr_node))
            {
                string op = binaryCond->getOperator();
                if (auto leftId = dynamic_pointer_cast<IdentifierNode>(binaryCond->getLeft()))
                {
                    if (leftId->getName() == loopVar)
                    {
                        if (op == "<" || op == "<=")
                        {
                            stopValue = transpileExpression(binaryCond->getRight());
                            inclusive_for_range = (op == "<=");
                        } // else: not a simple < or <=, stopValue remains empty for no range optimization
                    } // else: loopVar not on left, stopValue remains empty
                } // else: left of condition not id, stopValue remains empty
            } // else: condition not binary, stopValue remains empty
        }
    }

    // Handle Increment
    int step_for_range = 1; // Default step for range
    bool simple_increment_for_range = false;
    auto increment_expr_node = forNode->getIncrement();
    string increment_py_expr_for_while;

    if (increment_expr_node)
    {
        increment_py_expr_for_while = transpileExpression(increment_expr_node);
        if (!loopVar.empty())
        { // Attempt range optimization only if loopVar is identified
            if (auto assignInc = dynamic_pointer_cast<AssignmentNode>(increment_expr_node))
            {
                if (auto identLVal = dynamic_pointer_cast<IdentifierNode>(assignInc->getLValue()))
                {
                    if (identLVal->getName() == loopVar)
                    { // e.g. i = ...
                        if (auto binaryIncVal = dynamic_pointer_cast<BinaryExpressionNode>(assignInc->getRValue()))
                        { // e.g. i = i + 1
                            if (auto innerLeftIdent = dynamic_pointer_cast<IdentifierNode>(binaryIncVal->getLeft()))
                            {
                                if (innerLeftIdent->getName() == loopVar)
                                { // i = i ...
                                    if (auto numRight = dynamic_pointer_cast<NumberNode>(binaryIncVal->getRight()))
                                    { // i = i + N
                                        try
                                        {
                                            int val = stoi(numRight->getValue());
                                            if (binaryIncVal->getOperator() == "+")
                                            {
                                                step_for_range = val;
                                                simple_increment_for_range = true;
                                            }
                                            else if (binaryIncVal->getOperator() == "-")
                                            {
                                                step_for_range = -val;
                                                simple_increment_for_range = true;
                                            }
                                        }
                                        catch (const std::exception &)
                                        { /* stoi failed, not a simple number */
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else if (auto unaryInc = dynamic_pointer_cast<UnaryExpressionNode>(increment_expr_node))
            { // e.g. i++ or ++i
                if (auto operandId = dynamic_pointer_cast<IdentifierNode>(unaryInc->getOperand()))
                {
                    if (operandId->getName() == loopVar)
                    {
                        string op = unaryInc->getOperator();
                        if (op == "++")
                        {
                            step_for_range = 1;
                            simple_increment_for_range = true;
                        }
                        else if (op == "--")
                        {
                            step_for_range = -1;
                            simple_increment_for_range = true;
                        }
                    }
                }
            }
        }
    }

    // Decide whether to use range() or fallback to while
    bool use_range_optimization = !loopVar.empty() && !startValue.empty() && !stopValue.empty() && simple_increment_for_range && (step_for_range != 0);

    if (use_range_optimization)
    {
        string effective_stopValue_for_range = stopValue;
        if (inclusive_for_range)
        { // Python range's stop is exclusive
            if (step_for_range > 0)
                effective_stopValue_for_range = "(" + stopValue + " + 1)";
            else if (step_for_range < 0)
                effective_stopValue_for_range = "(" + stopValue + " - 1)";
        }

        string step_str_for_range;
        if (step_for_range != 1)
        { // Python's default step for range is 1
            step_str_for_range = ", " + to_string(step_for_range);
        }

        code += indent(loopVar + " = " + startValue + "\n", current_indent_level); // Ensure loop var is initialized if not by decl
        code += indent("for " + loopVar + " in range(" + startValue + ", " + effective_stopValue_for_range + step_str_for_range + "):\n", current_indent_level);
        auto body = forNode->getBody();
        if (body)
            code += transpileStatement(body, current_indent_level + 1);
        else
            code += indent("pass\n", current_indent_level + 1);
    }
    else
    {
        // Fallback to while loop
        if (!init_code_for_while_fallback.empty())
        {
            code += indent(init_code_for_while_fallback, current_indent_level); // Already has newline if needed
        }
        else if (!loopVar.empty() && dynamic_pointer_cast<VariableDeclarationNode>(initializer))
        {
            // If loop var was from a declaration in for() that didn't have an init expr, initialize it.
            // This specific 'startValue' (often "0") might need to be more carefully determined
            // if the var decl had no initializer in C for the 'range' case.
            code += indent(loopVar + " = " + startValue + "\n", current_indent_level);
        }
        // else: Initializer might have been complex and not translatable to a simple Python var init here.

        code += indent("while " + condition_py_expr_for_while + ":\n", current_indent_level);
        string bodyCode;
        if (auto body = forNode->getBody())
        {
            bodyCode += transpileStatement(body, current_indent_level + 1);
        }
        else
        {
            bodyCode += indent("pass\n", current_indent_level + 1);
        }

        if (!increment_py_expr_for_while.empty())
        { // Append transpiled increment expression
            bodyCode += indent(increment_py_expr_for_while + "\n", current_indent_level + 1);
        }
        code += bodyCode;
    }
    return code;
}
// REPLACE the old transpileFunctionDeclaration with this one:
// This should be the ONLY version in your file.

string Transpiler::transpileFunctionDeclaration(shared_ptr<FunctionDeclarationNode> funcDecl)
{
    const int base_indent = 0;
    ostringstream header;
    header << "def " << funcDecl->getName() << "(";

    // The corrected variable name
    const auto params = funcDecl->getParameters();

    // This loop now works because 'params' is correctly declared above
    for (size_t i = 0; i < params.size(); ++i)
    {
        if (i > 0)
            header << ", ";
        header << params[i].name;
    }
    header << "):\n";

    string code = indent(header.str(), base_indent);

    auto bodyNode = funcDecl->getBody();
    if (bodyNode && !bodyNode->getStatements().empty())
    {
        code += transpileStatement(bodyNode, base_indent + 1);
    }
    else
    {
        code += indent("pass\n", base_indent + 1);
    }
    return code;
}

string Transpiler::transpileAssignmentNode(shared_ptr<AssignmentNode> assign)
{
    string lvalue_py = transpileExpression(assign->getLValue()); // Assumes getLValue() exists
    string rvalue_py = transpileExpression(assign->getRValue()); // Assumes getRValue() exists
    return lvalue_py + " = " + rvalue_py;
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

// Transpiles ArrayDeclarationNode
// Returns the Python code line WITHOUT indent, but WITH a newline.
// e.g., "my_array = [None] * 10\n"
string Transpiler::transpileArrayDeclaration(shared_ptr<ArrayDeclarationNode> decl)
{
    string name = decl->getName();
    string size_py_expr = transpileExpression(decl->getSizeExpression());

    // In Python, C's `int arr[10];` is often represented as `arr = [None] * 10` or `arr = [0] * 10`.
    // Let's use [None] for generality, or you could use 0 if you check type.
    // Python needs parentheses around the size_py_expr if it could be complex (e.g. `var + 5`)
    // to ensure correct precedence with `*`.
    string py_decl = name + " = [None] * (" + size_py_expr + ")";

    // TODO: If supporting C initializers `int arr[3] = {1,2,3};`, they would be transpiled here.
    // e.g., `py_decl = name + " = [" + comma_separated_transpiled_initializers + "]";`

    return py_decl + "\n";
}

// Transpiles ArraySubscriptNode
// Returns Python expression string, e.g., "my_array[i]"
string Transpiler::transpileArraySubscriptNode(shared_ptr<ArraySubscriptNode> expr)
{
    string array_py_expr = transpileExpression(expr->getArrayExpression());
    string index_py_expr = transpileExpression(expr->getIndexExpression());

    return array_py_expr + "[" + index_py_expr + "]";
}

// --- MODIFY transpileStatement ---
string Transpiler::transpileStatement(shared_ptr<StatementNode> stmt, int base_indent_level)
{
    if (!stmt)
    {
        return "";
    }

    // ---- SECTION 1: Structural/Block statements ----
    if (auto funcDecl = dynamic_pointer_cast<FunctionDeclarationNode>(stmt))
    {
        return transpileFunctionDeclaration(funcDecl);
    }
    else if (auto ifStmt = dynamic_pointer_cast<IfNode>(stmt))
    {
        return transpileIfStatement(ifStmt, base_indent_level);
    }
    else if (auto forStmt = dynamic_pointer_cast<ForNode>(stmt))
    {
        return transpileForStatement(forStmt, base_indent_level);
    }
    else if (auto whileStmt = dynamic_pointer_cast<WhileNode>(stmt))
    {
        return transpileWhileStatement(whileStmt, base_indent_level);
    }
    else if (auto blockStmt = dynamic_pointer_cast<BlockNode>(stmt))
    {
        return transpileBlock(blockStmt, base_indent_level); // Pass base_indent_level, block will indent its content
    }

    // ---- SECTION 2: Leaf-like statements ----
    string statement_code_to_indent;

    // --- REMOVE transpileAssignmentStatement case ---
    // Assignments are now expressions within ExpressionStatementNode
    // Old case was:
    // else if (auto assignStmt = dynamic_pointer_cast<AssignmentStatementNode>(stmt)) {
    //     statement_code_to_indent = transpileAssignmentStatement(assignStmt);
    // }

    // The order of these 'else if' matters if there's inheritance.
    // ArrayDeclarationNode vs VariableDeclarationNode order is correct.
    if (auto arrayDecl = dynamic_pointer_cast<ArrayDeclarationNode>(stmt))
    { // Check this before VariableDeclarationNode if it inherits
        statement_code_to_indent = transpileArrayDeclaration(arrayDecl);
    }
    else if (auto varDecl = dynamic_pointer_cast<VariableDeclarationNode>(stmt))
    {
        statement_code_to_indent = transpileVariableDeclaration(varDecl);
    }
    else if (auto exprStmt = dynamic_pointer_cast<ExpressionStatementNode>(stmt))
    {
        statement_code_to_indent = transpileExpressionStatement(exprStmt); // This will handle assignments
    }
    else if (auto printfStmt = dynamic_pointer_cast<PrintfNode>(stmt))
    {
        statement_code_to_indent = transpilePrintfStatement(printfStmt);
    }
    else if (auto scanfStmt = dynamic_pointer_cast<ScanfNode>(stmt))
    {
        statement_code_to_indent = transpileScanfStatement(scanfStmt);
    }
    else if (auto returnStmt = dynamic_pointer_cast<ReturnNode>(stmt))
    {
        statement_code_to_indent = transpileReturnStatement(returnStmt);
    }
    else if (auto breakStmt = dynamic_pointer_cast<BreakNode>(stmt))
    {
        statement_code_to_indent = transpileBreakStatement(breakStmt);
    }
    else if (auto continueStmt = dynamic_pointer_cast<ContinueNode>(stmt))
    {
        statement_code_to_indent = transpileContinueStatement(continueStmt);
    }
    else
    { // Fallback for unhandled or if a structural node was missed above
        string node_type_name = "null_stmt_or_empty_type_name";
        if (stmt && !stmt->type_name.empty())
        {
            node_type_name = stmt->type_name;
        }
        else if (stmt)
        {
            node_type_name = typeid(*stmt).name();
        }
        return indent("# UNHANDLED_STATEMENT_TYPE: " + node_type_name + "\n", base_indent_level);
    }

    if (statement_code_to_indent.empty())
    {
        return "";
    }
    return indent(statement_code_to_indent, base_indent_level);
}
string Transpiler::transpileExpression(shared_ptr<ExpressionNode> expr)
{
    if (!expr)
        return "";
    if (auto binary = dynamic_pointer_cast<BinaryExpressionNode>(expr))
        return transpileBinaryExpression(binary);
    if (auto unary = dynamic_pointer_cast<UnaryExpressionNode>(expr))
        return transpileUnaryExpression(unary);
    if (auto arraySubscript = dynamic_pointer_cast<ArraySubscriptNode>(expr))
        return transpileArraySubscriptNode(arraySubscript);
    if (auto ident = dynamic_pointer_cast<IdentifierNode>(expr))
        return transpileIdentifierNode(ident);
    if (auto number = dynamic_pointer_cast<NumberNode>(expr))
        return transpileNumberNode(number);
    if (auto strLiteral = dynamic_pointer_cast<StringLiteralNode>(expr))
        return transpileStringLiteralNode(strLiteral);
    if (auto charLiteral = dynamic_pointer_cast<CharLiteralNode>(expr)) // You added this
        return transpileCharLiteralNode(charLiteral);
    if (auto boolLiteral = dynamic_pointer_cast<BooleanNode>(expr)) // You added this
        return transpileBooleanNode(boolLiteral);
    if (auto funcCall = dynamic_pointer_cast<FunctionCallNode>(expr)) // <<<< MAKE SURE THIS IS PRESENT AND ACTIVE
        return transpileFunctionCallNode(funcCall);                   // <<<< MAKE SURE THIS IS PRESENT AND ACTIVE
    if (auto assign = dynamic_pointer_cast<AssignmentNode>(expr))     // Check this is also present for assignments within expressions
        return transpileAssignmentNode(assign);

    cerr << "Transpiler Error: Unsupported expression type: " << (expr->type_name.empty() ? "Unknown" : expr->type_name) << endl;
    return "#UNSUPPORTED_EXPR_" + expr->type_name;
}