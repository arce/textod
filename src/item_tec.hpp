#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>
#include "dsl_tec.hpp"
#include "expr_tec.hpp"
#include "text_tec.hpp"
#include "theme_tec.hpp"

namespace FIG {

using Style = std::map<std::string, std::vector<const DSL::Node*>>;

inline Style style(const DSL::Node& tree, const std::string& names) {
  Style s;
  for (auto& name : DSL::list(names))
    for (auto& n : tree.kids) {
      auto w = n.pos();
      if (n.head == "style" && !w.empty() && w[0] == name)
        for (auto& k : n.kids) s[k.head].push_back(&k);
    }
  return s;
}

inline const std::map<std::string, bool> KINDS{{"row", true},  {"col", true},  {"group", true}, {"grid", true}, {"layer", true},
                                               {"canvas", true}, {"box", false}, {"text", false}, {"spacer", false}, {"shape", false}};

inline bool container(const std::string& kind) {
  auto f = KINDS.find(kind);
  return f != KINDS.end() && f->second;
}

inline const std::set<std::string>& flags() {
  static const std::set<std::string> f{"bold", "italic", "mono", "stretch", "open", "elbow", "both", "wrap", "frame", "float",
                                       "left", "right", "center", "top", "bottom", "grow", "flex", "curve", "full", "avoid"};
  return f;
}

inline bool reserved(const std::string& w) { return flags().count(w) || THEME::find(w); }

struct Item {
  std::string kind, id, text;
  const DSL::Node* node = nullptr;
  const Style* sheet = nullptr;
  std::vector<Item> kids;
  double x = 0, y = 0, w = 0, h = 0, fs = 16;

  std::vector<const DSL::Node*> layers() const {
    std::vector<const DSL::Node*> r;
    if (sheet && sheet->count(kind)) r = sheet->at(kind);
    r.push_back(node);
    return r;
  }
  bool has(const std::string& k) const {
    for (auto n : layers()) if (n->has(k)) return true;
    return false;
  }
  std::string attr(const std::string& k, const std::string& d = "") const {
    auto l = layers();
    for (auto n = l.rbegin(); n != l.rend(); ++n) if ((*n)->has(k)) return (*n)->attr(k);
    return d;
  }
  double num(const std::string& k, double d, const EXPR::Vars& v = {}) const {
    std::string s = attr(k);
    return s.empty() ? d : EXPR::eval(s, v);
  }
  std::vector<std::string> words() const {
    std::vector<std::string> r;
    bool skip = !id.empty() && !node->has("id");
    for (auto& t : node->args) {
      if (!t.key.empty() || t.q) continue;
      if (skip && t.v == id) { skip = false; continue; }
      r.push_back(t.v);
    }
    if (sheet && sheet->count(kind))
      for (auto n : sheet->at(kind)) for (auto& t : n->args) if (t.key.empty() && !t.q) r.push_back(t.v);
    return r;
  }
  bool flag(const std::string& f) const {
    for (auto& s : words()) if (s == f) return true;
    return false;
  }
  bool arrow() const {
    auto is = [](const std::string& s) { return s == "--" || s.compare(0, 2, "->") == 0; };
    if (kind == "arrow" || kind == "callout" || is(kind)) return true;
    for (auto& t : node->args) if (t.key.empty() && !t.q && is(t.v)) return true;
    return false;
  }
  std::string fillName(const std::string& d = "") const {
    if (has("fill")) return attr("fill");
    for (auto& s : words()) if (THEME::find(s)) return s;
    return d;
  }
};

inline Item build(const DSL::Node& n, const Style& st, double fs = 16) {
  Item it;
  it.kind = n.head;
  it.node = &n;
  it.sheet = &st;
  bool first = true;
  for (auto& t : n.args) {
    if (!t.key.empty()) continue;
    if (t.q) {
      if (it.text.empty()) it.text = t.v;
      continue;
    }
    if (first && !reserved(t.v)) it.id = t.v;
    first = false;
  }
  if (n.has("id")) it.id = n.attr("id");
  it.fs = it.has("size") ? EXPR::eval(it.attr("size")) : fs;
  for (auto& k : n.kids) it.kids.push_back(build(k, st, it.fs));
  return it;
}

struct Block {
  std::vector<std::string> lines;
  double w = 0, h = 0, lh = 0;
};

inline double factor(const Item& it) { return it.flag("mono") ? 1.1 : 1; }

inline Block block(const Item& it, const std::string& s, double size, bool bold, double maxw = 0) {
  Block b;
  b.lh = size * 1.25;
  for (size_t p = 0; !s.empty() && p <= s.size();) {
    size_t e = s.find('\n', p);
    std::string line = s.substr(p, e == std::string::npos ? e : e - p);
    if (maxw > 0) for (auto& l : TEXT::wrap(line, maxw / factor(it), size, bold)) b.lines.push_back(l);
    else b.lines.push_back(line);
    if (e == std::string::npos) break;
    p = e + 1;
  }
  for (auto& l : b.lines) b.w = std::max(b.w, TEXT::width(l, size, bold) * factor(it));
  b.h = b.lines.size() * b.lh;
  return b;
}

}
