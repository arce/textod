#include "connect_tec.hpp"
#include <iostream>
using namespace std;
using namespace CONNECT;
static int fails = 0;
static void fail(int line, const string& what) { cout << "FAIL line " << line << ": " << what << "\n"; fails++; }
template <class A, class B> static void eq(int line, const A& x, const B& y) { if (!(x == y)) fail(line, "values differ"); }
static void pt(int line, Pt p, double x, double y) {
  if (fabs(p.x - x) > 1e-9 || fabs(p.y - y) > 1e-9) fail(line, "got (" + to_string(p.x) + "," + to_string(p.y) + ") expected (" + to_string(x) + "," + to_string(y) + ")");
}
#define EQ(...) eq(__LINE__, __VA_ARGS__)
#define PT(...) pt(__LINE__, __VA_ARGS__)
int main() {
  Rect r{10, 20, 100, 40};
  PT(point(r, {"top", 0.5}), 60, 20);
  PT(point(r, {"bottom", 0.12}), 22, 60);
  PT(point(r, {"left", 0.25}), 10, 30);
  PT(point(r, {"right", 1}), 110, 60);
  try { point(r, {"middle", 0.5}); fail(__LINE__, "no throw"); }
  catch (const invalid_argument& e) { EQ(string(e.what()), string("anchor: unknown side 'middle' (use top, bottom, left or right)")); }

  EQ(facing({0, 0, 10, 10}, {50, 0, 10, 10}).a, string("right"));
  EQ(facing({50, 0, 10, 10}, {0, 0, 10, 10}).a, string("left"));
  EQ(facing({0, 0, 10, 10}, {5, 100, 10, 10}).b, string("top"));
  EQ(facing({5, 100, 10, 10}, {0, 0, 10, 10}).a, string("top"));
  EQ(facing({0, 0, 10, 10}, {50, 100, 10, 10}).a, string("bottom"));
  EQ(facing({0, 0, 100, 100}, {50, 60, 100, 100}).a, string("bottom"));
  EQ(facing({0, 0, 100, 100}, {60, 50, 100, 100}).a, string("right"));

  Rect a{0, 0, 100, 40}, b{200, 0, 100, 40};
  Route s = route(a, {}, b, {});
  EQ(s.line.size(), (size_t)2);
  PT(s.line[0], 100, 20);
  PT(s.line[1], 190.65, 20);
  EQ(s.head.size(), (size_t)3);
  PT(s.head[0], 200, 20);
  PT(s.head[1], 189, 25.06);
  PT(s.head[2], 189, 14.94);
  PT(s.mid, 150, 20);
  EQ(s.flat, true);

  Opts el;
  el.elbow = true;
  Route flat = route(a, {}, b, {}, el);
  EQ(flat.line.size(), (size_t)3);

  Rect c{200, 100, 100, 40};
  Route z = route(a, {}, c, {}, el);
  EQ(z.line.size(), (size_t)4);
  PT(z.line[1], 150, 20);
  PT(z.line[2], 150, 120);
  PT(z.line[3], 190.65, 120);
  EQ(z.flat, false);

  Route l = route(a, {"bottom", 0.5}, c, {"left", 0.5}, el);
  EQ(l.line.size(), (size_t)3);
  PT(l.line[1], 50, 120);

  Route l2 = route(a, {"right", 0.5}, c, {"top", 0.5}, el);
  EQ(l2.line.size(), (size_t)3);
  PT(l2.line[1], 250, 20);

  Rect d{0, 100, 100, 40};
  Route u = route(a, {"right", 0.5}, d, {"right", 0.5}, el);
  EQ(u.line.size(), (size_t)4);
  PT(u.line[1], 126, 20);
  PT(u.line[2], 126, 120);
  PT(u.head[0], 100, 120);
  EQ(route(a, {"left", 0.5}, d, {"left", 0.5}, el).line[1].x, -26.0);
  Rect e{300, 0, 100, 40};
  EQ(route(a, {"top", 0.5}, e, {"top", 0.5}, el).line[1].y, -26.0);
  EQ(route(a, {"bottom", 0.5}, e, {"bottom", 0.5}, el).line[1].y, 66.0);

  Rect up{300, -100, 100, 40};
  Route w = route(a, {"bottom", 0.5}, up, {"left", 0.5}, el);
  EQ(w.line.size(), (size_t)5);
  PT(w.line[1], 50, 66);
  PT(w.line[2], 274, 66);
  PT(w.line[3], 274, -80);

  Rect back{-300, 100, 100, 40};
  Route wr = route(a, {"right", 0.5}, back, {"left", 0.5}, el);
  EQ(wr.line.size(), (size_t)6);
  PT(wr.line[1], 126, 20);
  PT(wr.line[2], 126, 70);
  PT(wr.line[3], -326, 70);

  Opts bo;
  bo.both = true;
  Route bt = route(a, {}, b, {}, bo);
  EQ(bt.head.size(), (size_t)3);
  EQ(bt.head2.size(), (size_t)3);
  PT(bt.head2[0], 100, 20);
  PT(bt.line[0], 109.35, 20);
  PT(bt.line[1], 190.65, 20);
  EQ(bt.flat, true);

  Opts thick;
  thick.sw = 8;
  Route th = route(a, {}, b, {}, thick);
  PT(th.head[0], 200, 20);
  PT(th.head[1], 200 - 26.4, 20 + 26.4 * 0.46);
  PT(th.line[1], 200 - 26.4 * 0.85, 20);

  Rect tiny{0, 0, 10, 10}, near2{12, 0, 10, 10};
  Route sh = route(tiny, {}, near2, {});
  EQ(sh.line.size(), (size_t)2);
  EQ(sh.line[1].x > sh.line[0].x, true);

  Route diag = route(a, {"bottom", 0.5}, c, {"top", 0.5}, el);
  EQ(diag.line.size(), (size_t)4);
  PT(diag.line[1], 50, 70);
  PT(diag.line[2], 250, 70);
  // a route around obstacles: three screens in a row, the arrow goes from the first to the third without crossing the second
  Rect s1{0, 0, 100, 80}, s2{140, 0, 100, 80}, s3{280, 0, 100, 80};
  std::vector<Rect> screens{s1, s2, s3};
  Opts av;
  av.obstacles = &screens;
  Route rt = route(s1, {"right", 0.5}, s3, {"left", 0.5}, av);
  EQ(rt.line.size() >= 4, true);
  bool crosses = false;
  for (size_t i = 1; i < rt.line.size(); i++) {
    double x0 = std::min(rt.line[i - 1].x, rt.line[i].x), x1 = std::max(rt.line[i - 1].x, rt.line[i].x);
    double y0 = std::min(rt.line[i - 1].y, rt.line[i].y), y1 = std::max(rt.line[i - 1].y, rt.line[i].y);
    if (x1 > s2.x + 1.5 && x0 < s2.x + s2.w - 1.5 && y1 > s2.y + 1.5 && y0 < s2.y + s2.h - 1.5) crosses = true;
  }
  EQ(crosses, false);
  EQ(rt.line.size() >= 2 && !rt.cands.empty(), true);
  PT(rt.line.front(), 100, 40);
  cout << (fails ? "FAILED" : "all tests passed") << endl;
  return fails;
}
