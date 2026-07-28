#include "../include/eval_expr.h"
#include <cctype>
#include <cmath>
#include <limits>
#include <vector>
#include <variant>
#include <type_traits>
#include <iostream>

namespace {

struct Token {
  enum Kind { Number, Name, Op, LParen, RParen } kind;
  std::string text;  // Number/Name spelling.
  char op = 0;       // Operator character for Op tokens.
};

// Returns false on an illegal character (e.g. a quote), which means the text
// is not an expression and the caller should fall back to literal parsing.
bool tokenize(const std::string& expr, std::vector<Token>& out) {
  size_t i = 0;
  while (i < expr.size()) {
    char c = expr[i];
    if (std::isspace(static_cast<unsigned char>(c))) { i++; continue; }

    if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
      size_t start = i;
      while (i < expr.size()
        && (std::isdigit(static_cast<unsigned char>(expr[i])) || expr[i] == '.')) i++;
      out.push_back({Token::Number, expr.substr(start, i - start), 0});
      continue;
    }
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
      size_t start = i;
      while (i < expr.size()
        && (std::isalnum(static_cast<unsigned char>(expr[i])) || expr[i] == '_')) i++;
      out.push_back({Token::Name, expr.substr(start, i - start), 0});
      continue;
    }
    if (c == '+' || c == '-' || c == '*' || c == '/') {
      out.push_back({Token::Op, "", c});
      i++; continue;
    }
    if (c == '(') { out.push_back({Token::LParen, "", 0}); i++; continue; }
    if (c == ')') { out.push_back({Token::RParen, "", 0}); i++; continue; }
    return false;  // Illegal character -> not an expression.
  }
  return true;
}

// A running numeric value; `is_float` tracks whether the result should be Float.
struct Value { double v = 0; bool is_float = false; };

struct Parser {
  const std::vector<Token>& toks;
  const std::map<std::string, Variable>& vars;
  size_t pos = 0;
  bool ok = true;

  const Token* peek() { return pos < toks.size() ? &toks[pos] : nullptr; }

  Value parse_sum() {
    Value left = parse_product();
    while (ok) {
      const Token* t = peek();
      if (!t || t->kind != Token::Op || (t->op != '+' && t->op != '-')) break;
      char op = t->op; pos++;
      Value right = parse_product();
      if (!ok) break;
      left.v = (op == '+') ? left.v + right.v : left.v - right.v;
      left.is_float = left.is_float || right.is_float;
    }
    return left;
  }

  Value parse_product() {
    Value left = parse_atom();
    while (ok) {
      const Token* t = peek();
      if (!t || t->kind != Token::Op || (t->op != '*' && t->op != '/')) break;
      char op = t->op; pos++;
      Value right = parse_atom();
      if (!ok) break;
      if (op == '*') {
        left.v *= right.v;
      } else {
        if (right.v == 0) { std::cerr << "Division by zero\n"; ok = false; break; }
        left.v /= right.v;
      }
      left.is_float = left.is_float || right.is_float;
    }
    return left;
  }

  Value parse_atom() {
    const Token* t = peek();
    if (!t) { ok = false; return {}; }

    if (t->kind == Token::Op && t->op == '-') {  // Unary minus.
      pos++;
      Value a = parse_atom();
      a.v = -a.v;
      return a;
    }
    if (t->kind == Token::LParen) {
      pos++;
      Value inner = parse_sum();
      const Token* close = peek();
      if (!close || close->kind != Token::RParen) { ok = false; return {}; }
      pos++;
      return inner;
    }
    if (t->kind == Token::Number) {
      pos++;
      try {
        return {std::stod(t->text), t->text.find('.') != std::string::npos};
      } catch (...) { ok = false; return {}; }
    }
    if (t->kind == Token::Name) {
      pos++;
      auto it = vars.find(t->text);
      if (it == vars.end()) {
        std::cerr << "Unknown variable in expression: " << t->text << "\n";
        ok = false; return {};
      }
      return resolve(it->second);
    }
    ok = false; return {};
  }

  Value resolve(const Variable& var) {
    return std::visit([&](auto&& value) -> Value {
      using T = std::decay_t<decltype(value)>;
      if constexpr (std::is_same_v<T, std::string>) {
        std::cerr << "Cannot use string in expression\n";
        ok = false; return {};
      } else if constexpr (std::is_same_v<T, bool>) {
        return {value ? 1.0 : 0.0, false};
      } else if constexpr (std::is_same_v<T, double>) {
        return {value, true};
      } else {
        return {static_cast<double>(value), false};
      }
    }, var.value);
  }
};

}  // namespace

std::optional<VarValue> eval_expr(const std::string& expr,
                                  const std::map<std::string, Variable>& variables) {
  std::vector<Token> toks;
  if (!tokenize(expr, toks)) return std::nullopt;  // Illegal char -> not an expression.

  // Engage only when there is an actual arithmetic operator token; otherwise
  // this is a plain literal or a lone variable, left for the caller to handle.
  bool has_op = false;
  for (const Token& t : toks) if (t.kind == Token::Op) { has_op = true; break; }
  if (!has_op) return std::nullopt;

  Parser parser{toks, variables};
  Value result = parser.parse_sum();
  if (!parser.ok || parser.pos != toks.size()) return std::nullopt;

  // Real division: Float when any operand was Float or the result is fractional.
  if (result.is_float || std::floor(result.v) != result.v) return VarValue(result.v);

  if (result.v >= static_cast<double>(std::numeric_limits<int>::min())
    && result.v <= static_cast<double>(std::numeric_limits<int>::max())) {
    return VarValue(static_cast<int>(result.v));
  }
  return VarValue(static_cast<long long>(result.v));
}
