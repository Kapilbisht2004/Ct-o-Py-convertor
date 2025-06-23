#pragma once

#include <string>
#include <vector>
#include <unordered_map>

// ADD THIS:
#include <algorithm> // For std::remove_if for trimming
#include <cctype>    // For std::isspace

using namespace std;

enum class TokenType
{
  Keyword,
  Identifier,
  IntegerNumber,
  FloatNumber,
  StringLiteral,
  CharLiteral,
  PreprocessorDirective,
  Operator,
  Symbol,
  EndOfFile,
  Error,
  Unknown,
  BooleanLiteral // Added for boolean literals (true/false)
};
string tokenTypeToString(TokenType type); // Defined in Lexer.cpp
struct Token
{
  TokenType type;
  string value;
  int line = 1; // Default values
  int col = 1;  // Default values
  // Default constructor
  Token() : type(TokenType::Unknown), value(""), line(1), col(1) {}
  // Add this constructor
  Token(TokenType type, const string &value, int line = 1, int col = 1)
      : type(type), value(value), line(line), col(col) {}

  // Add this method
  string toString() const
  {
    return tokenTypeToString(type) + ": " + value;
  }
};

// ADD THIS STRUCTURE (outside the Lexer class, but in the Lexer.h namespace)
struct MacroDefinition
{
  string name;
  bool isFunctionLike = false;
  vector<string> parameters; // For function-like macros
  string body;               // The raw replacement string (after '#' 'define' 'name(<params>)')
  int line = 0;              // Line number of definition
  bool valid = true;         // Flag to mark if parsing the definition was successful

  MacroDefinition(int l) : line(l) {} // Constructor to set line easily
};

class Lexer
{
public:
  explicit Lexer(const string &src);
  vector<Token> tokenize();
  Token nextToken();

  // ADD THIS PUBLIC GETTER
  const vector<MacroDefinition> &getDefinedMacros() const;

private:
  string source;
  size_t pos = 0;
  int line = 1;
  int col = 1;

  // ADD THIS MEMBER VARIABLE
  vector<MacroDefinition> m_definedMacros;

  char peek();
  char peek_char_at(size_t offset);
  char peek_next();
  char get();

  void skipWhitespace();
  void skipSingleLineComment();
  void skipMultiLineComment();
  void processPreprocessorDirective(); // Renamed and will be implemented

  Token lexStringLiteral();
  Token lexCharacterLiteral();
  // void skipPreprocessorDirective();
  Token lexNumber();
  Token tryLexOperator();
};
