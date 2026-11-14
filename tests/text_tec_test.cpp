#include "text_tec.hpp"
#include <iostream>
using namespace std;
static int fails = 0;
#define EQ(a, b) do { auto x = (a); auto y = (b); if (!(x == y)) { cout << "FAIL line " << __LINE__ << ": " #a "\n"; fails++; } } while (0)
static string J(const vector<string>& v) { string r; for (auto& s : v) r += (r.empty() ? "" : "|") + s; return r; }
int main() {
  EQ(TEXT::ulen(""), 0);
  EQ(TEXT::ulen("abc"), 3);
  EQ(TEXT::ulen("a\xC3\xA9" "b"), 3);
  EQ(TEXT::ulen("\xE4\xB8\xAD"), 1);
  EQ(TEXT::ulen("\xF0\x9F\x98\x80"), 1);
  EQ(TEXT::ulen("\xFF\xFE"), 2);
  EQ(TEXT::ulen("\xC3"), 1);
  EQ(TEXT::units("Hello"), 722 + 556 + 222 + 222 + 556);
  EQ(TEXT::units("\xE4\xB8\xAD"), 1000);
  EQ(TEXT::units("\xC3\xA9"), 556);
  EQ(TEXT::units("a", true), 595);
  EQ(TEXT::width("Hello", 10), 22.78);
  EQ(J(TEXT::wrap("", 100, 10)), string(""));
  EQ(J(TEXT::wrap("aa bb cc", 1000, 10)), string("aa bb cc"));
  EQ(J(TEXT::wrap("aa  bb\ncc", 1000, 10)), string("aa bb|cc"));
  EQ(J(TEXT::wrap("aa\n\ncc", 1000, 10)), string("aa||cc"));
  EQ(TEXT::wrap("hello world foo", 40, 10).size(), (size_t)3);
  EQ(J(TEXT::wrap("hello world", 40, 10)), string("hello|world"));
  auto w = TEXT::wrap("abcdefghijkl", 30, 10);
  EQ(J(w), string("abcde|fghijkl"));
  for (auto& l : TEXT::wrap("the quick brown fox jumps over the lazy dog", 80, 10)) EQ(TEXT::width(l, 10) <= 80, true);
  cout << (fails ? "FAILED" : "all tests passed") << endl;
  return fails;
}
