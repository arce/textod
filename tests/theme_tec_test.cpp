#include "theme_tec.hpp"
#include <iostream>
#include <set>
using namespace std;
static int fails = 0;
#define EQ(a, b) do { auto x = (a); auto y = (b); if (!(x == y)) { cout << "FAIL line " << __LINE__ << ": " #a "\n--- got\n" << x << "\n--- expected\n" << y << "\n"; fails++; } } while (0)
int main() {
  EQ(THEME::color("primary"), string("#2c81ba"));
  EQ(THEME::color("light"), string("#dbe8f5"));
  EQ(THEME::color("#AbCdEf"), string("#AbCdEf"));
  EQ(THEME::color("none"), string("none"));
  EQ(THEME::color("#abc"), string(""));
  EQ(THEME::color("primay"), string(""));
  EQ(THEME::cls("primary", "border"), string("c-f-primary c-s-border"));
  EQ(THEME::cls("primary"), string("c-f-primary"));
  EQ(THEME::cls("none", "border"), string("c-s-border"));
  EQ(THEME::cls("#ffffff", "none"), string(""));
  EQ(THEME::names().size(), (size_t)29);
  set<string> uniq;
  for (auto& n : THEME::names()) uniq.insert(n);
  EQ(uniq.size(), (size_t)29);
  for (auto& e : THEME::PALETTE) {
    EQ(THEME::isHex(e.color), true);
  }
  string c = THEME::css();
  EQ(c.find("svg { font-family: var(--c-font, Helvetica, Arial, sans-serif); }\n"), (size_t)0);
  EQ(c.find(".c-f-primary { fill: var(--c-primary, #2c81ba); }\n") != string::npos, true);
  EQ(c.find(".c-s-accent { stroke: var(--c-accent, #e67e22); }\n") != string::npos, true);
  EQ(c.find("--c-primary:") == string::npos, true);
  cout << (fails ? "FAILED" : "all tests passed") << endl;
  return fails;
}
