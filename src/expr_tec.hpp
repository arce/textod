#pragma once
#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace EXPR {

using Vars = std::map<std::string, double>;

struct Error : std::runtime_error {
  explicit Error(const std::string& msg) : std::runtime_error(msg) {}
};

class Parser {
  const std::string& s;
  const Vars& v;
  size_t i = 0;

  static bool digit(char c) { return c >= '0' && c <= '9'; }
  static bool alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }

  [[noreturn]] void fail(const std::string& msg) const { throw Error("expression \"" + s + "\": " + msg); }

  void ws() { while (i < s.size() && s[i] == ' ') i++; }

  bool eat(char c) {
    ws();
    if (i < s.size() && s[i] == c) { i++; return true; }
    return false;
  }

  double number() {
    static const double P[] = {1, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15};
    double m = 0;
    int digits = 0, frac = 0;
    bool dot = false;
    for (; i < s.size() && (digit(s[i]) || (s[i] == '.' && !dot)); i++) {
      if (s[i] == '.') { dot = true; continue; }
      if (++digits > 15) fail("the number has too many digits");
      m = m * 10 + (s[i] - '0');
      if (dot) frac++;
    }
    if (!digits) fail("a number was expected");
    return m / P[frac];
  }

  double call(const std::string& name) {
    std::vector<double> a{sum()};
    while (eat(',')) a.push_back(sum());
    if (!eat(')')) fail("')' is missing");
    if (name == "min") return *std::min_element(a.begin(), a.end());
    if (name == "max") return *std::max_element(a.begin(), a.end());
    if (name == "sin" || name == "cos") {
      if (a.size() != 1) fail("'" + name + "' takes 1 value");
      return name == "sin" ? std::sin(a[0]) : std::cos(a[0]);
    }
    if (name == "abs") {
      if (a.size() != 1) fail("'abs' takes 1 value");
      return std::fabs(a[0]);
    }
    fail("unknown function '" + name + "'");
  }

  double primary() {
    if (eat('(')) {
      double x = sum();
      if (!eat(')')) fail("')' is missing");
      return x;
    }
    if (i < s.size() && (digit(s[i]) || s[i] == '.')) return number();
    if (i < s.size() && alpha(s[i])) {
      size_t j = i;
      while (j < s.size() && (alpha(s[j]) || digit(s[j]))) j++;
      std::string name = s.substr(i, j - i);
      i = j;
      if (eat('(')) return call(name);
      auto it = v.find(name);
      if (it == v.end() && name == "pi") return 3.14159265358979323846;
      if (it == v.end()) fail("unknown variable '" + name + "'");
      return it->second;
    }
    fail("a value was expected");
  }

  double unary() { return eat('-') ? -unary() : primary(); }

  double term() {
    double x = unary();
    for (;;) {
      if (eat('*')) x *= unary();
      else if (eat('/')) {
        double d = unary();
        if (d == 0) fail("division by zero");
        x /= d;
      } else return x;
    }
  }

  double sum() {
    double x = term();
    for (;;) {
      if (eat('+')) x += term();
      else if (eat('-')) x -= term();
      else return x;
    }
  }

public:
  Parser(const std::string& text, const Vars& vars) : s(text), v(vars) {}

  double run() {
    double x = sum();
    ws();
    if (i < s.size()) fail("unexpected '" + std::string(1, s[i]) + "'");
    return x;
  }
};

inline double eval(const std::string& s, const Vars& v = {}) { return Parser(s, v).run(); }

template <class F>
std::string fill(const std::string& d, const Vars& v, F fmt) {
  auto stop = [](char c) { return c == ' ' || c == ',' || (c >= 'A' && c <= 'Z'); };
  std::string out;
  for (size_t i = 0; i < d.size();) {
    if (stop(d[i])) { out += d[i++]; continue; }
    size_t j = i;
    for (int depth = 0; j < d.size(); j++) {
      if (d[j] == '(') depth++;
      else if (d[j] == ')') depth--;
      else if (!depth && stop(d[j])) break;
    }
    out += fmt(eval(d.substr(i, j - i), v));
    i = j;
  }
  return out;
}

}
