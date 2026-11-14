#include <iostream>
#include "draw_tec.hpp"
using namespace std;
static int fails = 0;
template <class A, class B> static void eq(int line, const A& x, const B& y) { if (!(x == y)) { cout << "FAIL line " << line << "\n--- got\n" << x << "\n--- expected\n" << y << "\n"; fails++; } }
#define EQ(...) eq(__LINE__, __VA_ARGS__)
#define R(s) string(s)

struct Doc {
  DSL::Node tree;
  FIG::Style st;
  FIG::Item root;
  Doc(const string& text, const string& style = "") {
    tree = DSL::compile(text);
    st = FIG::style(tree, style);
    root.kind = "canvas";
    for (auto& n : tree.kids) if (n.head != "style") root.kids.push_back(FIG::build(n, st));
    for (auto& k : root.kids) place(k);
  }
  static void place(FIG::Item& i) {
    auto at = i.attr("at");
    if (!at.empty()) { auto c = at.find(','); i.x = EXPR::eval(at.substr(0, c)); i.y = EXPR::eval(at.substr(c + 1)); }
    i.w = i.num("w", 100);
    i.h = i.num("h", 40);
    for (auto& k : i.kids) { k.x = i.x; k.y = i.y; k.w = i.w; k.h = i.h; }
  }
  FIG::Item& at(size_t i) { return root.kids[i]; }
};

static string draw(const FIG::Item& it) {
  SVG::Svg s(300, 200);
  DRAW::item(s, it);
  return s.toString();
}

static string run(int n) {
  switch (n) {
    case 0: { Doc d("box a \"Hola\" light at=10,20 w=110 h=44\n"); return draw(d.at(0)); }
    case 1: { Doc d("box b \"Dark\" dark at=0,0 w=50 h=30 sw=3 radius=0\n"); return draw(d.at(0)); }
    case 2: { Doc d("box c at=5,5 w=20 h=20 fill=#ff8800\n"); return draw(d.at(0)); }
    case 3: { Doc d("text t \"Uno\\nDos\" bold left at=10,10 w=100 h=40\n"); return draw(d.at(0)); }
    case 4: { Doc d("shape s \"X\" fill=dark w=90 h=62 at=5,5\n  set ry=min(14,w*0.15)\n  rect fill=self sw=1.6\n  ellipse cy=ry rx=w/2 ry=ry fill=lid\n  path d=\"M0,ry L w/2,h A 4,4 0 0 1 w,ry Z\" fill=none sw=2\n  line x1=1 x2=w-1 y1=h/2 y2=h/2 dash=\"0 5\" linecap=round\n  poly pts=\"0,0 w,0 w,h\" open\n"); return draw(d.at(0)); }
    case 5: { Doc d("group g \"Titulo\" pad=10 at=0,0 w=200 h=100 caption=\"Figura\" captionstyle=italic\n  box in \"A\" at=10,30 w=40 h=30\n"); return draw(d.at(0)); }
    case 6: { Doc d("box a \"A\" at=0,0 w=100 h=40\nbox b \"B\" at=200,100 w=100 h=40\narrow x label=\"Escribe\\ndatos\" elbow sw=3 color=primary\n"); SVG::Svg sv(300, 200); DRAW::arrow(sv, {&d.at(0), "", 0.5}, {&d.at(1), "", 0.5}, d.at(2)); return sv.toString(); }
  }
  return "";
}

int main() {
  static const char* golden[] = {
    R"X(<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="300" height="200">
  <rect x="10" y="20" width="110" height="44" rx="6" ry="6" class="c-f-light c-s-text" fill="#dbe8f5" stroke="#2c3e50" stroke-width="1.6"/>
  <text x="65" y="47.6" font-family="Helvetica,Arial,sans-serif" font-size="16" text-anchor="middle" class="c-f-text" fill="#2c3e50">Hola</text>
</svg>)X",
    R"X(<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="300" height="200">
  <rect x="0" y="0" width="50" height="30" class="c-f-dark c-s-text" fill="#2c81ba" stroke="#2c3e50" stroke-width="3"/>
  <text x="25" y="20.6" font-family="Helvetica,Arial,sans-serif" font-size="16" text-anchor="middle" class="c-f-white" fill="#ffffff">Dark</text>
</svg>)X",
    R"X(<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="300" height="200">
  <rect x="5" y="5" width="20" height="20" rx="6" ry="6" class="c-s-text" fill="#ff8800" stroke="#2c3e50" stroke-width="1.6"/>
</svg>)X",
    R"X(<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="300" height="200">
  <text x="10" y="25.6" font-family="Helvetica,Arial,sans-serif" font-size="16" text-anchor="start" font-weight="bold" class="c-f-text" fill="#2c3e50">Uno</text>
  <text x="10" y="45.6" font-family="Helvetica,Arial,sans-serif" font-size="16" text-anchor="start" font-weight="bold" class="c-f-text" fill="#2c3e50">Dos</text>
</svg>)X",
    R"X(<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="300" height="200">
  <g transform="translate(5,5)">
  <rect x="0" y="0" width="90" height="62" class="c-f-dark c-s-text" fill="#2c81ba" stroke="#2c3e50" stroke-width="1.6"/>
  <ellipse cx="45" cy="13.5" rx="45" ry="13.5" class="c-s-text" fill="#80b3d5" stroke="#2c3e50" stroke-width="1.6"/>
  <path d="M0,13.5 L 45,62 A 4,4 0 0 1 90,13.5 Z" class="c-s-text" fill="none" stroke="#2c3e50" stroke-width="2"/>
  <line x1="1" y1="31" x2="89" y2="31" class="c-s-text" stroke="#2c3e50" stroke-width="1.6" stroke-dasharray="0 5" stroke-linecap="round"/>
  <polyline points="0,0 90,0 90,62" class="c-s-text" fill="none" stroke="#2c3e50" stroke-width="1.6"/>
  <text x="45" y="36.6" font-family="Helvetica,Arial,sans-serif" font-size="16" text-anchor="middle" class="c-f-white" fill="#ffffff">X</text>
</g>
</svg>)X",
    R"X(<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="300" height="200">
  <rect x="0" y="0" width="200" height="100" rx="8" ry="8" class="c-f-light c-s-text" fill="#dbe8f5" stroke="#2c3e50" stroke-width="1.6"/>
  <text x="100" y="25.6" font-family="Helvetica,Arial,sans-serif" font-size="16" text-anchor="middle" font-weight="bold" class="c-f-text" fill="#2c3e50">Titulo</text>
  <text x="100" y="121.6" font-family="Helvetica,Arial,sans-serif" font-size="16" text-anchor="middle" font-style="italic" class="c-f-text" fill="#2c3e50">Figura</text>
  <rect x="0" y="0" width="200" height="100" rx="6" ry="6" class="c-f-white c-s-text" fill="#ffffff" stroke="#2c3e50" stroke-width="1.6"/>
  <text x="100" y="55.6" font-family="Helvetica,Arial,sans-serif" font-size="16" text-anchor="middle" class="c-f-text" fill="#2c3e50">A</text>
</svg>)X",
    R"X(<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" width="300" height="200">
  <path d="M 100 20 L 150 20 L 150 120 L 190.65 120" class="c-s-primary" fill="none" stroke="#2c81ba" stroke-width="3"/>
  <polygon points="200,120 189,125.06 189,114.94" class="c-f-primary" fill="#2c81ba" stroke="none"/>
  <text x="157" y="65.6" font-family="Helvetica,Arial,sans-serif" font-size="16" text-anchor="start" class="c-f-primary" fill="#2c81ba" stroke="#ffffff" stroke-width="4" stroke-linejoin="round" paint-order="stroke">Escribe</text>
  <text x="157" y="85.6" font-family="Helvetica,Arial,sans-serif" font-size="16" text-anchor="start" class="c-f-primary" fill="#2c81ba" stroke="#ffffff" stroke-width="4" stroke-linejoin="round" paint-order="stroke">datos</text>
</svg>)X",
  };
  for (int i = 0; i < 7; i++) EQ(run(i), string(golden[i]));

  EQ(Doc("box a \"A\" at=0,0 w=10 h=10 sw=7 radius=0\n", "").at(0).attr("sw"), string("7"));
  {
    Doc d("style k\n  box radius=0 sw=3 dark\nstyle k\n  box padx=9\nbox a \"A\" at=0,0 w=60 h=30\nbox b \"B\" at=0,0 w=60 h=30 sw=1\n", "k");
    string s = draw(d.at(0)), t = draw(d.at(1));
    EQ(s.find(" rx=") == string::npos, true);
    EQ(s.find("stroke-width=\"3\"") != string::npos, true);
    EQ(s.find("fill=\"#2c81ba\"") != string::npos, true);
    EQ(t.find("stroke-width=\"1\"") != string::npos, true);
    EQ(d.at(0).attr("padx"), string("9"));
  }
  {
    Doc d("box ord \"Order\" w=1\nbox light \"x\"\nbox \"y\"\nbox z1 light bold \"t\"\ntext \"solo\"\nrow gap=3\n");
    EQ(d.at(0).id, string("ord"));
    EQ(d.at(0).text, string("Order"));
    EQ(d.at(1).id, string(""));
    EQ(d.at(1).fillName(), string("light"));
    EQ(d.at(2).id, string(""));
    EQ(d.at(2).text, string("y"));
    EQ(d.at(3).id, string("z1"));
    EQ(d.at(3).fillName(), string("light"));
    EQ(d.at(3).flag("bold"), true);
    EQ(d.at(3).flag("z1"), false);
    EQ(d.at(4).id, string(""));
    EQ(d.at(5).num("gap", 0), 3.0);
    EQ(d.at(0).num("w", 0), 1.0);
    EQ(d.at(0).num("h", 7), 7.0);
  }
  {
    Doc d("diagram x size=11\n  box a \"A\" size=20\n  box b \"B\"\n");
    EQ(d.at(0).fs, 11.0);
    EQ(d.at(0).kids[0].fs, 20.0);
    EQ(d.at(0).kids[1].fs, 11.0);
  }
  {
    Doc d("box a \"Hola\\nmundo largo\" mono\nbox b \"\" \nbox c \"uno dos tres cuatro\"\n");
    FIG::Block m = FIG::block(d.at(0), d.at(0).text, 10, false);
    EQ(m.lines.size(), (size_t)2);
    EQ(m.lh, 12.5);
    EQ(m.h, 25.0);
    EQ(m.w, TEXT::width("mundo largo", 10, false) * 1.1);
    EQ(FIG::block(d.at(1), "", 10, false).lines.size(), (size_t)0);
    FIG::Block w = FIG::block(d.at(2), d.at(2).text, 10, false, 40);
    EQ(w.lines.size(), (size_t)3);
    EQ(w.lines[0], string("uno dos"));
  }
  {
    Doc d("box a \"A\" at=0,0 w=10 h=10\nshape s at=0,0 w=10 h=10\n  bezier d=\"M0,0\"\n");
    try { draw(d.at(1)); EQ(false, true); } catch (const DSL::Error& e) { EQ(string(e.what()), string("line 2: 'bezier' cannot go inside a shape")); }
    Doc e("box a \"A\" fill=lihgt at=0,0 w=10 h=10\n");
    try { draw(e.at(0)); EQ(false, true); } catch (const DSL::Error& x) { EQ(string(x.what()), string("line 1: unknown color 'lihgt' (did you mean 'light'?)")); }
  }
  {
    Doc d("text t \"aa bb cc\" wrap left at=0,0 w=40 h=40\n");
    string s = draw(d.at(0));
    EQ(s.find("<text") != string::npos && s.find(">aa<") != string::npos && s.find(">bb cc<") != string::npos, true);
  }
  cout << (fails ? "FAILED" : "all tests passed") << endl;
  return fails;
}
