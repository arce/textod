// The plain level: dsl_tec.hpp without the expander (what textod links).
#include <iostream>
#include "dsl_tec.hpp"
using namespace std;
static int fails = 0;
#define EQ(a, b) do { auto x = (a); auto y = (b); if (!(x == y)) { cout << "FAIL line " << __LINE__ << ": " #a "\n--- got\n" << x << "--- expected\n" << y << "\n"; fails++; } } while (0)
static string err(const string& t, DSL::Resolver r = nullptr) {
  try { DSL::compile(t, r); } catch (const DSL::Error& e) { return e.what(); }
  return "ok";
}
int main() {
  DSL::Resolver lib = [](const string& n) -> string {
    if (n == "styles") return "style a\n  box sw=1\n";
    if (n == "more") return "import styles\nstyle b\n  box sw=2\n";
    if (n == "comp") return "def p\n  box\n";
    return "";
  };
  // the text goes through as it is
  EQ(DSL::show(DSL::compile("diagram d\n  box a \"x y\"\n  a -> b\n")), string("diagram d\n  box a \"x y\"\n  a -> b\n"));
  // an import of styles is accepted, and its styles come first (also when the library imports another)
  EQ(DSL::show(DSL::compile("import more\nimport styles\ndiagram d\n  box a\n", lib)), string("style a\n  box sw=1\nstyle b\n  box sw=2\ndiagram d\n  box a\n"));
  // what only the expander does is an error that says so
  EQ(err("def p\n  box\n"), string("line 1: 'def' needs the expander module, which is not loaded"));
  EQ(err("diagram d\n  repeat n=2\n    box\n"), string("ok"));  // the plain level does not look inside a diagram: textod rejects it when it draws
  EQ(err("data t a\n  1\n"), string("line 1: 'data' needs the expander module, which is not loaded"));
  EQ(err("import comp\n", lib), string("line 1: import 'comp': the library has more than styles, it needs the expander module, which is not loaded"));
  EQ(err("import nada\n", lib), string("line 1: import 'nada': no such library"));
  // #line N
  auto n = DSL::parse("a\n#line 10\nb\nc\n#line 3\nd");
  EQ(to_string(n.kids[0].line) + "," + to_string(n.kids[1].line) + "," + to_string(n.kids[2].line) + "," + to_string(n.kids[3].line), string("1,10,11,3"));
  if (!fails) cout << "all tests passed\n";
  return fails;
}
