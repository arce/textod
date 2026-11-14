#pragma once
#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace CONNECT {

struct Pt {
  double x = 0, y = 0;
};

struct Rect {
  double x = 0, y = 0, w = 0, h = 0;
};

struct Anchor {
  std::string side;
  double at = 0.5;
};

struct Opts {
  bool elbow = false, head = true, both = false, curve = false, trim = true;
  double sw = 2.5, margin = 26;
  std::string tip = "arrow", tail;
  std::vector<std::pair<Pt, Pt>>* used = nullptr;  // segments already taken by other detours: reusing them costs more
  size_t channels = static_cast<size_t>(-1);        // only the first `channels` obstacles shape the grid; the rest just block
  const std::vector<Rect>* obstacles = nullptr;  // when set, the line goes around these rectangles
};

struct Cand {
  Pt m;
  bool flat = true, left = false;
};

struct Route {
  std::vector<Cand> cands;  // other places for the label, best first (only for detours)
  std::vector<Pt> line, head, head2;
  std::string kind, kind2;
  Pt mid;
  bool flat = true, left = false, bezier = false;
};

struct Sides {
  std::string a, b;
};

inline bool horiz(const std::string& side) { return side == "left" || side == "right"; }

inline Pt point(const Rect& r, const Anchor& a) {
  if (a.side == "top") return {r.x + r.w * a.at, r.y};
  if (a.side == "bottom") return {r.x + r.w * a.at, r.y + r.h};
  if (a.side == "left") return {r.x, r.y + r.h * a.at};
  if (a.side == "right") return {r.x + r.w, r.y + r.h * a.at};
  throw std::invalid_argument("anchor: unknown side '" + a.side + "' (use top, bottom, left or right)");
}

inline Sides facing(const Rect& a, const Rect& b) {
  double hg = std::max(b.x - (a.x + a.w), a.x - (b.x + b.w));
  double vg = std::max(b.y - (a.y + a.h), a.y - (b.y + b.h));
  double dx = (b.x + b.w / 2) - (a.x + a.w / 2), dy = (b.y + b.h / 2) - (a.y + a.h / 2);
  bool h = hg > 0 && hg >= vg;
  if (!h && vg <= 0) h = std::fabs(dx) >= std::fabs(dy);
  if (h) return dx >= 0 ? Sides{"right", "left"} : Sides{"left", "right"};
  return dy >= 0 ? Sides{"bottom", "top"} : Sides{"top", "bottom"};
}

inline Pt dir(const std::string& s) { return s == "right" ? Pt{1, 0} : s == "left" ? Pt{-1, 0} : s == "bottom" ? Pt{0, 1} : Pt{0, -1}; }

inline std::vector<Pt> elbow(Pt p, const std::string& sa, Pt q, const std::string& sb, double margin) {
  bool ha = horiz(sa), hb = horiz(sb);
  if (ha && hb) {
    if (sa != sb) {
      if ((q.x - p.x) * dir(sa).x >= 0) return {p, {(p.x + q.x) / 2, p.y}, {(p.x + q.x) / 2, q.y}, q};
      double my = (p.y + q.y) / 2, x0 = p.x + dir(sa).x * margin, x1 = q.x + dir(sb).x * margin;
      return {p, {x0, p.y}, {x0, my}, {x1, my}, {x1, q.y}, q};
    }
    double x = sa == "right" ? std::max(p.x, q.x) + margin : std::min(p.x, q.x) - margin;
    return {p, {x, p.y}, {x, q.y}, q};
  }
  if (!ha && !hb) {
    if (sa != sb) {
      if ((q.y - p.y) * dir(sa).y >= 0) return {p, {p.x, (p.y + q.y) / 2}, {q.x, (p.y + q.y) / 2}, q};
      double mx = (p.x + q.x) / 2, y0 = p.y + dir(sa).y * margin, y1 = q.y + dir(sb).y * margin;
      return {p, {p.x, y0}, {mx, y0}, {mx, y1}, {q.x, y1}, q};
    }
    double y = sa == "bottom" ? std::max(p.y, q.y) + margin : std::min(p.y, q.y) - margin;
    return {p, {p.x, y}, {q.x, y}, q};
  }
  Pt c = ha ? Pt{q.x, p.y} : Pt{p.x, q.y}, da = dir(sa), db = dir(sb);
  if ((c.x - p.x) * da.x + (c.y - p.y) * da.y > 0 && (q.x - c.x) * db.x + (q.y - c.y) * db.y < 0) return {p, c, q};
  Pt p0{p.x + da.x * margin, p.y + da.y * margin}, q0{q.x + db.x * margin, q.y + db.y * margin};
  return {p, p0, ha ? Pt{p0.x, q0.y} : Pt{q0.x, p0.y}, q0, q};
}

inline std::vector<Pt> simplify(const std::vector<Pt>& in) {
  std::vector<Pt> r;
  for (auto& p : in) {
    if (!r.empty() && std::fabs(r.back().x - p.x) < 1e-9 && std::fabs(r.back().y - p.y) < 1e-9) continue;
    r.push_back(p);
  }
  return r;
}

inline double dist(Pt a, Pt b) { return std::hypot(b.x - a.x, b.y - a.y); }

inline bool diamond(const std::string& k) { return k == "diamond" || k == "diamondfill"; }

inline std::vector<Pt> headAt(Pt tip, Pt from, double len, const std::string& kind = "arrow") {
  double d = dist(from, tip);
  if (d == 0) return {};
  if (diamond(kind)) {
    double ux = (tip.x - from.x) / d, uy = (tip.y - from.y) / d, whole = len * 1.8, half = len * 0.5;
    Pt mid{tip.x - ux * whole / 2, tip.y - uy * whole / 2}, back{tip.x - ux * whole, tip.y - uy * whole};
    return {tip, {mid.x - uy * half, mid.y + ux * half}, back, {mid.x + uy * half, mid.y - ux * half}};
  }
  double ux = (tip.x - from.x) / d, uy = (tip.y - from.y) / d, half = len * 0.46;
  Pt base{tip.x - ux * len, tip.y - uy * len};
  return {tip, {base.x - uy * half, base.y + ux * half}, {base.x + uy * half, base.y - ux * half}};
}

inline void shorten(std::vector<Pt>& pts, double len) {
  Pt& e = pts.back();
  Pt o = pts[pts.size() - 2];
  double d = dist(o, e);
  if (d < len * 1.2) return;
  double k = len * 0.85 / d;
  e = {e.x + (o.x - e.x) * k, e.y + (o.y - e.y) * k};
}

inline Pt midpoint(const std::vector<Pt>& pts, bool& flat, bool& left) {
  size_t best = 1;
  for (size_t i = 2; i < pts.size(); i++) if (dist(pts[i - 1], pts[i]) > dist(pts[best - 1], pts[best])) best = i;
  if (pts.size() < 2) return pts.empty() ? Pt{} : pts.back();
  Pt a = pts[best - 1], b = pts[best];
  flat = std::fabs(b.x - a.x) >= std::fabs(b.y - a.y);
  left = !flat && best >= 2 && best + 1 < pts.size() && pts[best - 2].x > a.x && pts[best + 1].x > a.x;
  return {(a.x + b.x) / 2, (a.y + b.y) / 2};
}

// Orthogonal route from p (leaving by side sa) to q (arriving by side sb) that does not cross any obstacle.
// Dijkstra over the grid made by the channels between the obstacles; each bend costs extra.
inline std::vector<Pt> around(Pt p, const std::string& sa, Pt q, const std::string& sb, const std::vector<Rect>& obs, std::vector<std::pair<Pt, Pt>>* used = nullptr, size_t channels = static_cast<size_t>(-1)) {
  const double stub = 16, eps = 1.5, bendCost = 40;
  Pt p0{p.x + dir(sa).x * stub, p.y + dir(sa).y * stub}, q0{q.x + dir(sb).x * stub, q.y + dir(sb).y * stub};
  auto inside = [&](const Rect& r, double x, double y) { return x > r.x + eps && x < r.x + r.w - eps && y > r.y + eps && y < r.y + r.h - eps; };
  auto hit = [&](Pt a, Pt b) {
    for (auto& r : obs) {
      double x0 = std::min(a.x, b.x), x1 = std::max(a.x, b.x), y0 = std::min(a.y, b.y), y1 = std::max(a.y, b.y);
      if (x1 > r.x + eps && x0 < r.x + r.w - eps && y1 > r.y + eps && y0 < r.y + r.h - eps) return true;
    }
    return false;
  };
  auto chans = [&](bool xs) {
    std::vector<double> e, c;
    for (size_t i = 0; i < obs.size() && i < channels; i++) { auto& r = obs[i]; e.push_back(xs ? r.x : r.y); e.push_back(xs ? r.x + r.w : r.y + r.h); }
    std::sort(e.begin(), e.end());
    for (size_t i = 0; i + 1 < e.size(); i++) {
      double g = e[i + 1] - e[i];
      if (g <= 8) continue;
      int n = std::min(6, std::max(1, static_cast<int>(g / 12)));
      for (int k = 0; k < n; k++) c.push_back((e[i] + e[i + 1]) / 2 + (k - (n - 1) / 2.0) * 12);
    }
    if (!e.empty()) for (double d : {14.0, 24.0, 34.0, 46.0}) c.push_back(e.front() - d), c.push_back(e.back() + d);
    for (Pt t : {p0, q0}) c.push_back(xs ? t.x : t.y);
    std::sort(c.begin(), c.end());
    std::vector<double> u;
    for (double v : c) if (u.empty() || v - u.back() > 0.5) u.push_back(v);
    return u;
  };
  std::vector<double> X = chans(true), Y = chans(false);
  int nx = X.size(), ny = Y.size();
  auto at = [&](double v, const std::vector<double>& a) { int b = 0; for (int i = 0; i < (int)a.size(); i++) if (std::fabs(a[i] - v) < std::fabs(a[b] - v)) b = i; return b; };
  int sx = at(p0.x, X), sy = at(p0.y, Y), tx = at(q0.x, X), ty = at(q0.y, Y);
  auto dn = [&](const std::string& s) { return s == "right" ? 0 : s == "bottom" ? 1 : s == "left" ? 2 : 3; };
  const int DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};
  int d0 = dn(sa), dEnd = (dn(sb) + 2) % 4;
  auto id = [&](int i, int j, int d) { return (j * nx + i) * 4 + d; };
  std::vector<double> best(nx * ny * 4, 1e18);
  std::vector<int> from(nx * ny * 4, -1);
  using QI = std::pair<double, int>;
  std::priority_queue<QI, std::vector<QI>, std::greater<QI>> pq;
  best[id(sx, sy, d0)] = 0;
  pq.push({0, id(sx, sy, d0)});
  int goal = -1;
  double goalCost = 1e18;
  while (!pq.empty()) {
    auto [c, u] = pq.top();
    pq.pop();
    if (c > best[u]) continue;
    int d = u % 4, i = (u / 4) % nx, j = u / 4 / nx;
    if (i == tx && j == ty) {
      double t = c + (d == dEnd ? 0 : bendCost);
      if (t < goalCost) goalCost = t, goal = u;
      continue;
    }
    for (int nd = 0; nd < 4; nd++) {
      if ((nd + 2) % 4 == d) continue;
      int ni = i + DX[nd], nj = j + DY[nd];
      if (ni < 0 || nj < 0 || ni >= nx || nj >= ny) continue;
      Pt a{X[i], Y[j]}, b{X[ni], Y[nj]};
      bool bad = false;
      for (auto& r : obs) if (inside(r, b.x, b.y)) { bad = true; break; }
      if (bad || hit(a, b)) continue;
      double len = std::fabs(b.x - a.x) + std::fabs(b.y - a.y), shared = 0;
      if (used)
        for (auto& u : *used) {
          bool vx = std::fabs(a.x - b.x) < 0.5, ux = std::fabs(u.first.x - u.second.x) < 0.5;
          if (vx && ux && std::fabs(a.x - u.first.x) < 3) shared += std::max(0.0, std::min(std::max(a.y, b.y), std::max(u.first.y, u.second.y)) - std::max(std::min(a.y, b.y), std::min(u.first.y, u.second.y)));
          if (!vx && !ux && std::fabs(a.y - u.first.y) < 3) shared += std::max(0.0, std::min(std::max(a.x, b.x), std::max(u.first.x, u.second.x)) - std::max(std::min(a.x, b.x), std::min(u.first.x, u.second.x)));
        }
      double nc = c + len + shared * 4 + (nd == d ? 0 : bendCost);
      int v = id(ni, nj, nd);
      if (nc < best[v]) best[v] = nc, from[v] = u, pq.push({nc, v});
    }
  }
  if (goal < 0) return {p, q};
  std::vector<Pt> rev;
  for (int u = goal; u >= 0; u = from[u]) rev.push_back({X[(u / 4) % nx], Y[u / 4 / nx]});
  std::reverse(rev.begin(), rev.end());
  std::vector<Pt> out{p};
  for (auto& t : rev) out.push_back(t);
  out.push_back(q);
  // the stubs may not be aligned with the channel grid: snap them to the ends
  out[1] = p0;
  out[out.size() - 2] = q0;
  // merge the collinear points the grid leaves
  std::vector<Pt> r;
  for (auto& t : out) {
    while (r.size() >= 2 && ((std::fabs(r[r.size() - 2].x - r.back().x) < 1e-9 && std::fabs(r.back().x - t.x) < 1e-9) || (std::fabs(r[r.size() - 2].y - r.back().y) < 1e-9 && std::fabs(r.back().y - t.y) < 1e-9))) r.pop_back();
    r.push_back(t);
  }
  if (used) for (size_t i = 0; i + 1 < r.size(); i++) used->push_back({r[i], r[i + 1]});
  return r;
}

inline Route route(const Rect& a, Anchor sa, const Rect& b, Anchor sb, const Opts& o = {}) {
  Sides f = facing(a, b);
  if (sa.side.empty()) sa.side = f.a;
  if (sb.side.empty()) sb.side = f.b;
  Pt p = point(a, sa), q = point(b, sb);
  Route r;
  r.line = simplify(o.obstacles ? around(p, sa.side, q, sb.side, *o.obstacles, o.used, o.channels) : o.elbow ? elbow(p, sa.side, q, sb.side, o.margin) : std::vector<Pt>{p, q});
  bool bend = horiz(sa.side) ? std::fabs(p.y - q.y) > 1e-9 : true;
  if (o.curve && !o.elbow && !o.obstacles && bend) {
    double k = sa.side == sb.side ? o.margin : std::max(40.0, 0.45 * dist(p, q));
    Pt da = dir(sa.side), db = dir(sb.side);
    r.line = {p, {p.x + da.x * k, p.y + da.y * k}, {q.x + db.x * k, q.y + db.y * k}, q};
    r.bezier = true;
  }
  if (o.obstacles && r.line.size() >= 2) {
    std::vector<std::pair<double, Cand>> h, v;
    for (size_t i = 1; i < r.line.size(); i++) {
      Pt a = r.line[i - 1], b = r.line[i], m{(a.x + b.x) / 2, (a.y + b.y) / 2};
      double lx = std::fabs(b.x - a.x), ly = std::fabs(b.y - a.y);
      if (ly < 1e-9 && lx >= 36) h.push_back({lx, {m, true, false}});
      if (lx < 1e-9 && ly >= 28) v.push_back({ly, {m, false, false}}), v.push_back({ly - 0.1, {m, false, true}});
    }
    auto by = [](const std::pair<double, Cand>& x, const std::pair<double, Cand>& y) { return x.first > y.first; };
    std::stable_sort(h.begin(), h.end(), by);
    std::stable_sort(v.begin(), v.end(), by);
    for (auto& c : h) r.cands.push_back(c.second);
    for (auto& c : v) r.cands.push_back(c.second);
  }
  r.mid = r.bezier ? Pt{(p.x + 3 * r.line[1].x + 3 * r.line[2].x + q.x) / 8, (p.y + 3 * r.line[1].y + 3 * r.line[2].y + q.y) / 8} : midpoint(r.line, r.flat, r.left);
  if (r.bezier) r.flat = horiz(sa.side);
  r.kind = o.head && o.tip != "none" ? o.tip : "";
  r.kind2 = !o.tail.empty() ? (o.tail == "none" ? "" : o.tail) : o.both ? "arrow" : "";
  if (r.line.size() >= 2) {
    double len = std::max(11.0, o.sw * 3.3);
    if (!r.kind.empty()) {
      double cut = diamond(r.kind) ? len * 1.8 : len;
      r.head = headAt(r.line.back(), r.line[r.line.size() - 2], len, r.kind);
      if (o.trim && !r.bezier) shorten(r.line, cut);
    }
    if (!r.kind2.empty()) {
      double cut = diamond(r.kind2) ? len * 1.8 : len;
      r.head2 = headAt(r.line.front(), r.line[1], len, r.kind2);
      if (o.trim && !r.bezier) {
        std::reverse(r.line.begin(), r.line.end());
        shorten(r.line, cut);
        std::reverse(r.line.begin(), r.line.end());
      }
    }
  }
  return r;
}

}
