#include "Lexer.h"
#include <cctype>
#include <iostream>
using namespace std;

Lexer::Lexer(const string &src) : source(src), pos(0), line(1), col(1) {}

char Lexer::peek()
{
    return pos < source.size() ? source[pos] : '\0';
}

// Helper to peek N characters ahead
char Lexer::peek_char_at(size_t offset)
{
    if (pos + offset < source.size())
    {
        return source[pos + offset];
    }
    return '\0';
}

char Lexer::peek_next()
{
    return peek_char_at(1);
}

char Lexer::get()
{
    if (pos >= source.length())
        return '\0';
    char c = source[pos++];
    if (c == '\n')
    {
        line++;
        col = 1;
    }
    else
    {
        col++;
    }
    return c;
}

void Lexer::skipWhitespace()
{
    while (isspace(peek()))
        get();
}

void Lexer::skipSingleLineComment()
{
    // Assumes '//' has been identified
    get(); // Consume first /
    get(); // Consume second /
    while (peek() != '\n' && peek() != '\0')
    {
        get();
    }
    // Optionally consume the newline as well if it's not significant:
    if (peek() == '\n')
        get();
}

void Lexer::skipMultiLineComment()
{
    // Assumes '/*' has been identified
    get(); // Consume /
    get(); // Consume *
    while (true)
    {
        if (peek() == '\0')
        {
            // Unterminated comment, error handling might be needed here
            // For now, we'll just break and EOF will be reached.
            // A production lexer might store this as an error.
            break;
        }
        if (peek() == '*' && peek_next() == '/')
        {
            get(); // Consume *
            get(); // Consume /
            break;
        }
        get(); // Consume character inside comment
    }
}

Token Lexer::lexStringLiteral()
{
    int start_line = line;
    int start_col = col;

    get(); // Consume opening "
    string value;
    while (peek() != '"' && peek() != '\0')
    {
        if (peek() == '\\')
        {                               // Handle escape sequence
            get();                      // Consume '\'
            char escaped_char = peek(); // Look at char after '\'
            if (escaped_char == '\0')
            { // Unterminated escape sequence
                return {TokenType::Error, "Unterminated escape sequence in string literal: \"" + value,
                        start_line, start_col};
            }
            get(); // Consume the character after '\'
            switch (escaped_char)
            {
            case 'n':
                value += '\n';
                break;
            case 't':
                value += '\t';
                break;
            case '"':
                value += '"';
                break;
            case '\\':
                value += '\\';
                break;
            case 'r':
                value += '\r';
                break;
            case 'b':
                value += '\b';
                break;
            case 'f':
                value += '\f';
                break;
            default:
                // Unknown escape sequence, could be an error or pass char through
                value += escaped_char; // Simple: pass through
                // Or: return {TokenType::Error, "Unknown escape sequence \\" + string(1, escaped_char) + " in string"};
                break;
            }
        }
        else
        {
            value += get();
        }
    }
    if (peek() == '"')
    {
        get(); // Consume closing "
        return {TokenType::StringLiteral, value, start_line, start_col};
    }
    else
    {
        return {TokenType::Error, "Unterminated string literal: \"" + value, start_line, start_col};
    }
}

Token Lexer::lexCharacterLiteral()
{
    int start_line = line;
    int start_col = col;

    get(); // Consume opening '
    string value;
    if (peek() == '\'')
    {
        get(); // Consume closing '
        return {TokenType::Error, "Empty character literal", start_line, start_col};
    }
    if (peek() == '\0')
        return {TokenType::Error, "Unterminated character literal (EOF after ')", start_line, start_col};

    if (peek() == '\\')
    {          // Handle escape sequence
        get(); // Consume '\'
        char escaped_char = peek();
        if (escaped_char == '\0')
            return {TokenType::Error, "Unterminated escape sequence in char literal", start_line, start_col};
        get(); // Consume the character after '\'
        switch (escaped_char)
        {
        case 'n':
            value += '\n';
            break;
        case 't':
            value += '\t';
            break;
        case '\'':
            value += '\'';
            break;
        case '\\':
            value += '\\';
            break;
        case 'r':
            value += '\r';
            break;
        case 'b':
            value += '\b';
            break;
        case 'f':
            value += '\f';
            break;
        // Add more as needed
        default:
            value += escaped_char;
            break; // Or error for unknown
        }
    }
    else
    {
        value += get();
    }

    if (peek() == '\'')
    {
        get(); // Consume closing '
        return {TokenType::CharLiteral, value, start_line, start_col};
    }
    else
    {
        // Scan ahead to find the closing quote if possible
        string rest = "";
        while (peek() != '\'' && peek() != '\0' && peek() != '\n')
        {
            rest += get();
        }
        if (peek() == '\'')
        {
            get(); // Consume closing '
            // In C, character literals with more than one character are allowed
            // but might be implementation-defined. For the lexer, we'll accept it.
            value += rest;
            return {TokenType::CharLiteral, value, start_line, start_col};
        }
        return {TokenType::Error, "Unterminated character literal: '" + value + rest, start_line, start_col};
    }
}

Token Lexer::lexNumber()
{
    int start_line = line;
    int start_col = col;

    string num_str;
    bool is_float = false;
    char initial_char = peek();

    // Part 1: Integer part or leading decimal for floats like .5
    if (initial_char == '.')
    {
        // This case is entered if nextToken saw '.' followed by a digit
        num_str += get(); // Consume '.'
        is_float = true;
        while (isdigit(peek()))
        {
            num_str += get();
        }
    }
    else
    { // Starts with a digit
        while (isdigit(peek()))
        {
            num_str += get();
        }
        // Part 2: Fractional part
        if (peek() == '.')
        {
            bool dot_is_part_of_number = isdigit(peek_next()); // e.g. "1.2"
            if (!dot_is_part_of_number)
            {
                // Check for "1.e5" or "1.E5" case
                char char_after_dot = peek_next();
                if ((char_after_dot == 'e' || char_after_dot == 'E'))
                {
                    char exponent_char = peek_char_at(2); // char after 'e'/'E'
                    char sign_char_or_digit = (exponent_char == '+' || exponent_char == '-') ? peek_char_at(3) : exponent_char;
                    if (isdigit(sign_char_or_digit))
                    {
                        dot_is_part_of_number = true;
                    }
                }
            }

            if (dot_is_part_of_number)
            {
                num_str += get(); // Consume '.'
                is_float = true;
                while (isdigit(peek()))
                { // Consume digits after '.'
                    num_str += get();
                }
            }
            else
            {
                // Dot is not part of this number (e.g., "1.foo" or "1. " at end of input).
                // Return integer found so far. The dot will be tokenized separately.
                return {TokenType::IntegerNumber, num_str, start_line, start_col};
            }
        }
    }

    // Part 3: Exponent part
    if (peek() == 'e' || peek() == 'E')
    {
        char char_after_e = peek_next();
        bool valid_exponent = isdigit(char_after_e);
        if (!valid_exponent && (char_after_e == '+' || char_after_e == '-'))
        {
            valid_exponent = isdigit(peek_char_at(2));
        }

        if (valid_exponent)
        {
            is_float = true;
            num_str += get(); // Consume 'e' or 'E'
            if (peek() == '+' || peek() == '-')
            {
                num_str += get(); // Consume sign
            }
            // Digits for exponent must follow
            if (!isdigit(peek()))
            { // Should be caught by valid_exponent check, but as safeguard
                return {TokenType::Error, "Malformed exponent in number (expected digits): " + num_str, start_line, start_col};
            }
            while (isdigit(peek()))
            {
                num_str += get();
            }
        }
        else
        {
            // 'e' or 'E' is not followed by a valid exponent.
            // The number parsed so far (num_str) is complete. 'e'/'E' will be tokenized next.
        }
    }

    if (num_str.empty() || (num_str == "." && !is_float))
    { // Should be caught by call site
        return {TokenType::Error, "Invalid number tokenization state", start_line, start_col};
    }

    return is_float ? Token{TokenType::FloatNumber, num_str, start_line, start_col} : Token{TokenType::IntegerNumber, num_str, start_line, start_col};
}

// IMPLEMENT THE GETTER ADDED IN Lexer.h
const vector<MacroDefinition> &Lexer::getDefinedMacros() const
{
    return m_definedMacros;
}

static string trimWhitespace(const string &str)
{
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char c)
                                  { return std::isspace(c); });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c)
                                { return std::isspace(c); })
                   .base();
    return (start < end ? string(start, end) : string());
}

// REPLACE skipPreprocessorDirective() WITH THIS IMPLEMENTATION:
void Lexer::processPreprocessorDirective()
{
    // Assumes '#' has been identified by peek() and this method is called.
    int directive_start_line = line;
    int directive_start_col = col;
    get(); // Consume the '#' character

    string directive_name;
    skipWhitespace(); // Skip spaces after #
    while (isalnum(peek()))
    {
        directive_name += get();
    }

    if (directive_name == "define")
    {
        MacroDefinition currentMacro(directive_start_line);
        skipWhitespace(); // Skip spaces after "define"

        // 1. Parse Macro Name
        if (!isalpha(peek()) && peek() != '_')
        {
            // Error: macro name must be an identifier
            cerr << "Lexer Error (Line " << directive_start_line << "): Invalid macro name after #define." << endl;
            currentMacro.valid = false;
            // Consume rest of the line to avoid cascading errors
            while (peek() != '\n' && peek() != '\0')
                get();
            if (peek() == '\n')
                get();
            m_definedMacros.push_back(currentMacro); // Add invalid macro to list if desired for tracking
            return;
        }
        while (isalnum(peek()) || peek() == '_')
        {
            currentMacro.name += get();
        }

        // 2. Check for function-like macro (no space between name and '(' )
        if (peek() == '(')
        {
            currentMacro.isFunctionLike = true;
            get(); // Consume '('
            skipWhitespace();
            if (peek() != ')')
            { // If there are parameters
                string param_buffer;
                while (peek() != '\0')
                {
                    if (peek() == ')')
                    {
                        if (!param_buffer.empty())
                        {
                            currentMacro.parameters.push_back(trimWhitespace(param_buffer));
                        }
                        break;
                    }
                    else if (peek() == ',')
                    {
                        if (!param_buffer.empty())
                        {
                            currentMacro.parameters.push_back(trimWhitespace(param_buffer));
                        }
                        else
                        {
                            // Error: empty parameter or unexpected comma
                            cerr << "Lexer Error (Line " << line << "): Unexpected comma or empty parameter in macro " << currentMacro.name << endl;
                            currentMacro.valid = false; // Mark as invalid
                        }
                        param_buffer.clear();
                        get(); // Consume ','
                        skipWhitespace();
                    }
                    else if (peek() == '\\' && peek_next() == '\n')
                    {                          // Line continuation
                        param_buffer += get(); // '\'
                        param_buffer += get(); // '\n'
                    }
                    else if (peek() == '\n')
                    {
                        // Error: newline in parameter list without line continuation
                        cerr << "Lexer Error (Line " << line << "): Unexpected newline in parameter list for macro " << currentMacro.name << endl;
                        currentMacro.valid = false;
                        break; // Stop parsing params
                    }
                    else
                    {
                        param_buffer += get();
                    }
                }
            }
            if (peek() == ')')
            {
                get(); // Consume ')'
            }
            else
            {
                cerr << "Lexer Error (Line " << line << "): Missing ')' for function-like macro " << currentMacro.name << endl;
                currentMacro.valid = false;
            }
        }

        // 3. Parse Macro Body (rest of the logical line)
        skipWhitespace(); // Skip spaces before the body starts
        string body_str;
        while (true)
        {
            char currentChar = peek();
            if (currentChar == '\0')
                break;
            if (currentChar == '\\' && peek_next() == '\n')
            {
                get(); // Consume '\'
                get(); // Consume '\n'
                // In C, a line continuation effectively removes the backslash and newline.
                // For simplicity, we might keep them if our body parser can handle it or strip them.
                // For now, let's append a space to represent a logical continuation without the newline chars themselves.
                body_str += ' '; // Or decide if you want to perfectly reconstruct.
            }
            else if (currentChar == '\n')
            {
                get(); // Consume newline, ends the logical line for the macro body
                break;
            }
            else
            {
                body_str += get();
            }
        }
        currentMacro.body = trimWhitespace(body_str);

        if (currentMacro.name.empty())
        {
            cerr << "Lexer Error (Line " << directive_start_line << "): #define without a macro name." << endl;
            currentMacro.valid = false;
        }

        if (currentMacro.valid)
        { // Only add if parsed correctly
            m_definedMacros.push_back(currentMacro);
        }
        else
        {
            // Optionally add to m_definedMacros with valid=false for tracking, or just skip
            cerr << "Lexer Info: Macro definition for '" << currentMacro.name << "' on line " << directive_start_line << " was invalid and skipped." << endl;
        }
    }
    else
    {
        // It's some other preprocessor directive (e.g., #include, #ifdef), skip it
        // cerr << "Lexer Info: Skipping non-#define preprocessor directive '" << directive_name << "' on line " << directive_start_line << endl;
        while (true)
        {
            char currentChar = peek();
            if (currentChar == '\0')
                break;
            if (currentChar == '\\' && peek_next() == '\n')
            {
                get();
                get(); // Consume line continuation
            }
            else if (currentChar == '\n')
            {
                get(); // Consume newline
                break;
            }
            else
            {
                get();
            }
        }
    }
}

Token Lexer::tryLexOperator()
{
    int start_line = line;
    int start_col = col;

    char c1 = peek();
    char c2 = peek_next();

    // Two-character operators (try to match longest first)
    string op_str;
    op_str += c1;
    if (c2 != '\0')
        op_str += c2; // Tentatively form two-char string

    // Check for three-character operators first
    if (c1 == '.' && c2 == '.' && peek_char_at(2) == '.')
    {
        get();
        get();
        get(); // Consume "..."
        return {TokenType::Operator, "...", start_line, start_col};
    }
    if (op_str == "<<" && peek_char_at(2) == '=')
    {
        get();
        get();
        get(); // Consume "<<<"
        return {TokenType::Operator, "<<=", start_line, start_col};
    }
    if (op_str == ">>" && peek_char_at(2) == '=')
    {
        get();
        get();
        get(); // Consume ">>>"
        return {TokenType::Operator, ">>=", start_line, start_col};
    }

    // Two-character operators
    if (op_str == "==")
    {
        get();
        get();
        return {TokenType::Operator, "==", start_line, start_col};
    }
    if (op_str == "!=")
    {
        get();
        get();
        return {TokenType::Operator, "!=", start_line, start_col};
    }
    if (op_str == "<=")
    {
        get();
        get();
        return {TokenType::Operator, "<=", start_line, start_col};
    }
    if (op_str == ">=")
    {
        get();
        get();
        return {TokenType::Operator, ">=", start_line, start_col};
    }
    if (op_str == "+=")
    {
        get();
        get();
        return {TokenType::Operator, "+=", start_line, start_col};
    }
    if (op_str == "-=")
    {
        get();
        get();
        return {TokenType::Operator, "-=", start_line, start_col};
    }
    if (op_str == "*=")
    {
        get();
        get();
        return {TokenType::Operator, "*=", start_line, start_col};
    }
    if (op_str == "/=")
    {
        get();
        get();
        return {TokenType::Operator, "/=", start_line, start_col};
    }
    if (op_str == "%=")
    {
        get();
        get();
        return {TokenType::Operator, "%=", start_line, start_col};
    }
    if (op_str == "&&")
    {
        get();
        get();
        return {TokenType::Operator, "&&", start_line, start_col};
    }
    if (op_str == "||")
    {
        get();
        get();
        return {TokenType::Operator, "||", start_line, start_col};
    }
    if (op_str == "->")
    {
        get();
        get();
        return {TokenType::Operator, "->", start_line, start_col};
    }
    if (op_str == "++")
    {
        get();
        get();
        return {TokenType::Operator, "++", start_line, start_col};
    }
    if (op_str == "--")
    {
        get();
        get();
        return {TokenType::Operator, "--", start_line, start_col};
    }
    if (op_str == "<<")
    {
        get();
        get();
        return {TokenType::Operator, "<<", start_line, start_col};
    }
    if (op_str == ">>")
    {
        get();
        get();
        return {TokenType::Operator, ">>", start_line, start_col};
    }
    if (op_str == "&=")
    {
        get();
        get();
        return {TokenType::Operator, "&=", start_line, start_col};
    }
    if (op_str == "|=")
    {
        get();
        get();
        return {TokenType::Operator, "|=", start_line, start_col};
    }
    if (op_str == "^=")
    {
        get();
        get();
        return {TokenType::Operator, "^=", start_line, start_col};
    }
    if (op_str == ".*")
    {
        get();
        get();
        return {TokenType::Operator, ".*", start_line, start_col};
    }
    if (op_str == "::")
    {
        get();
        get();
        return {TokenType::Operator, "::", start_line, start_col};
    }

    // Single-character operators
    switch (c1)
    {
    case '+':
    case '-':
    case '*':
    case '/':
    case '%':
    case '=':
    case '!':
    case '<':
    case '>':
    case '&':
    case '|':
    case '^':
    case '~':
    case '.':
    case '?':
    case ':':  // Added ternary operators
        get(); // Consume the single character
        return {TokenType::Operator, string(1, c1), start_line, start_col};
    default:
        return {TokenType::Unknown, "", start_line, start_col}; // Not an operator we recognize
    }
}


Token Lexer::nextToken()
{
    // 1. Skip Whitespace and Comments AND HANDLE PREPROCESSOR
    while (true)
    {
        skipWhitespace();

        char c = peek();

        // MODIFICATION START: Handle Preprocessor Directives
        if (c == '#')
        {
            // Check if '#' is the first non-whitespace char on a logical line.
            // This simple check might need refinement if you have complex line checks,
            // but for typical C, this works.
            // We need to check if 'pos' is at the beginning of a line or only whitespace preceded it.
            // A more robust way: track column since last newline, or check if all chars from last newline to 'pos' are whitespace.
            // For now, let's assume if we see '#', and it's not inside a comment/string, it's a directive.
            processPreprocessorDirective(); // This will parse #define or skip others
            continue;                       // Restart loop to find next token or more skippable items
        }
        // MODIFICATION END

        char n = peek_next();

        if (c == '/' && n == '/')
        {
            skipSingleLineComment();
            continue;
        }
        if (c == '/' && n == '*')
        {
            skipMultiLineComment();
            continue;
        }
        break; // No more whitespace, comments, or directives to skip
    }

    // ... (rest of nextToken remains the same: EOF, String, Char, Identifiers, Numbers, Operators, Symbols) ...
    // Ensure the original order of checks in nextToken is maintained.
    // The '#' check for preprocessor should be done early, after skipping initial whitespace on a line.

    int start_line = line; // Current line/col after skipping
    int start_col = col;
    char c = peek();

    // 2. End Of File
    if (c == '\0')
    {
        return {TokenType::EndOfFile, "", start_line, start_col};
    }

    // 3. String Literals
    if (c == '"')
    {
        return lexStringLiteral();
    }

    // 4. Character Literals
    if (c == '\'')
    {
        return lexCharacterLiteral();
    }

    // 5. Identifiers and Keywords
    if (isalpha(c) || c == '_')
    {
        string value;
        while (isalnum(peek()) || peek() == '_')
        {
            value += get();
        }

        static const unordered_map<string, bool> keywords = {
            {"auto", true},
            {"break", true},
            {"case", true},
            {"char", true},
            {"const", true},
            {"continue", true},
            {"default", true},
            {"do", true},
            {"double", true},
            {"else", true},
            {"enum", true},
            {"extern", true},
            {"float", true},
            {"for", true},
            {"goto", true},
            {"if", true},
            {"int", true},
            {"long", true},
            {"register", true},
            {"return", true},
            {"short", true},
            {"signed", true},
            {"sizeof", true},
            {"static", true},
            {"struct", true},
            {"switch", true},
            {"typedef", true},
            {"union", true},
            {"unsigned", true},
            {"void", true},
            {"volatile", true},
            {"while", true},
            {"bool", true},
            {"true", true},
            {"false", true},
        };

        if (keywords.find(value) != keywords.end())
        {
            if (value == "true" || value == "false")
            {
                return {TokenType::BooleanLiteral, value, start_line, start_col};
            }
            return {TokenType::Keyword, value, start_line, start_col};
        }
        return {TokenType::Identifier, value, start_line, start_col};
    }

    // 6. Numbers
    if (isdigit(c) || (c == '.' && isdigit(peek_next())))
    {
        return lexNumber();
    }

    // 7. Operators
    Token opToken = tryLexOperator();
    if (opToken.type != TokenType::Unknown)
    {
        return opToken;
    }

    // 8. Symbols
    switch (c)
    {
    case ';':
    case ',':
    case '(':
    case ')':
    case '{':
    case '}':
    case '[':
    case ']':
        get();
        return {TokenType::Symbol, string(1, c), start_line, start_col};
    }

    // 9. Unrecognized character
    get();
    return {TokenType::Error, "Unrecognized character: " + string(1, c), start_line, start_col};
}

vector<Token> Lexer::tokenize()
{
    vector<Token> tokens;
    Token token;
    do
    {
        token = nextToken();
        tokens.push_back(token);
        if (token.type == TokenType::Error)
        {
            // Optional: Stop tokenizing on first error, or collect all errors.
            // For now, we continue, but a parser might stop.
            // cerr << "Lexical Error: " << token.value << " at line " << token.line << ", col " << token.col << endl;
        }
    } while (token.type != TokenType::EndOfFile);
    return tokens;
}

string tokenTypeToString(TokenType type)
{
    switch (type)
    {
    case TokenType::Keyword:
        return "Keyword";
    case TokenType::Identifier:
        return "Identifier";
    case TokenType::IntegerNumber:
        return "IntegerNumber";
    case TokenType::FloatNumber:
        return "FloatNumber";
    case TokenType::StringLiteral:
        return "StringLiteral";
    case TokenType::CharLiteral:
        return "CharLiteral";
    case TokenType::PreprocessorDirective:
        return "PreprocessorDirective";
    case TokenType::Operator:
        return "Operator";
    case TokenType::Symbol:
        return "Symbol";
    case TokenType::EndOfFile:
        return "EndOfFile";
    case TokenType::Error:
        return "Error";
    case TokenType::BooleanLiteral:
        return "BooleanLiteral"; // Added support for boolean literals
    default:
        return "Unknown";
    }
}