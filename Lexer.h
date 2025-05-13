#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory> // For std::shared_ptr

enum class TokenType {
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
  type, // If this is actually needed; might be for type keywords specifically
  BooleanLiteral // Added for boolean literals (true/false)
};
std::string tokenTypeToString(TokenType type); // Defined in Lexer.cpp
struct Token {
  TokenType type;
  std::string value;
  int line = 1; // Default values
  int col = 1;  // Default values
  // Default constructor
  Token() : type(TokenType::Unknown), value(""), line(1), col(1) {}
  // Add this constructor
  Token(TokenType type, const std::string& value, int line = 1, int col = 1)
      : type(type), value(value), line(line), col(col) {}

  // Add this method
  std::string toString() const {
      return tokenTypeToString(type) + ": " + value;
  }
};

class Lexer {
public:
  explicit Lexer(const std::string &src);
  std::vector<Token> tokenize();
  Token nextToken();

private:
  std::string source;
  size_t pos = 0;
  int line = 1;
  int col = 1;

  char peek();
  char peek_char_at(size_t offset);
  char peek_next();
  char get();

  void skipWhitespace();
  void skipSingleLineComment();
  void skipMultiLineComment();

  Token lexStringLiteral();
  Token lexCharacterLiteral();
  Token lexPreprocessorDirective();
  Token lexNumber();
  Token tryLexOperator();
};

#endif