#include "expr_tec.hpp"
#include "svg_tec.hpp"
#include <iostream>
using namespace std;
static int fails = 0;
#define EQ(a, b) do { auto x = (a); auto y = (b); if (!(x == y)) { cout << "FAIL line " << __LINE__ << ": " #a "\n--- got\n" << x << "\n--- expected\n" << y << "\n"; fails++; } } while (0)
static string err(const string& s, const EXPR::Vars& v = {}) {
  try { EXPR::eval(s, v); } catch (const EXPR::Error& e) { return e.what(); }
  return "ok";
}
static string num(double x) { return SVG::num(x); }
int main() {
  EXPR::Vars v{{"w", 90}, {"h", 62}, {"ry", 14}, {"k", 2}};
  EQ(EXPR::eval("42"), 42.0);
  EQ(EXPR::eval("0.1"), 0.1);
  EQ(EXPR::eval("12.345"), 12.345);
  EQ(EXPR::eval(".5"), 0.5);
  EQ(EXPR::eval("3."), 3.0);
  EQ(EXPR::eval("0.000001"), 0.000001);
  EQ(EXPR::eval("1+2*3"), 7.0);
  EQ(EXPR::eval("(1+2)*3"), 9.0);
  EQ(EXPR::eval("10-4-3"), 3.0);
  EQ(EXPR::eval("100/10/5"), 2.0);
  EQ(EXPR::eval("-3+5"), 2.0);
  EQ(EXPR::eval("2*-3"), -6.0);
  EQ(EXPR::eval("--4"), 4.0);
  EQ(EXPR::eval(" 1 + 2 "), 3.0);
  EQ(EXPR::eval("w/2", v), 45.0);
  EQ(EXPR::eval("(h-2*ry)/2+ry*0.3", v), (62 - 2 * 14) / 2.0 + 14 * 0.3);
  EQ(EXPR::eval("min(14,max(6,w*0.15))", v), 13.5);
  EQ(EXPR::eval("max(54,3*16+22)"), 70.0);
  EQ(EXPR::eval("min(1,2,0.5)"), 0.5);
  EQ(EXPR::eval("abs(-3)*abs(4)"), 12.0);
  EQ(EXPR::eval("sin(0)+cos(0)"), 1.0);
  EQ(EXPR::eval("pi*2") > 6.28 && EXPR::eval("pi*2") < 6.29, true);
  EQ(err("abs(1,2)"), string("expression \"abs(1,2)\": 'abs' takes 1 value"));
  for (const char* gone : {"floor", "ceil", "round", "sqrt", "log10", "pow"})
    EQ(err(string(gone) + "(1)"), string("expression \"") + gone + "(1)\": unknown function '" + gone + "'");
  EQ(EXPR::eval("max(k)", v), 2.0);
  EQ(err("1+"), string("expression \"1+\": a value was expected"));
  EQ(err("(1+2"), string("expression \"(1+2\": ')' is missing"));
  EQ(err("1 2"), string("expression \"1 2\": unexpected '2'"));
  EQ(err("x+1"), string("expression \"x+1\": unknown variable 'x'"));
  EQ(err("foo(1)"), string("expression \"foo(1)\": unknown function 'foo'"));
  EQ(err("1/0"), string("expression \"1/0\": division by zero"));
  EQ(err("1/(w-90)", v), string("expression \"1/(w-90)\": division by zero"));
  EQ(err("1234567890123456"), string("expression \"1234567890123456\": the number has too many digits"));
  EQ(err(""), string("expression \"\": a value was expected"));
  EQ(err("."), string("expression \".\": a number was expected"));
  EXPR::Vars d{{"w", 90}, {"yt", 10}, {"yb", 20}, {"ry", 14}};
  EQ(EXPR::fill("M0,yt L0,yb A w/2,ry 0 0 0 w,yb L w,yt Z", d, num), string("M0,10 L0,20 A 45,14 0 0 0 90,20 L 90,10 Z"));
  EQ(EXPR::fill("1 2 3", {}, num), string("1 2 3"));
  EQ(EXPR::fill("M w*0.22,h*0.86", {{"w", 100}, {"h", 50}}, num), string("M 22,43"));
  EQ(EXPR::fill("x0,0 x0+step*0.9,h/2", {{"x0", 3}, {"step", 10}, {"h", 36}}, num), string("3,0 12,18"));
  EQ(EXPR::fill("M p,min(1,2) H p+bw", {{"p", 8}, {"bw", 4}}, num), string("M 8,1 H 12"));
  EQ(EXPR::fill("", {}, num), string(""));
  try { EXPR::fill("M q", {}, num); cout << "FAIL no throw\n"; fails++; } catch (const EXPR::Error&) {}
  cout << (fails ? "FAILED" : "all tests passed") << endl;
  return fails;
}
