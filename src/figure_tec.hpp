#pragma once
#include "draw_tec.hpp"
#include "layout_tec.hpp"

namespace FIGURE {

using DSL::Error;
using FIG::Item;

struct Out {
  std::string id, svg;
};

inline const std::set<std::string> ELEMENTS = [] {
  std::set<std::string> s;
  for (auto& k : FIG::KINDS) s.insert(k.first);
  return s;
}();
inline const std::set<std::string> PRIMS{"rect", "ellipse", "line", "path", "poly", "set", "label"};
inline const std::set<std::string> ATTRS_CORE{
  "id", "w", "h", "minw", "minh", "gap", "pad", "padx", "pady", "align", "cols", "span", "at", "alignx", "aligny", "x", "y",
  "fill", "stroke", "sw", "dash", "radius", "size", "color", "style", "caption", "title", "links"};
inline const std::set<std::string> ATTRS_SHAPE{"r", "cx", "cy", "rx", "ry", "x1", "y1", "x2", "y2", "d", "pts", "tx", "ty"};
inline const std::set<std::string> ATTRS_EXT{
  "alpha", "rotate", "border", "capsize", "captionpos", "captionstyle", "rspan", "layout", "label", "side", "margin", "linecap", "join",
  "theme", "gapx", "gapy", "base", "weights", "metric", "valign", "around", "padtop", "tip", "tail", "flex", "shadow", "tooltip", "hover"};
inline const std::set<std::string> ATTRS = [] {
  std::set<std::string> s = ATTRS_CORE;
  s.insert(ATTRS_SHAPE.begin(), ATTRS_SHAPE.end());
  s.insert(ATTRS_EXT.begin(), ATTRS_EXT.end());
  return s;
}();

[[noreturn]] inline void unknown(const Item& it, const std::string& what, const std::string& w, const std::set<std::string>& ok,
                                 const std::vector<std::string>& more = {}) {
  std::vector<std::string> names(ok.begin(), ok.end());
  names.insert(names.end(), more.begin(), more.end());
  throw Error(it.node->line, "unknown " + what + " '" + w + "'" + DSL::suggest(w, names));
}

inline void check(const Item& it, bool inShape = false) {
  if (!it.arrow()) {
    if (inShape && ELEMENTS.count(it.kind)) throw Error(it.node->line, "'" + it.kind + "' cannot go inside a shape");
    if (!inShape && PRIMS.count(it.kind)) throw Error(it.node->line, "'" + it.kind + "' only goes inside a shape");
    const auto& ok = inShape ? PRIMS : ELEMENTS;
    if (!ok.count(it.kind)) unknown(it, "element", it.kind, ok);
    for (auto& w : it.words())
      if (!FIG::flags().count(w) && !THEME::find(w)) unknown(it, "word", w, FIG::flags(), THEME::names());
  }
  if (it.kind != "set" && it.kind != "shape")
    for (auto& t : it.node->args) if (!t.key.empty() && !ATTRS.count(t.key)) unknown(it, "attribute", t.key, ATTRS);
  for (auto& k : it.kids) check(k, it.kind == "shape");
}

struct Ref {
  std::string name, side, at;
  int nth = 0;
};

struct Stmt {
  Item* st;
  std::vector<std::vector<Ref>> ends;
  std::vector<bool> plain;
  bool sibPlain = false;
  Item *prev = nullptr, *next = nullptr;
};

inline Ref parseRef(const Item& st, std::string t) {
  Ref r;
  size_t a = t.find('@');
  if (a != std::string::npos) { r.at = t.substr(a + 1); t.resize(a); }
  size_t d = t.rfind('.');
  if (d != std::string::npos) {
    r.side = t.substr(d + 1);
    if (r.side != "top" && r.side != "bottom" && r.side != "left" && r.side != "right")
      throw Error(st.node->line, "'" + r.side + "' is not a side (top, bottom, left or right)");
    t.resize(d);
  }
  size_t m = t.find('~');
  if (m != std::string::npos) {
    std::string n = t.substr(m + 1);
    if (n.empty() || n.find_first_not_of("0123456789") != std::string::npos) throw Error(st.node->line, "~ must be followed by a number");
    r.nth = std::stoi(n);
    t.resize(m);
  }
  r.name = t;
  return r;
}

inline Stmt parseArrow(Item& st) {
  Stmt s{&st, {}};
  const DSL::Node& n = *st.node;
  std::vector<DSL::Tok> seq;
  bool sibling = n.head == "->" || n.head == "--";
  s.sibPlain = n.head == "--";
  bool note = n.head == "callout";
  if (!sibling && !note && n.head != "arrow") {
    DSL::Tok h;
    h.v = n.head;
    seq.push_back(h);
  }
  for (auto& t : n.args) if (t.key.empty()) seq.push_back(t);
  size_t i = 0;
  auto group = [&](const std::string& v) {
    std::vector<Ref> g;
    for (size_t p = 0; p <= v.size();) {
      size_t c = v.find(',', p);
      if (c == std::string::npos) c = v.size();
      g.push_back(parseRef(st, v.substr(p, c - p)));
      p = c + 1;
    }
    s.ends.push_back(g);
  };
  if (note) {
    if (seq.empty() || seq[0].q) throw Error(n.line, "the callout needs the element it points to");
    s.ends.push_back({parseRef(st, seq[0].v)});
    i = 1;
  } else if (!sibling) {
    if (seq.empty() || seq[0].q) throw Error(n.line, "the arrow needs the element it starts from");
    group(seq[0].v);
    for (i = 1; i + 1 < seq.size() && !seq[i].q && (seq[i].v == "->" || seq[i].v == "--"); i += 2) {
      s.plain.push_back(seq[i].v == "--");
      if (seq[i + 1].q) throw Error(n.line, "the arrow needs an element after '->'");
      group(seq[i + 1].v);
    }
    if (s.ends.size() < 2) throw Error(n.line, "the arrow needs two ends");
  }
  for (; i < seq.size(); i++)
    if (!seq[i].q && !FIG::flags().count(seq[i].v)) unknown(st, "word", seq[i].v, FIG::flags());
  return s;
}

inline void collect(Item& parent, std::vector<Stmt>& out) {
  auto real = [&](int from, int step) -> Item* {
    for (int j = from; j >= 0 && j < static_cast<int>(parent.kids.size()); j += step)
      if (!parent.kids[j].arrow()) return &parent.kids[j];
    return nullptr;
  };
  for (size_t i = 0; i < parent.kids.size(); i++) {
    Item& k = parent.kids[i];
    if (!k.arrow()) { collect(k, out); continue; }
    out.push_back(parseArrow(k));
    k.id.clear();
    if (k.kind != "callout") k.kind = "arrow";
    if (out.back().ends.empty()) {
      out.back().prev = real(static_cast<int>(i) - 1, -1);
      out.back().next = real(static_cast<int>(i) + 1, 1);
      if (!out.back().prev || !out.back().next) throw Error(k.node->line, "an arrow between siblings needs an element before and after it");
    }
  }
}

using Ids = std::map<std::string, Item*>;

inline void index(Item& parent, Ids& ids) {
  for (auto& k : parent.kids) {
    if (k.arrow()) continue;
    if (!k.id.empty() && !ids.emplace(k.id, &k).second) throw Error(k.node->line, "duplicate id '" + k.id + "'");
    index(k, ids);
  }
}


inline bool chainTo(const Item& cur, const Item* t, std::vector<const Item*>& out) {
  out.push_back(&cur);
  if (&cur == t) return true;
  for (auto& k : cur.kids) if (!k.arrow() && chainTo(k, t, out)) return true;
  out.pop_back();
  return false;
}

inline void levelItems(const Item& cur, size_t depth, size_t want, std::vector<const Item*>& out) {
  for (auto& k : cur.kids) {
    if (k.arrow() || k.flag("float")) continue;
    if (depth + 1 == want) out.push_back(&k);
    else levelItems(k, depth + 1, want, out);
  }
}

// An `avoid` arrow (a link from a button to another screen) leaves the border of the screen that holds the button,
// enters the border of the destination and goes around every other screen, which are the items at the destination's depth.
struct Detour {
  DRAW::End a, b;
  std::vector<CONNECT::Rect> obs;
  size_t screens = 0;
  double key = 0;
  bool on = false;
};

inline Detour detour(const Item& root, DRAW::End a, DRAW::End b) {
  Detour d;
  d.a = a, d.b = b;
  std::vector<const Item*> ca, cb;
  if (!chainTo(root, a.item, ca) || !chainTo(root, b.item, cb) || ca.size() < cb.size()) return d;
  const Item* lift = ca[cb.size() - 1];
  if (lift == b.item) return d;
  std::vector<const Item*> lv;
  levelItems(root, 0, cb.size() - 1 + 1, lv);
  for (auto* k : lv) d.obs.push_back(DRAW::rect(*k));
  d.screens = d.obs.size();
  if (cb.size() >= 3)  // what else the diagram holds (title, footer) is not crossed either
    for (auto& k : cb[cb.size() - 3]->kids)
      if (&k != cb[cb.size() - 2] && !k.arrow() && !k.flag("float") && k.w > 0 && k.h > 0) d.obs.push_back(DRAW::rect(k));
  CONNECT::Rect ra = DRAW::rect(*lift), rb = DRAW::rect(*b.item), rs = DRAW::rect(*a.item);
  CONNECT::Sides f = CONNECT::facing(ra, rb);
  d.a.item = lift, d.a.side = f.a;
  d.b.side = f.b, d.b.at = 0.5;
  bool h = CONNECT::horiz(f.a);
  double len = h ? ra.h : ra.w;
  double c = h ? rs.y + rs.h / 2 - ra.y : rs.x + rs.w / 2 - ra.x;
  d.a.at = len > 0 ? std::min(0.9, std::max(0.1, c / len)) : 0.5;
  d.key = h ? rs.y : rs.x;
  d.on = true;
  return d;
}

inline Item& find(const Ids& ids, const Item& st, const std::string& id) {
  auto f = ids.find(id);
  if (f != ids.end()) return *f->second;
  std::vector<std::string> names;
  for (auto& e : ids) names.push_back(e.first);
  throw Error(st.node->line, (st.kind == "callout" ? "callout" : "arrow") + std::string(": there is no element '") + id + "'" + DSL::suggest(id, names));
}

inline DRAW::End endOf(const Ids& ids, const Item& st, const Ref& r) {
  Item* it = &find(ids, st, r.name);
  if (r.nth) {
    auto ks = LAYOUT::flow(*it);
    if (r.nth > static_cast<int>(ks.size())) throw Error(st.node->line, "arrow: '" + r.name + "' has no element number " + std::to_string(r.nth));
    it = ks[r.nth - 1];
  }
  DRAW::End e;
  e.item = it;
  e.side = r.side;
  if (r.at.empty()) return e;
  if (r.at[0] == '=') {
    if (r.side.empty()) throw Error(st.node->line, "@=id needs a side, as in a.left@=b");
    const Item* o = &find(ids, st, r.at.substr(1));
    bool v = r.side == "left" || r.side == "right";
    double len = v ? it->h : it->w;
    e.at = len > 0 ? ((v ? o->y + o->h / 2 - it->y : o->x + o->w / 2 - it->x) / len) : 0.5;
  } else if (LAYOUT::pct(r.at)) {
    e.at = LAYOUT::ev(st, "@", r.at.substr(0, r.at.size() - 1)) / 100;
  } else throw Error(st.node->line, "@ must be followed by a percentage or =id");
  e.at = std::min(1.0, std::max(0.0, e.at));
  return e;
}

inline DRAW::Note noteAt(const Ids& ids, const Stmt& s) {
  Ref r = s.ends[0][0];
  std::string side = s.st->attr("side", r.side.empty() ? "right" : r.side);
  if (r.side.empty()) r.side = side;
  DRAW::End e = endOf(ids, *s.st, r);
  return {s.st, side, CONNECT::point(DRAW::rect(*e.item), {e.side, e.at}), {}};
}

inline void bounds(const Item& it, double& x0, double& y0, double& x1, double& y1) {
  for (auto& k : it.kids) {
    if (k.arrow() || k.flag("float")) continue;
    x0 = std::min(x0, k.x);
    y0 = std::min(y0, k.y);
    x1 = std::max(x1, k.x + k.w);
    y1 = std::max(y1, k.y + k.h);
    bounds(k, x0, y0, x1, y1);
  }
}

inline bool implicitColumn(const Item& root) {
  size_t flows = 0;
  for (auto& k : root.kids) {
    if (k.arrow()) continue;
    if (k.has("at") || k.has("alignx") || k.has("aligny") || k.has("around")) return false;
    flows++;
  }
  return flows >= 2;
}

// The groups of the marks of the figure that ask for `hover=`.
inline void hovers(const Item& it, std::set<std::string>& groups) {
  if (DRAW::interactive(it)) groups.insert(DRAW::group(it));
  for (auto& k : it.kids) hovers(k, groups);
}

inline void theme(SVG::Svg& svg, const DSL::Node& n, const std::string& names, const DSL::Resolver& res) {
  std::string css = THEME::css(), cls;
  for (auto& name : DSL::list(names)) {
    std::string file = name.find('.') == std::string::npos ? name + ".css" : name, sheet = res ? res(file) : "";
    if (sheet.empty()) throw Error(n.line, "unknown theme '" + name + "' (there is no " + file + ")");
    css += "\n" + sheet;
    cls += (cls.empty() ? "theme-" : " theme-") + file.substr(0, file.rfind('.'));
  }
  svg.style(css).cls(cls);
}

inline Out block(const DSL::Node& tree, const DSL::Node& n, const DSL::Resolver& res, const std::string& dflt) {
  std::string sn = n.attr("style");
  if (!sn.empty()) {
    std::vector<std::string> names;
    for (auto& k : tree.kids) if (k.head == "style" && !k.pos().empty()) names.push_back(k.pos()[0]);
    for (auto& one : DSL::list(sn))
      if (std::find(names.begin(), names.end(), one) == names.end())
        throw Error(n.line, "unknown style '" + one + "'" + DSL::suggest(one, names));
  }
  FIG::Style st = FIG::style(tree, sn);
  Item root = FIG::build(n, st);
  root.kind = implicitColumn(root) ? "col" : "canvas";
  check(root);
  struct Est {
    bool old = TEXT::est;
    ~Est() { TEXT::est = old; }
  } est;
  TEXT::est = root.attr("metric") == "fixed";
  std::stable_partition(root.kids.begin(), root.kids.end(), [](const Item& k) { return k.has("around"); });
  Ids ids;
  std::vector<Stmt> arrows;
  index(root, ids);
  collect(root, arrows);
  LAYOUT::run(root, 0, 0);
  for (auto& k : root.kids) {
    if (!k.has("around")) continue;
    double a = 1e18, b = 1e18, c = -1e18, d = -1e18;
    for (auto& n : DSL::list(k.attr("around"))) {
      Item& t = find(ids, k, n);
      a = std::min(a, t.x), b = std::min(b, t.y), c = std::max(c, t.x + t.w), d = std::max(d, t.y + t.h);
    }
    double p = k.num("pad", 14), top = k.num("padtop", p);
    k.x = a - p, k.y = b - top, k.w = c - a + 2 * p, k.h = d - b + p + top;
  }
  double m = root.num("margin", 6), x0 = 0, y0 = 0, x1 = root.w, y1 = root.h;
  bounds(root, x0, y0, x1, y1);
  std::vector<DRAW::Note> notes;
  for (auto& s : arrows) if (s.st->kind == "callout") notes.push_back(noteAt(ids, s));
  std::map<const Item*, std::vector<Detour>> detours;
  {
    std::map<std::pair<const Item*, std::string>, std::vector<Detour*>> into;
    for (auto& s : arrows) {
      if (s.st->kind == "callout" || !s.st->flag("avoid")) continue;
      auto& v = detours[s.st];
      for (size_t i = 0; i + 1 < s.ends.size(); i++)
        for (auto& a : s.ends[i]) for (auto& b : s.ends[i + 1]) v.push_back(detour(root, endOf(ids, *s.st, a), endOf(ids, *s.st, b)));
    }
    for (auto& e : detours) for (auto& d : e.second) if (d.on) into[{d.b.item, d.b.side}].push_back(&d);
    for (auto& e : into) {
      auto& v = e.second;
      std::stable_sort(v.begin(), v.end(), [](Detour* x, Detour* y) { return x->key < y->key; });
      for (size_t i = 0; i < v.size(); i++) v[i]->b.at = (i + 1.0) / (v.size() + 1.0);
    }
  }
  std::vector<std::pair<CONNECT::Pt, CONNECT::Pt>> usedLanes;
  std::vector<CONNECT::Rect> usedLabels;
  auto drawArrows = [&](SVG::Group& g, double* bb) {
    usedLanes.clear();
    usedLabels.clear();
    for (auto& s : arrows) {
      if (s.st->kind == "callout") continue;
      try {
        std::vector<std::pair<DRAW::End, DRAW::End>> pairs;
        std::vector<bool> plain;
        if (s.ends.empty()) {
          pairs.push_back({DRAW::End{s.prev, "", 0.5}, DRAW::End{s.next, "", 0.5}});
          plain.push_back(s.sibPlain);
        }
        for (size_t i = 0; i + 1 < s.ends.size(); i++)
          for (auto& a : s.ends[i]) for (auto& b : s.ends[i + 1]) {
            pairs.push_back({endOf(ids, *s.st, a), endOf(ids, *s.st, b)});
            plain.push_back(s.plain[i]);
          }
        for (size_t i = 0; i < pairs.size(); i++) {
          auto f = detours.find(s.st);
          if (f != detours.end() && i < f->second.size() && f->second[i].on) {
            Detour& d = f->second[i];
            DRAW::arrow(g, d.a, d.b, *s.st, plain[i], bb, &d.obs, &usedLanes, &usedLabels, d.screens);
          } else DRAW::arrow(g, pairs[i].first, pairs[i].second, *s.st, plain[i], bb);
        }
      } catch (const Error&) {
        throw;
      } catch (const std::exception& e) {
        throw Error(s.st->node->line, e.what());
      }
    }
  };
  {
    SVG::Svg scratch(1, 1);
    double bb[4] = {x0, y0, x1, y1};
    drawArrows(scratch.group(), bb);
    const double e[4] = {bb[0], bb[1], bb[2], bb[3]};
    DRAW::place(notes, e, bb);
    x0 = bb[0], y0 = bb[1], x1 = bb[2], y1 = bb[3];
  }
  double w = x1 - x0 + 2 * m, h = y1 - y0 + 2 * m, total = h;
  std::string cap = root.attr("caption"), cs = root.attr("captionstyle", "italic");
  DRAW::Face face{root.num("capsize", root.fs), cs == "bold", cs == "italic", "start"};
  face.serif = true;
  FIG::Block cb = FIG::block(root, cap, face.size, face.bold);
  double capw = cb.w * 0.9356 + 4;
  if (!cap.empty()) total += 8 + cb.h + 4;
  w = std::max(w, capw);
  SVG::Svg svg(w, total);
  std::string th = n.attr("theme", dflt);
  if (!th.empty()) theme(svg, n, th, res);
  std::set<std::string> groups;
  hovers(root, groups);
  if (!groups.empty()) svg.addStyle(THEME::hoverCss(groups));
  if (root.flag("frame")) DRAW::paint(svg.rect(0.5, 0.5, w - 1, h - 1), "", "text", 1.0);
  if (root.has("fill")) DRAW::paint(svg.rect(1, 1, w - 2, h - 2), root.attr("fill"), root.attr("stroke", "none"), 1.0);
  SVG::Group& g = svg.group();
  g.translate(m - x0, m - y0);
  for (auto& k : root.kids) DRAW::item(g, k);
  drawArrows(g, nullptr);
  for (auto& n : notes) DRAW::note(g, n);
  if (!cap.empty()) DRAW::label(svg, root, cap, 2, h + 8 + cb.h / 2, face);
  return {root.id, svg.toString()};
}

inline std::vector<Out> render(const std::string& src, const DSL::Resolver& res = nullptr, const std::string& dflt = "") {
  DSL::Node tree = DSL::compile(src, res);
  std::vector<Out> out;
  for (auto& n : tree.kids) {
    if (n.head == "style") continue;
    if (n.head != "diagram") throw Error(n.line, "only 'diagram' goes at the top level, not '" + n.head + "'");
    out.push_back(block(tree, n, res, dflt));
  }
  return out;
}

}
