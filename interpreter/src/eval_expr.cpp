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
  enum Kind { Number, Name, Op, LParen, RParen, LBracket, RBracket } kind;
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
    if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%') {
      out.push_back({Token::Op, "", c});
      i++; continue;
    }
    if (c == '(') { out.push_back({Token::LParen, "", 0}); i++; continue; }
    if (c == ')') { out.push_back({Token::RParen, "", 0}); i++; continue; }
    if (c == '[') { out.push_back({Token::LBracket, "", 0}); i++; continue; }
    if (c == ']') { out.push_back({Token::RBracket, "", 0}); i++; continue; }
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
      if (!t || t->kind != Token::Op
        || (t->op != '*' && t->op != '/' && t->op != '%')) break;
      char op = t->op; pos++;
      Value right = parse_atom();
      if (!ok) break;
      if (op == '*') {
        left.v *= right.v;
      } else if (op == '/') {
        if (right.v == 0) { std::cerr << "Division by zero\n"; ok = false; break; }
        left.v /= right.v;
      } else {
        //  fmod rather than %, since operands are carried as double. Whole
        //  operands still give a whole answer, so 7 % 2 comes back as int.
        if (right.v == 0) { std::cerr << "Division by zero\n"; ok = false; break; }
        left.v = std::fmod(left.v, right.v);
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

      // One bracket for arr[0], two for grid[1][2], each index an expression
      // of its own. The mangled name is built up as they are read.
      std::string mangled = t->text;
      while (peek() && peek()->kind == Token::LBracket) {
        pos++;
        Value index = parse_sum();
        const Token* close = peek();
        if (!ok || !close || close->kind != Token::RBracket) { ok = false; return {}; }
        pos++;
        mangled += "[" + std::to_string(static_cast<long long>(index.v)) + "]";
      }

      auto it = vars.find(mangled);
      if (it == vars.end()) {
        if (mangled == t->text) std::cerr << "Unknown variable in expression: " << t->text << "\n";
        else std::cerr << "Index out of range: " << mangled << "\n";
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
  //  An index counts too: `arr[i]` has no operator but still needs resolving,
  //  since the caller cannot look it up by name without knowing what `i` is.
  bool has_op = false;
  for (const Token& t : toks)
    if (t.kind == Token::Op || t.kind == Token::LBracket) { has_op = true; break; }
  if (!has_op) return std::nullopt;

  //  A lone element — `arr[0]`, `grid[1][2]`, `arr[i + 1]` — is answered
  //  straight from the map. Going through the arithmetic below would reject a
  //  string element as "Cannot use string in expression" when nothing
  //  arithmetic was asked for, and would round a whole number through double.
  if (toks.size() >= 4 && toks[0].kind == Token::Name && toks[1].kind == Token::LBracket) {
    std::string mangled = toks[0].text;
    size_t at = 1;
    bool whole = true;

    while (at < toks.size() && toks[at].kind == Token::LBracket) {
      int depth = 0;
      size_t closing = 0;
      for (size_t j = at; j < toks.size(); j++) {
        if (toks[j].kind == Token::LBracket) depth++;
        else if (toks[j].kind == Token::RBracket && --depth == 0) { closing = j; break; }
      }
      if (closing == 0) { whole = false; break; }

      const std::vector<Token> index_tokens(toks.begin() + at + 1, toks.begin() + closing);
      Parser index_parser{index_tokens, variables};
      Value index = index_parser.parse_sum();
      if (!index_parser.ok || index_parser.pos != index_tokens.size()) { whole = false; break; }

      mangled += "[" + std::to_string(static_cast<long long>(index.v)) + "]";
      at = closing + 1;
    }

    if (whole && at == toks.size()) {
      auto element = variables.find(mangled);
      if (element == variables.end()) {
        std::cerr << "Index out of range: " << mangled << "\n";
        return std::nullopt;
      }
      return element->second.value;
    }
  }

  Parser parser{toks, variables};
  Value result = parser.parse_sum();
  if (!parser.ok || parser.pos != toks.size()) return std::nullopt;

  // Real division: Float when any operand was Float or the result is fractional.
  if (result.is_float || std::floor(result.v) != result.v) return VarValue(result.v);

  if (result.v >= static_cast<double>(std::numeric_limits<int>::min())
    && result.v <= static_cast<double>(std::numeric_limits<int>::max())) {
    return VarValue(static_cast<int>(result.v));
  }

  //  Past what a long long can hold the cast would be undefined and wrap to a
  //  negative, so the result stays a float instead. (double)LLONG_MAX rounds
  //  up to 2^63, hence the strict <.
  if (result.v >= static_cast<double>(std::numeric_limits<long long>::min())
    && result.v < std::ldexp(1.0, 63)) {
    return VarValue(static_cast<long long>(result.v));
  }
  return VarValue(result.v);
}
