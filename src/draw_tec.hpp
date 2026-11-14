#pragma once
#include <algorithm>
#include <cmath>
#include <sstream>
#include "connect_tec.hpp"
#include "item_tec.hpp"
#include "svg_tec.hpp"

namespace DRAW {

using FIG::Item;
using SVG::ShapeContainer;

struct End {
  const Item* item = nullptr;
  std::string side;
  double at = 0.5;
};

inline CONNECT::Rect rect(const Item& i) { return {i.x, i.y, i.w, i.h}; }

inline bool hidden(const std::string& c) { return c.empty() || c == "none"; }

inline SVG::Color color(const std::string& name) {
  std::string c = THEME::color(name);
  if (hidden(c)) throw std::invalid_argument("unknown color '" + name + "'" + DSL::suggest(name, THEME::names()));
  return SVG::Color(c);
}

inline std::string family(const Item& it) {
  if (it.flag("mono")) return "Menlo,Consolas,'DejaVu Sans Mono','Courier New',monospace";
  return "Helvetica,Arial,sans-serif";
}

inline std::string ink(const std::string& fill) {
  std::string c = THEME::color(fill);
  if (hidden(c)) return "text";
  SVG::Color k(c);
  return 0.299 * k.r + 0.587 * k.g + 0.114 * k.b < 140 ? "white" : "text";
}

inline std::string lid(const std::string& fill) {
  std::string c = THEME::color(fill);
  if (hidden(c)) return fill;
  SVG::Color k(c);
  return SVG::Color(k.r + (255 - k.r) * 2 / 5, k.g + (255 - k.g) * 2 / 5, k.b + (255 - k.b) * 2 / 5).toHex();
}

template <class S>
S& paint(S& s, const std::string& fill, const std::string& stroke, double sw, const std::string& dash = "") {
  bool st = !hidden(stroke) && sw > 0;
  if (hidden(fill)) s.noFill(); else s.fill(color(fill));
  if (st) s.stroke(color(stroke)).stroke(sw, dash); else s.noStroke();
  s.cls(THEME::cls(hidden(fill) ? "" : fill, st ? stroke : ""));
  return s;
}

// `hover=NAME` (any value but 0): the mark is highlighted under the cursor and the other marks with the same NAME fade;
// marks with another NAME do not react to it (the CSS is in the SVG, see THEME::hoverCss). `hover=NAME:SERIES`: the marks
// of the same series are highlighted together and only the marks of the other series of NAME fade.
inline bool interactive(const Item& it) { return it.has("hover") && it.attr("hover") != "0" && !it.attr("hover").empty(); }
inline std::string safe(std::string g) {
  for (auto& c : g) if (!isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_') c = '_';
  return g;
}
inline std::string group(const Item& it) {  // NAME or NAME:SERIES, without strange characters
  std::string g = it.attr("hover"), p = g.find(':') == std::string::npos ? "" : g.substr(g.find(':') + 1);
  g = safe(g.substr(0, g.find(':')));
  return p.empty() ? g : g + ":" + safe(p);
}
inline std::string classes(const Item& it) {
  std::string g = group(it), n = g.substr(0, g.find(':'));
  return "hl hl-" + n + (g == n ? "" : " hs-" + n + "_" + g.substr(n.size() + 1));
}

template <class S>
S& finish(S& s, const Item& it) {
  if (it.has("linecap")) s.cap(it.attr("linecap"));
  if (it.has("join")) s.join(it.attr("join"));
  if (it.num("alpha", 1) < 1) s.opacity(it.num("alpha", 1));
  if (it.has("tooltip")) s.tooltip(it.attr("tooltip"));
  if (interactive(it)) s.addCls(classes(it));
  return s;
}

struct Face {
  double size;
  bool bold = false, italic = false;
  std::string anchor = "middle", color = "text";
  bool halo = false;
  double maxw = 0;
  bool serif = false;
  double base = 0.35;
};

inline void label(ShapeContainer& o, const Item& it, const std::string& s, double x, double cy, const Face& f) {
  FIG::Block b = FIG::block(it, s, f.size, f.bold, f.maxw);
  for (size_t i = 0; i < b.lines.size(); i++) {
    auto& t = o.text(x, cy - (b.lines.size() - 1) * b.lh / 2 + i * b.lh + f.size * f.base, b.lines[i]);
    t.font(f.serif ? "Georgia,'Palatino Linotype','Times New Roman',serif" : family(it), f.size).anchor(f.anchor).bold(f.bold).italic(f.italic).fill(color(f.color)).cls(THEME::cls(f.color));
    if (it.num("rotate", 0) != 0) t.rotate(it.num("rotate", 0), x, cy);
    if (f.halo) t.stroke(SVG::Color(255, 255, 255)).stroke(4.0).attr("paint-order", "stroke").join("round");
  }
}

inline void frame(ShapeContainer& o, const Item& it, const std::string& fill) {
  bool line = it.has("border") || it.kind == "group";
  if (hidden(fill) && !line) return;
  paint(o.rect(it.x, it.y, it.w, it.h, it.num("radius", it.kind == "group" ? 8 : 0)), fill, line ? it.attr("stroke", "text") : "none", it.num("sw", 1.6), it.attr("dash"));
}

inline void caption(ShapeContainer& o, const Item& it) {
  std::string c = it.attr("caption"), st = it.attr("captionstyle");
  if (c.empty()) return;
  Face f{it.num("capsize", it.fs), st == "bold", st == "italic"};
  FIG::Block b = FIG::block(it, c, f.size, f.bold);
  bool top = it.attr("captionpos") == "top";
  label(o, it, c, it.x + it.w / 2, top ? it.y - 6 - b.h / 2 : it.y + it.h + 6 + b.h / 2, f);
}

inline Face face(const Item& it, const std::string& anchor, const std::string& color) {
  Face f{it.fs, it.flag("bold"), it.flag("italic"), anchor, color};
  f.base = it.num("base", 0.35);
  return f;
}

inline void shape(ShapeContainer& o, const Item& it) {
  EXPR::Vars v{{"w", it.w}, {"h", it.h}};
  std::string fill = it.fillName("white"), stroke = it.attr("stroke", "text");
  double sw = it.num("sw", 1.6);
  SVG::Group& g = o.group();
  g.translate(it.x, it.y);
  for (auto& k : it.kids) {
    if (k.kind == "set") {
      for (auto& t : k.node->args) if (!t.key.empty()) v[t.key] = EXPR::eval(t.v, v);
      continue;
    }
    std::string f = k.attr("fill"), s = k.attr("stroke", stroke);
    if (f == "self") f = fill;
    else if (f == "lid") f = lid(fill);
    auto n = [&](const std::string& name, double d) { return k.num(name, d, v); };
    double lw = n("sw", sw);
    if (k.kind == "rect") finish(paint(g.rect(n("x", 0), n("y", 0), n("w", it.w), n("h", it.h), n("r", 0)), f, s, lw, k.attr("dash")), k);
    else if (k.kind == "ellipse") finish(paint(g.ellipse(n("cx", it.w / 2), n("cy", it.h / 2), n("rx", it.w / 2), n("ry", it.h / 2)), f, s, lw, k.attr("dash")), k);
    else if (k.kind == "line") finish(paint(g.line(n("x1", 0), n("y1", 0), n("x2", it.w), n("y2", 0)), "", s, lw, k.attr("dash")), k);
    else if (k.kind == "path") finish(paint(g.path(EXPR::fill(k.attr("d"), v, SVG::num)), f, s, lw, k.attr("dash")), k);
    else if (k.kind == "poly") {
      std::string d = EXPR::fill(k.attr("pts"), v, SVG::num);
      std::replace(d.begin(), d.end(), ',', ' ');
      std::istringstream in(d);
      SVG::Points pts;
      for (double x, y; in >> x >> y;) pts.emplace_back(x, y);
      if (k.flag("open")) finish(paint(g.polyline(pts), f, s, lw, k.attr("dash")), k);
      else finish(paint(g.polygon(pts), f, s, lw, k.attr("dash")), k);
    } else if (k.kind == "label") label(g, k, k.text, n("x", it.w / 2), n("y", it.h / 2), face(k, k.flag("left") ? "start" : k.flag("right") ? "end" : "middle", k.attr("color", "text")));
    else throw std::invalid_argument("'" + k.kind + "' cannot go inside a shape");
  }
  if (!it.text.empty()) {
    Face f = face(it, it.flag("left") ? "start" : it.flag("right") ? "end" : "middle", it.attr("color", ink(fill)));
    if (it.flag("wrap")) f.maxw = it.w - 2 * it.num("padx", 8);
    label(g, it, it.text, it.num("tx", it.w / 2, v), it.num("ty", it.h / 2, v), f);
  }
}

inline void box(ShapeContainer& o, const Item& it) {
  std::string fill = it.fillName("white");
  double sh = it.num("shadow", 0);
  if (sh > 0) paint(o.rect(it.x + sh, it.y + sh, it.w, it.h, it.num("radius", 6)), "shadow-color", "none", 0);
  paint(o.rect(it.x, it.y, it.w, it.h, it.num("radius", 6)), fill, it.attr("stroke", "text"), it.num("sw", 1.6), it.attr("dash"));
  label(o, it, it.text, it.x + it.w / 2, it.y + it.h / 2, face(it, "middle", ink(fill)));
}

inline void text(ShapeContainer& o, const Item& it) {
  std::string a = it.flag("right") ? "end" : it.flag("center") ? "middle" : it.flag("left") ? "start" : "middle";
  double x = a == "start" ? it.x : a == "end" ? it.x + it.w : it.x + it.w / 2;
  std::string bg = it.fillName();
  if (!hidden(THEME::color(bg))) paint(o.rect(it.x, it.y, it.w, it.h), bg, "none", 0);
  Face f = face(it, a, it.attr("color", "text"));
  if (it.flag("wrap")) f.maxw = it.w;
  label(o, it, it.text, x, it.y + it.h / 2, f);
}

inline void group(ShapeContainer& o, const Item& it) {
  frame(o, it, it.fillName("light"));
  std::string title = it.attr("title", it.text);
  if (title.empty()) return;
  FIG::Block b = FIG::block(it, title, it.fs, true);
  label(o, it, title, it.x + it.w / 2, it.y + it.num("pady", it.num("pad", 14)) + b.h / 2, {it.fs, true});
}

inline void item(ShapeContainer& o, const Item& it) {
  if (it.arrow()) return;
  try {
    const std::string& k = it.kind;
    if (k == "shape") shape(o, it);
    else if (k == "box") box(o, it);
    else if (k == "text") text(o, it);
    else if (k == "group") group(o, it);
    else if (FIG::container(k)) frame(o, it, it.fillName());
    caption(o, it);
  } catch (const DSL::Error&) {
    throw;
  } catch (const std::exception& e) {
    throw DSL::Error(it.node->line, e.what());
  }
  if (it.kind != "shape") for (auto& c : it.kids) item(o, c);
}

inline void arrow(ShapeContainer& o, const End& a, const End& b, const Item& st, bool plain = false, double* bb = nullptr, const std::vector<CONNECT::Rect>* obs = nullptr, std::vector<std::pair<CONNECT::Pt, CONNECT::Pt>>* used = nullptr, std::vector<CONNECT::Rect>* placed = nullptr, size_t channels = static_cast<size_t>(-1)) {
  CONNECT::Opts op;
  op.head = !plain;
  op.elbow = st.flag("elbow");
  op.both = st.flag("both");
  op.tip = st.attr("tip", "arrow");
  op.tail = st.attr("tail");
  op.curve = st.flag("curve");
  op.trim = !st.flag("full");
  op.sw = st.num("sw", 2.5);
  op.margin = st.num("margin", 26);
  op.obstacles = obs;
  op.channels = channels;
  op.used = used;
  CONNECT::Route r = CONNECT::route(rect(*a.item), {a.side, a.at}, rect(*b.item), {b.side, b.at}, op);
  if (bb) {
    for (auto* v : {&r.line, &r.head, &r.head2})
      for (size_t n = 0; n < v->size(); n++) {
        if (r.bezier && v == &r.line && (n == 1 || n == 2)) continue;
        auto& p = (*v)[n];
        bb[0] = std::min(bb[0], p.x - 2), bb[1] = std::min(bb[1], p.y - 2);
        bb[2] = std::max(bb[2], p.x + 2), bb[3] = std::max(bb[3], p.y + 2);
      }
  }
  std::string col = st.attr("color", "text"), d;
  auto xy = [&](size_t i) { return SVG::num(r.line[i].x) + " " + SVG::num(r.line[i].y); };
  if (r.bezier) d = "M " + xy(0) + " C " + xy(1) + ", " + xy(2) + ", " + xy(3);
  else for (size_t i = 0; i < r.line.size(); i++) d += (i ? " L " : "M ") + xy(i);
  paint(o.path(d), "", col, op.sw, st.attr("dash"));
  for (int side = 0; side < 2; side++) {
    auto* h = side ? &r.head2 : &r.head;
    const std::string& kind = side ? r.kind2 : r.kind;
    if (h->empty()) continue;
    SVG::Points pts;
    for (auto& p : *h) pts.emplace_back(p.x, p.y);
    if (kind == "arrow" || kind == "diamondfill") paint(o.polygon(pts), col, "none", 0);
    else paint(o.polygon(pts), "white", col, op.sw * 0.7);
  }
  if ((st.text.empty() && !st.has("label")) || r.line.size() < 2) return;
  std::string text = st.has("label") ? st.attr("label") : st.text, side = st.attr("side");
  FIG::Block blk = FIG::block(st, text, st.fs, false);
  Face f{st.fs, false, st.flag("italic")};
  f.color = col;
  f.halo = true;
  f.base = st.num("base", f.base);
  std::string fixed = st.attr("side");
  auto put = [&](CONNECT::Pt m, bool flat, bool left, double& x, double& cy, std::string& anchor, CONNECT::Rect& box) {
    std::string side = fixed;
    if (side.empty()) side = flat ? "top" : left ? "left" : "right";
    x = m.x, cy = m.y, anchor = "middle";
    if (side == "center") {}
    else if (side == "top" || side == "above") cy -= 7 + blk.h / 2;
    else if (side == "bottom" || side == "below") cy += 7 + blk.h / 2;
    else if (side == "right") { x += 7; anchor = "start"; }
    else { x -= 7; anchor = "end"; }
    double lx = anchor == "start" ? x : anchor == "end" ? x - blk.w : x - blk.w / 2;
    box = {lx, cy - blk.h / 2, blk.w, blk.h};
  };
  double x = 0, cy = 0;
  std::string anchor;
  CONNECT::Rect box;
  put(r.mid, r.flat, r.left, x, cy, anchor, box);
  if (obs && !r.cands.empty()) {
    auto cost = [&](const CONNECT::Rect& b) {
      auto ov = [&](const CONNECT::Rect& k, double m) {
        double w = std::min(b.x + b.w, k.x + k.w - m) - std::max(b.x, k.x + m), h = std::min(b.y + b.h, k.y + k.h - m) - std::max(b.y, k.y + m);
        return w > 0 && h > 0 ? w * h : 0.0;
      };
      double c = 0;
      for (auto& k : *obs) c += ov(k, 1) * 4;
      if (placed) for (auto& k : *placed) c += ov(k, 0);
      return c;
    };
    double bestCost = cost(box);
    for (auto& c : r.cands) {
      if (bestCost <= 0) break;
      double cx2, cy2;
      std::string an2;
      CONNECT::Rect b2;
      put(c.m, c.flat, c.left, cx2, cy2, an2, b2);
      double k = cost(b2);
      if (k < bestCost) { bestCost = k, x = cx2, cy = cy2, anchor = an2, box = b2; }
    }
    if (placed) placed->push_back(box);
  }
  f.anchor = anchor;
  if (bb) {
    bb[0] = std::min(bb[0], box.x), bb[1] = std::min(bb[1], box.y);
    bb[2] = std::max(bb[2], box.x + box.w), bb[3] = std::max(bb[3], box.y + box.h);
  }
  label(o, st, text, x, cy, f);
}

struct Note {
  const Item* st;
  std::string side;
  CONNECT::Pt dot;
  CONNECT::Rect box;
};

inline void place(std::vector<Note>& v, const double* e, double* bb) {
  for (const char* side : {"right", "left", "top", "bottom"}) {
    std::vector<Note*> s;
    for (auto& n : v) if (n.side == side) s.push_back(&n);
    bool h = CONNECT::horiz(side);
    std::stable_sort(s.begin(), s.end(), [&](Note* a, Note* b) { return h ? a->dot.y < b->dot.y : a->dot.x < b->dot.x; });
    double next = -1e18;
    for (Note* n : s) {
      const Item& st = *n->st;
      FIG::Block b = FIG::block(st, st.text, st.fs, st.flag("bold"));
      CONNECT::Rect& r = n->box;
      r.w = b.w + 2 * st.num("padx", 12), r.h = b.h + 2 * st.num("pady", 8);
      double gap = st.num("gap", 48);
      if (h) {
        r.x = n->side == "right" ? e[2] + gap : e[0] - gap - r.w;
        r.y = std::max(n->dot.y - r.h / 2, next), next = r.y + r.h + 8;
      } else {
        r.y = n->side == "top" ? e[1] - gap - r.h : e[3] + gap;
        r.x = std::max(n->dot.x - r.w / 2, next), next = r.x + r.w + 8;
      }
      bb[0] = std::min(bb[0], r.x), bb[1] = std::min(bb[1], r.y);
      bb[2] = std::max(bb[2], r.x + r.w), bb[3] = std::max(bb[3], r.y + r.h);
    }
  }
}

inline void note(ShapeContainer& o, const Note& n) {
  const Item& st = *n.st;
  const CONNECT::Rect& r = n.box;
  std::string line = st.attr("stroke", "callout-border");
  std::string near = n.side == "right" ? "left" : n.side == "left" ? "right" : n.side == "top" ? "bottom" : "top";
  CONNECT::Pt p = CONNECT::point(r, {near, 0.5});
  paint(o.line(n.dot.x, n.dot.y, p.x, p.y), "", line, 1);
  paint(o.ellipse(n.dot.x, n.dot.y, 3, 3), line, "none", 0);
  paint(o.rect(r.x, r.y, r.w, r.h, st.num("radius", 4)), st.attr("fill", "callout"), line, 1);
  label(o, st, st.text, r.x + st.num("padx", 12), r.y + r.h / 2, face(st, "start", st.attr("color", "callout-ink")));
}

}
