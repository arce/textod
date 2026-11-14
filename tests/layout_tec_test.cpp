#include <iostream>
#include "layout_tec.hpp"
#include "svg_tec.hpp"
using namespace std;
static int fails = 0;
template <class A, class B> static void eq(int line, const A& x, const B& y) { if (!(x == y)) { cout << "FAIL line " << line << "\n--- got\n" << x << "\n--- expected\n" << y << "\n"; fails++; } }
#define EQ(...) eq(__LINE__, __VA_ARGS__)
#define R(s) string(s)

static void dump(const FIG::Item& it, string& out) {
  out += (it.id.empty() ? it.kind : it.id) + ":" + SVG::num(it.x) + "," + SVG::num(it.y) + "," + SVG::num(it.w) + "," + SVG::num(it.h) + " ";
  for (auto& k : it.kids) if (!k.arrow()) dump(k, out);
}

static string lay(const string& text, double x = 0, double y = 0) {
  DSL::Node tree = DSL::compile(text);
  FIG::Style st = FIG::style(tree, "");
  FIG::Item root = FIG::build(tree.kids[0], st);
  string out;
  try {
    LAYOUT::run(root, x, y);
    dump(root, out);
  } catch (const DSL::Error& e) { return e.what(); }
  return out;
}

int main() {
  EQ(lay("col gap=0 h=100 w=50\n  spacer a w=10 h=10\n  spacer b flex\n  spacer c flex=3\n"), R("col:0,0,50,100 a:20,0,10,10 b:25,10,0,22.5 c:25,32.5,0,67.5 "));
  EQ(lay("col gap=0 h=100 w=50\n  spacer a w=10 h=10 flex\n  spacer b flex\n"), R("col:0,0,50,100 a:20,0,10,10 b:25,10,0,90 "));
  EQ(lay("col gap=0 h=100 w=50 weights=1,3\n  spacer a w=10 h=5\n  spacer b w=10 h=5\n"), R("col:0,0,50,100 a:20,0,10,25 b:20,25,10,75 "));
  EQ(lay("row gap=0 w=100\n  spacer a w=10 h=10\n  spacer b flex\n  spacer c flex=3\n"), R("row:0,0,100,10 a:0,0,10,10 b:10,5,22.5,0 c:32.5,5,67.5,0 "));
  EQ(lay("row gap=0 w=100\n  spacer a w=10 h=10 flex=0\n  spacer b flex\n"), R("row:0,0,100,10 a:0,0,10,10 b:10,5,90,0 "));
  EQ(lay("row gap=10\n  spacer a w=40 h=20\n  spacer b w=60 h=30\n  spacer c w=20 h=10\n"),
     R("row:0,0,140,30 a:0,5,40,20 b:50,0,60,30 c:120,10,20,10 "));
  EQ(lay("col gap=5 align=center\n  spacer a w=40 h=20\n  spacer b w=60 h=30\n  spacer c w=20 h=10\n", 100, 50),
     R("col:100,50,60,70 a:110,50,40,20 b:100,75,60,30 c:120,110,20,10 "));
  EQ(lay("col gap=0 w=100\n  spacer a w=40 h=10\n  spacer b stretch w=20 h=10\n"),
     R("col:0,0,100,20 a:30,0,40,10 b:0,10,100,10 "));
  EQ(lay("row align=end pad=4\n  spacer a w=10 h=10\n  spacer b w=10 h=30\n"),
     R("row:0,0,40,38 a:4,24,10,10 b:26,4,10,30 "));
  EQ(lay("row gap=10\n  spacer a w=10 h=10; -> \"p\"; spacer b w=10 h=10\n"), R("row:0,0,30,10 a:0,0,10,10 b:20,0,10,10 "));
  EQ(lay("group \"Titulo\"\n  spacer a w=40 h=20\n"), R("group:0,0,69.87,78 a:14,44,40,20 "));
  EQ(lay("group \"Titulo\" layout=row gap=4\n  spacer a w=40 h=20\n  spacer b w=40 h=20\n"), R("group:0,0,112,72 a:14,38,40,20 b:58,38,40,20 "));
  EQ(lay("grid cols=2 gap=10\n  spacer a w=40 h=20\n  spacer b w=60 h=30\n  spacer c span=2 w=150 h=10\n  spacer d w=20 h=10\n"),
     R("grid:0,0,150,70 a:0,0,40,20 b:50,0,60,30 c:0,40,150,10 d:0,60,20,10 "));
  EQ(lay("layer w=200 h=100\n  spacer a x=10% y=20 w=50% h=50%\n", 5, 5), R("layer:5,5,200,100 a:25,25,100,50 "));
  EQ(lay("layer\n  spacer a w=30 h=10\n  spacer b w=10 h=40\n"), R("layer:0,0,30,40 a:0,0,30,10 b:0,0,10,40 "));
  EQ(lay("canvas\n  spacer a at=0,90 w=110 h=44\n  spacer b alignx=a at=0,10 w=50 h=20\n  spacer c aligny=a at=200,0 w=10 h=10\n"),
     R("canvas:0,0,210,134 a:0,90,110,44 b:30,10,50,20 c:200,107,10,10 "));
  EQ(lay("canvas pad=5\n  spacer a at=10,10 w=20 h=20\n", 100, 0), R("canvas:100,0,40,40 a:115,15,20,20 "));
  EQ(lay("col align=stretch w=100 gap=0\n  spacer a w=40 h=10\n  spacer b w=20 h=10\n"), R("col:0,0,100,20 a:0,0,100,10 b:0,10,100,10 "));
  EQ(lay("grid cols=2 gap=0 align=stretch\n  spacer a w=40 h=20\n  spacer b w=10 h=10\n  spacer c w=10 h=30\n"),
     R("grid:0,0,50,50 a:0,0,40,20 b:40,0,10,20 c:0,20,40,30 "));
  EQ(lay("row\n  canvas\n    spacer a at=3,4 w=10 h=10\n  spacer b pad=0 w=5 h=5\n", 10, 10),
     R("row:10,10,30,14 canvas:10,10,13,14 a:13,14,10,10 b:35,14.5,5,5 "));
  EQ(lay("col w=100\n  box a \"A\"\n  spacer s w=20 h=10\n"), R("col:0,0,100,58 a:0,0,100,36 s:40,48,20,10 "));
  EQ(lay("grid cols=2 gap=0 align=center\n  spacer a w=40 h=20\n  spacer b w=20 h=10\n"), R("grid:0,0,60,20 a:0,0,40,20 b:40,5,20,10 "));
  EQ(lay("grid cols=2 gapx=10 gapy=0\n  spacer a w=40 h=20\n  spacer b w=20 h=10\n  spacer c w=40 h=20\n"), R("grid:0,0,70,40 a:0,0,40,20 b:50,0,20,10 c:0,20,40,20 "));
  EQ(lay("row weights=1,3 gap=0 w=100\n  spacer a w=10 h=10 grow\n  spacer b w=10 h=10 grow\n"), R("row:0,0,100,10 a:0,0,25,10 b:25,0,75,10 "));
  EQ(lay("row gap=0 w=100\n  spacer a w=10 h=10\n  spacer s grow\n  spacer b w=10 h=10\n"), R("row:0,0,100,10 a:0,0,10,10 s:10,5,80,0 b:90,0,10,10 "));
  EQ(lay("col h=100 align=center\n  spacer a w=40 h=20\n"), R("col:0,0,40,100 a:0,40,40,20 "));
  EQ(lay("box a \"A\"\n"), R("a:0,0,38.67,36 "));
  EQ(lay("text t \"Hola\"\n"), R("t:0,0,32.9,20 "));
  EQ(lay("box a \"A\\nBB\"\n"), R("a:0,0,49.34,56 "));
  EQ(lay("text t \"aa bb cc\" wrap w=40\n"), R("t:0,0,40,40 "));
  EQ(lay("box a \"A\" minw=80 h=30\n"), R("a:0,0,80,36 "));
  EQ(lay("spacer s w=7\n"), R("s:0,0,7,0 "));
  EQ(lay("canvas\n  spacer a alignx=zz w=1 h=1\n"), R("line 2: alignx: there is no element 'zz' before this one"));
  EQ(lay("canvas\n  spacer alpha w=1 h=1\n  box b alignx=alpa\n"), R("line 3: alignx: there is no element 'alpa' before this one (did you mean 'alpha'?)"));
  EQ(lay("canvas\n  box b alignx=a\n  spacer a w=1 h=1\n"), R("line 2: alignx: there is no element 'a' before this one"));
  EQ(lay("col align=middlee\n  spacer a w=1 h=1\n"), R("line 1: align: 'middlee' is not start, center, end or stretch"));
  EQ(lay("canvas\n  spacer a at=3 w=1 h=1\n"), R("line 2: at: x,y was expected"));
  EQ(lay("spacer a w=3+\n").substr(0, 11), R("line 1: w: "));
  cout << (fails ? "FAILED" : "all tests passed") << endl;
  return fails;
}
