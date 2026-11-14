#pragma once
#include <string>
#include <vector>

namespace THEME {

struct Entry {
  const char* name;
  const char* color;
};

static const Entry PALETTE[] = {
  {"text", "#2c3e50"},
  {"muted", "#777777"},
  {"primary", "#2c81ba"},
  {"success", "#3eaf7c"},
  {"warning", "#e67e22"},
  {"primary-soft", "#eaf3fa"},
  {"success-soft", "#eef6f2"},
  {"warning-soft", "#fdf3ea"},
  {"border-soft", "#dcdcdc"},
  {"placeholder", "#d5dbe1"},
  {"bar", "#eef1f4"},
  {"border", "#b8c2cc"},
  {"primary-border", "#9cc3de"},
  {"surface", "#ffffff"},
  {"canvas", "#fafbfc"},
  {"light", "#dbe8f5"},
  {"blue", "#dbe8f5"},
  {"dark", "#2c81ba"},
  {"white", "#ffffff"},
  {"silver", "#e8ecef"},
  {"gray", "#e6e9ec"},
  {"green", "#d2ecd0"},
  {"orange", "#f8d9b4"},
  {"yellow", "#fbeeb0"},
  {"red", "#f3b7b0"},
  {"accent", "#e67e22"},
  {"callout", "#fefcf3"},
  {"callout-border", "#b8a26b"},
  {"callout-ink", "#3d3526"},
  {"shadow-color", "#9aa5b1"},
};

inline const Entry* find(const std::string& name) {
  for (auto& e : PALETTE) if (name == e.name) return &e;
  return nullptr;
}

inline std::vector<std::string> names() {
  std::vector<std::string> r;
  for (auto& e : PALETTE) r.push_back(e.name);
  return r;
}

inline bool isHex(const std::string& s) {
  if (s.size() != 7 || s[0] != '#') return false;
  for (size_t i = 1; i < s.size(); i++) {
    char c = s[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return false;
  }
  return true;
}

inline std::string color(const std::string& name) {
  if (name == "none" || isHex(name)) return name;
  auto e = find(name);
  return e ? e->color : "";
}

inline std::string cls(const std::string& fill, const std::string& stroke = "") {
  std::string r;
  if (find(fill)) r += "c-f-" + fill;
  if (find(stroke)) r += (r.empty() ? "" : " ") + std::string("c-s-") + stroke;
  return r;
}

inline std::string css() {
  std::string s = "svg { font-family: var(--c-font, Helvetica, Arial, sans-serif); }\n";
  for (auto& e : PALETTE) {
    std::string n = e.name;
    s += ".c-f-" + n + " { fill: var(--c-" + n + ", " + e.color + "); }\n";
    s += ".c-s-" + n + " { stroke: var(--c-" + n + ", " + e.color + "); }\n";
  }
  return s;
}

}
