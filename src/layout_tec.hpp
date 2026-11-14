#pragma once
#include <algorithm>
#include <map>
#include "item_tec.hpp"

namespace LAYOUT {

using FIG::Item;
using Mem = double Item::*;

[[noreturn]] inline void fail(const Item& it, const std::string& m) { throw DSL::Error(it.node->line, m); }

inline double ev(const Item& it, const std::string& k, const std::string& s) {
  try { return EXPR::eval(s); } catch (const std::exception& e) { fail(it, k + ": " + e.what()); }
}

inline bool pct(const std::string& s) { return !s.empty() && s.back() == '%'; }

inline double val(const Item& it, const std::string& k, double d) {
  std::string s = it.attr(k);
  return s.empty() || pct(s) ? d : ev(it, k, s);
}

inline double rel(const Item& it, const std::string& k, double total, double d) {
  std::string s = it.attr(k);
  if (!pct(s)) return val(it, k, d);
  s.pop_back();
  return ev(it, k, s) * total / 100;
}

inline double frac(const Item& it, const std::string& s) {
  if (s == "start") return 0;
  if (s == "center") return 0.5;
  if (s == "end") return 1;
  fail(it, "align: '" + s + "' is not start, center, end or stretch");
}

inline double pad(const Item& it, bool x) {
  double d = it.kind == "group" ? 14 : it.kind == "box" ? (x ? 14 : 8) : 0;
  std::string k = x ? "padx" : "pady";
  if (it.node && it.node->has("pad") && !it.node->has(k)) return val(it, "pad", d);
  return val(it, k, val(it, "pad", d));
}

inline std::vector<Item*> flow(Item& it) {
  std::vector<Item*> r;
  for (auto& k : it.kids) if (!k.arrow()) r.push_back(&k);
  return r;
}

inline bool container(const Item& it) {
  const std::string& k = it.kind;
  return FIG::container(k) || (k == "box" && !flow(const_cast<Item&>(it)).empty());
}

inline double gapOf(const Item& it) { return val(it, "gap", it.kind == "group" ? 10 : 12); }

inline std::string title(const Item& it) { return it.kind == "group" ? it.attr("title", it.text) : ""; }

inline FIG::Block head(const Item& it) { return FIG::block(it, title(it), it.fs, true); }

inline bool vertical(const Item& it) { return it.kind == "col" || it.kind == "box" || (it.kind == "group" && it.attr("layout") != "row"); }

inline void fit(Item& it, double w, double h) {
  it.w = std::max({val(it, "w", 0), w, val(it, "minw", 0)});
  it.h = std::max({val(it, "h", 0), h, val(it, "minh", 0)});
}

inline double top(const Item& it) {
  FIG::Block b = head(it);
  return b.lines.empty() ? 0 : b.h + gapOf(it);
}

inline void measureFlow(Item& it) {
  bool v = vertical(it);
  auto ks = flow(it);
  double main = ks.empty() ? 0 : gapOf(it) * (ks.size() - 1), cross = 0;
  for (auto k : ks) {
    main += v ? k->h : k->w;
    cross = std::max(cross, v ? k->w : k->h);
  }
  double px = pad(it, true), py = pad(it, false);
  double w = (v ? cross : main) + 2 * px, h = (v ? main : cross) + 2 * py + top(it);
  w = std::max(w, head(it).w + 2 * px);
  if (it.kind == "box") {
    it.w = it.has("w") ? val(it, "w", 0) : w;
    it.h = it.has("h") ? val(it, "h", 0) : h;
  } else fit(it, w, h);
}

inline std::vector<double> weightsOf(const Item& it) {
  std::vector<double> r;
  for (auto& s : DSL::list(it.attr("weights"))) if (!s.empty()) r.push_back(EXPR::eval(s));
  return r;
}

inline double flexW(const Item& k, bool v) {
  if (k.has(v ? "h" : "w")) return 0;
  if (k.has("flex")) { double x = val(k, "flex", 1); return x > 0 ? x : 0; }
  return k.flag("flex") ? 1 : 0;
}

inline bool flexy(const Item& k) { return k.flag("flex") || (k.has("flex") && k.attr("flex") != "0"); }

inline bool wide(const Item& k) { return k.flag("stretch") || k.flag("grow") || flexy(k) || (k.kind == "text" && (k.flag("right") || k.flag("center"))); }

inline void placeFlow(Item& it) {
  bool v = vertical(it);
  Mem mp = v ? &Item::y : &Item::x, ms = v ? &Item::h : &Item::w, cp = v ? &Item::x : &Item::y, cs = v ? &Item::w : &Item::h;
  double px = pad(it, true), py = pad(it, false), gap = gapOf(it);
  double c0 = v ? it.x + px : it.y + py + top(it), cur = v ? it.y + py + top(it) : it.x + px;
  double inner = v ? it.w - 2 * px : it.h - 2 * py - top(it);
  double along = v ? it.h - 2 * py - top(it) : it.w - 2 * px;
  std::string al = it.attr("align", v ? "start" : "center");
  bool all = al == "stretch", fill = al == "fill";
  double f = all || fill ? 0 : frac(it, al);
  auto ks = flow(it);
  std::vector<double> cell(ks.size(), -1);
  auto wt = weightsOf(it);
  if (!v && wt.empty()) {
    bool any = false;
    for (auto k : ks) any = any || k->flag("grow");
    if (any) for (auto k : ks) wt.push_back(k->flag("grow") ? 1 : 0);
  }
  if (wt.empty()) {
    std::vector<double> fw;
    bool any = false;
    for (auto k : ks) { fw.push_back(flexW(*k, v)); any = any || fw.back() > 0; }
    if (any) wt = fw;
  }
  if (!wt.empty()) {
    double fixed = 0, sum = 0;
    for (size_t i = 0; i < ks.size(); i++) {
      double x = i < wt.size() ? wt[i] : 1;
      if (x == 0) fixed += ks[i]->*ms; else sum += x;
    }
    double free = along - gap * (ks.size() - 1) - fixed;
    for (size_t i = 0; i < ks.size(); i++) {
      double x = i < wt.size() ? wt[i] : 1;
      if (x == 0) continue;
      Item* k = ks[i];
      if (v) {
        cell[i] = std::max(free * x / sum, k->*ms);
        k->*ms = cell[i];
      } else {
        cell[i] = free * x / sum;
        k->*ms = container(*k) || wide(*k) ? cell[i] : std::min(k->*ms, cell[i]);
      }
    }
  } else if (v && it.has("align") && !all && !fill) {
    double used = ks.empty() ? 0 : gap * (ks.size() - 1);
    for (auto k : ks) used += k->h;
    cur += f * (along - used);
  }
  for (size_t i = 0; i < ks.size(); i++) {
    Item* k = ks[i];
    bool auto_ = !it.has("align") && it.kind == "col";
    bool grow = all || k->flag("stretch") || (auto_ && k->kind == "box") || (fill && ((container(*k) && !k->has("w")) || wide(*k)));
    if (grow) k->*cs = inner;
    k->*mp = cur;
    bool mid = it.kind == "col" && k->kind == "text" && !k->flag("left") && !k->flag("right");
    k->*cp = c0 + ((auto_ && k->kind != "box") || (mid && !all) ? 0.5 : f) * (inner - k->*cs);
    cur += (cell[i] >= 0 ? cell[i] : k->*ms) + gap;
  }
}

struct Cell {
  Item* item;
  int c, r, s, rs;
};

struct Plan {
  std::vector<Cell> cells;
  std::vector<double> cw, rh;
};

inline double gapx(const Item& it) { return val(it, "gapx", gapOf(it)); }
inline double gapy(const Item& it) { return val(it, "gapy", gapOf(it)); }

inline Plan plan(Item& it) {
  Plan p;
  int n = std::max(1, static_cast<int>(val(it, "cols", 1))), c = 0, r = 0;
  double gap = gapx(it), gv = gapy(it);
  p.cw.assign(n, 0);
  std::set<std::pair<int, int>> busy;
  for (auto k : flow(it)) {
    int s = std::min(n, std::max(1, static_cast<int>(val(*k, "span", 1)))), rs = std::max(1, static_cast<int>(val(*k, "rspan", 1)));
    while (busy.count({c, r})) if (++c >= n) { c = 0; r++; }
    if (c + s > n) { c = 0; r++; }
    p.cells.push_back({k, c, r, s, rs});
    for (int i = 0; i < rs; i++) for (int j = 0; j < s; j++) busy.insert({c + j, r + i});
    if (static_cast<int>(p.rh.size()) < r + rs) p.rh.resize(r + rs, 0);
    if (rs == 1) p.rh[r] = std::max(p.rh[r], k->h);
    if (s == 1) p.cw[c] = std::max(p.cw[c], k->w);
    c += s;
    if (c >= n) { c = 0; r++; }
  }
  for (auto& e : p.cells) {
    if (e.rs > 1) {
      double rows = gv * (e.rs - 1);
      for (int i = e.r; i < e.r + e.rs; i++) rows += p.rh[i];
      if (e.item->h > rows) p.rh[e.r + e.rs - 1] += e.item->h - rows;
    }
    double have = gap * (e.s - 1);
    for (int i = e.c; i < e.c + e.s; i++) have += p.cw[i];
    if (e.item->w > have) p.cw[e.c + e.s - 1] += e.item->w - have;
  }
  return p;
}

inline void measureGrid(Item& it) {
  Plan p = plan(it);
  double gap = gapx(it), gv = gapy(it), w = 0, h = 0;
  for (double x : p.cw) w += x;
  for (double x : p.rh) h += x;
  w += gap * (p.cw.size() - 1);
  if (!p.rh.empty()) h += gv * (p.rh.size() - 1);
  fit(it, w + 2 * pad(it, true), h + 2 * pad(it, false));
}

inline void placeGrid(Item& it) {
  Plan p = plan(it);
  double gap = gapx(it), gv = gapy(it);
  std::string al = it.attr("align", "start");
  bool all = al == "stretch";
  double f = all ? 0 : frac(it, al);
  for (auto& e : p.cells) {
    double x = it.x + pad(it, true), y = it.y + pad(it, false), w = gap * (e.s - 1);
    for (int i = 0; i < e.c; i++) x += p.cw[i] + gap;
    for (int i = 0; i < e.r; i++) y += p.rh[i] + gv;
    for (int i = e.c; i < e.c + e.s; i++) w += p.cw[i];
    if (all || e.item->flag("stretch")) { e.item->w = w; e.item->h = p.rh[e.r]; }
    e.item->x = x + f * (w - e.item->w);
    double hh = gv * (e.rs - 1);
    for (int i = e.r; i < e.r + e.rs; i++) hh += p.rh[i];
    e.item->y = y + (e.item->has("valign") ? frac(*e.item, e.item->attr("valign")) : f) * (hh - e.item->h);
  }
}

inline void measureLayer(Item& it) {
  double w = 0, h = 0;
  for (auto k : flow(it)) { w = std::max(w, k->w); h = std::max(h, k->h); }
  fit(it, w, h);
}

inline void placeLayer(Item& it) {
  for (auto k : flow(it)) {
    k->w = rel(*k, "w", it.w, k->flag("stretch") ? it.w : k->w);
    k->h = rel(*k, "h", it.h, k->flag("stretch") ? it.h : k->h);
    k->x = it.x + rel(*k, "x", it.w, 0);
    k->y = it.y + rel(*k, "y", it.h, 0);
  }
}

inline void locate(Item& k, const std::map<std::string, Item*>& seen, double margin) {
  double x = margin, y = margin;
  if (k.has("at")) {
    std::string at = k.attr("at");
    size_t c = at.find(',');
    if (c == std::string::npos) fail(k, "at: x,y was expected");
    x += ev(k, "at", at.substr(0, c));
    y += ev(k, "at", at.substr(c + 1));
  }
  auto ref = [&](const char* key) -> Item* {
    if (!k.has(key)) return nullptr;
    std::string id = k.attr(key);
    auto f = seen.find(id);
    if (f != seen.end()) return f->second;
    std::vector<std::string> names;
    for (auto& e : seen) names.push_back(e.first);
    fail(k, std::string(key) + ": there is no element '" + id + "' before this one" + DSL::suggest(id, names));
  };
  if (Item* a = ref("alignx")) x = a->x + (a->w - k.w) / 2;
  if (Item* a = ref("aligny")) y = a->y + (a->h - k.h) / 2;
  k.x = x;
  k.y = y;
}

inline void measureCanvas(Item& it) {
  double margin = val(it, "pad", 0), w = 0, h = 0;
  std::map<std::string, Item*> seen;
  for (auto k : flow(it)) {
    locate(*k, seen, margin);
    if (!k->id.empty()) seen[k->id] = k;
    w = std::max(w, k->x + k->w);
    h = std::max(h, k->y + k->h);
  }
  it.w = std::max(it.has("w") ? val(it, "w", 0) : w + margin, val(it, "minw", 0));
  it.h = std::max(it.has("h") ? val(it, "h", 0) : h + margin, val(it, "minh", 0));
}

inline void measure(Item& it) {
  if (container(it)) {
    for (auto k : flow(it)) measure(*k);
    if (it.kind == "grid") measureGrid(it);
    else if (it.kind == "layer") measureLayer(it);
    else if (it.kind == "canvas") measureCanvas(it);
    else measureFlow(it);
    return;
  }
  bool wrap = it.kind == "text" && it.flag("wrap");
  FIG::Block b = FIG::block(it, it.text, it.fs, it.flag("bold"), wrap ? val(it, "w", 0) : 0);
  double empty = it.kind == "box" && it.text.empty() ? 36 : 0;
  double bw = it.kind != "text" && it.flag("wrap") && it.has("w") ? 0 : b.w;
  fit(it, std::max(empty, bw + 2 * pad(it, true)), std::max(empty, b.h + 2 * pad(it, false)));
}

inline void place(Item& it) {
  if (!container(it)) return;
  if (it.kind == "grid") placeGrid(it);
  else if (it.kind == "layer") placeLayer(it);
  else if (it.kind == "canvas") for (auto k : flow(it)) { k->x += it.x; k->y += it.y; }
  else placeFlow(it);
  for (auto k : flow(it)) place(*k);
}

inline void run(Item& root, double x = 0, double y = 0) {
  measure(root);
  root.x = x;
  root.y = y;
  place(root);
}

}
