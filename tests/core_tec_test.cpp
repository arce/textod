#include <iostream>
#include "figure_tec.hpp"
using namespace std;
static int fails = 0;
#define EQ(a, b) do { auto x = (a); auto y = (b); if (!(x == y)) { cout << "FAIL line " << __LINE__ << ": " #a "\n--- got\n" << x << "\n--- expected\n" << y << "\n"; fails++; } } while (0)

static string one(const string& src) {
  try { return FIGURE::render(src)[0].svg; } catch (const DSL::Error& e) { return e.what(); }
}

int main() {
  EQ(one("diagram\n  box a \"A\" w=40 h=40\n").find("<rect") != string::npos, true);
  EQ(one("diagram\n  graph\n    a -> b\n"), string("line 2: unknown element 'graph'"));
  EQ(one("diagram\n  chart\n    bar data=x\n"), string("line 2: unknown element 'chart'"));
  EQ(FIGURE::ATTRS_CORE.size() + FIGURE::ATTRS_SHAPE.size() + FIGURE::ATTRS_EXT.size(), FIGURE::ATTRS.size());
  EQ(one("diagram\n  box a \"A\" serif\n").find("unknown word 'serif'") != string::npos, true);
  EQ(one("diagram\n  box a \"A\" zzz=1\n").find("unknown attribute 'zzz'") != string::npos, true);
  cout << (fails ? "FAILED" : "all tests passed") << endl;
  return fails;
}
