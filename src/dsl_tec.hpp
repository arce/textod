#pragma once
#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace DSL {

struct Error : std::runtime_error {
  int line;
  std::string text;
  Error(int ln, const std::string& msg)
    : std::runtime_error((ln ? "line " + std::to_string(ln) + ": " : "") + msg), line(ln), text(msg) {}
};

struct Tok {
  std::string key, v;
  bool q = false;
};

struct Node {
  std::string head;
  std::vector<Tok> args;
  std::vector<Node> kids;
  int line = 0;
  int indent = 0;

  bool has(const std::string& k) const {
    for (auto& t : args) if (t.key == k) return true;
    return false;
  }
  std::string attr(const std::string& k, const std::string& d = "") const {
    for (auto& t : args) if (t.key == k) return t.v;
    return d;
  }
  bool flag(const std::string& w) const {
    for (auto& t : args) if (t.key.empty() && !t.q && t.v == w) return true;
    return false;
  }
  std::vector<std::string> pos() const {
    std::vector<std::string> r;
    for (auto& t : args) if (t.key.empty()) r.push_back(t.v);
    return r;
  }
};

using Nodes = std::vector<Node>;
using Vars = std::map<std::string, std::string>;
using Resolver = std::function<std::string(const std::string&)>;
using Macro = std::function<std::vector<Node>(const Node&, std::map<std::string, Node>&, const std::map<std::string, Node>&)>;
inline std::map<std::string, Macro>& macros() { static std::map<std::string, Macro> m; return m; }

inline bool wc(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

inline std::vector<std::vector<Tok>> split(const std::string& s, size_t i, int ln) {
  std::vector<std::vector<Tok>> out(1);
  auto sp = [](char c) { return c == ' ' || c == '\t'; };
  auto quoted = [&]() {
    std::string v;
    for (i++;; i++) {
      if (i >= s.size()) throw Error(ln, "the string is not closed");
      if (s[i] == '"') break;
      if (s[i] == '\\' && i + 1 < s.size()) { i++; v += s[i] == 'n' ? '\n' : s[i]; }
      else v += s[i];
    }
    i++;
    for (;;) {
      size_t j = i;
      while (j < s.size() && sp(s[j])) j++;
      if (j >= s.size() || s[j] != '&' || (j + 1 < s.size() && !sp(s[j + 1]))) break;
      j++;
      while (j < s.size() && sp(s[j])) j++;
      if (j >= s.size() || s[j] != '"') break;
      i = j;
      for (i++;; i++) {
        if (i >= s.size()) throw Error(ln, "the string is not closed");
        if (s[i] == '"') break;
        if (s[i] == '\\' && i + 1 < s.size()) { i++; v += s[i] == 'n' ? '\n' : s[i]; }
        else v += s[i];
      }
      i++;
    }
    return v;
  };
  while (i < s.size()) {
    char c = s[i];
    if (sp(c)) i++;
    else if (c == '#') break;
    else if (c == ';') { out.emplace_back(); i++; }
    else if (c == '"') { Tok t; t.v = quoted(); t.q = true; out.back().push_back(t); }
    else {
      size_t j = i;
      while (j < s.size() && !sp(s[j]) && s[j] != ';' && s[j] != '"') j++;
      std::string w = s.substr(i, j - i);
      size_t e = w.find('=');
      i = j;
      Tok t;
      if (e != std::string::npos && e > 0 && std::all_of(w.begin(), w.begin() + e, wc)) {
        t.key = w.substr(0, e);
        if (e + 1 == w.size() && i < s.size() && s[i] == '"') { t.v = quoted(); t.q = true; }
        else t.v = w.substr(e + 1);
      } else t.v = w;
      out.back().push_back(t);
    }
  }
  return out;
}

inline std::vector<Tok> splitRow(const std::string& s, size_t i, int ln) {
  std::vector<Tok> out;
  std::string cur;
  bool quoted = false, inq = false, any = false;
  auto flush = [&]() {
    if (!quoted) {
      size_t a = cur.find_first_not_of(" \t"), b = cur.find_last_not_of(" \t");
      cur = a == std::string::npos ? "" : cur.substr(a, b - a + 1);
    }
    Tok t;
    t.v = cur;
    t.q = true;
    out.push_back(t);
    cur.clear();
    quoted = inq = false;
  };
  for (; i < s.size(); i++) {
    char c = s[i];
    if (inq) {
      if (c == '"') inq = false;
      else if (c == '\\' && i + 1 < s.size()) { i++; cur += s[i] == 'n' ? '\n' : s[i]; }
      else cur += c;
    } else if (c == '"' && cur.find_first_not_of(" \t") == std::string::npos) { cur.clear(); inq = quoted = true; }
    else if (c == ',') flush();
    else if (!quoted || (c != ' ' && c != '\t')) cur += c;
    any = true;
  }
  if (inq) throw Error(ln, "the string is not closed");
  if (any) flush();
  return out;
}

inline std::string cut(const std::string& s) {
  bool q = false;
  size_t e = s.size();
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\\' && q) i++;
    else if (s[i] == '"') q = !q;
    else if (s[i] == '#' && !q && (i == 0 || s[i - 1] == ' ' || s[i - 1] == '\t')) { e = i; break; }
  }
  size_t b = s.find_last_not_of(" \t\r", e == 0 ? 0 : e - 1);
  return e == 0 || b == std::string::npos ? "" : s.substr(0, b + 1);
}

inline bool opens(const std::string& c) { return !c.empty() && c.back() == '(' && (c.size() == 1 || c[c.size() - 2] == ' '); }
inline bool closes(const std::string& c) { return !c.empty() && c.back() == ')' && (c.size() == 1 || c[c.size() - 2] == ' '); }

inline Nodes csvRows(const std::string& text, const std::string& name) {
  Nodes rows;
  size_t p = text.compare(0, 3, "\xEF\xBB\xBF") == 0 ? 3 : 0;
  int k = 0;
  while (p <= text.size()) {
    size_t e = text.find('\n', p);
    if (e == std::string::npos) e = text.size();
    std::string l = text.substr(p, e - p);
    p = e + 1;
    k++;
    if (!l.empty() && l.back() == '\r') l.pop_back();
    if (l.find_first_not_of(" \t") == std::string::npos) continue;
    rows.emplace_back();
    rows.back().head = "row";
    rows.back().line = k;
    try { rows.back().args = splitRow(l, 0, k); }
    catch (const Error& x) { throw Error(0, "file '" + name + "', line " + std::to_string(k) + ": " + x.text); }
  }
  return rows;
}

inline std::set<std::string>& raws() { static std::set<std::string> r{"data"}; return r; }

inline Node parse(const std::string& text) {
  Node root;
  std::vector<std::pair<int, Node*>> st{{-1, &root}};
  size_t p = 0;
  int ln = 0;
  while (p <= text.size()) {
    size_t e = text.find('\n', p);
    if (e == std::string::npos) e = text.size();
    std::string s = text.substr(p, e - p);
    p = e + 1;
    ln++;
    const int at = ln;
    size_t ind = s.find_first_not_of(' ');
    if (ind == std::string::npos) continue;
    // `#line N`: the next line of this text is the line N of the file it came from
    if (s.compare(ind, 6, "#line ") == 0 && s.size() > ind + 6 && s.size() <= ind + 6 + 7 &&
        s.find_first_not_of("0123456789", ind + 6) == std::string::npos) {
      ln = std::stoi(s.substr(ind + 6)) - 1;
      continue;
    }
    if (s[ind] == '\t') throw Error(ln, "tabs are not allowed in the indentation");
    {
      size_t k = st.size() - 1;
      while (st[k].first > (int)ind) k--;
      if (st[k].first == (int)ind) k--;
      Node* par = st[k].second;
      if (raws().count(par->head) && par != &root) {
        if (s[ind] == '#' && (ind + 1 == s.size() || s[ind + 1] == ' ')) continue;
        par->kids.emplace_back();
        Node& r = par->kids.back();
        r.head = "row";
        r.line = ln;
        r.indent = (int)ind;
        r.args = splitRow(s, ind, ln);
        continue;
      }
    }
    {
      std::string c = cut(s);
      if (opens(c)) {
        std::string joined = c.substr(0, c.size() - 1);
        bool closed = false;
        while (!closed && p <= text.size()) {
          size_t f = text.find('\n', p);
          if (f == std::string::npos) f = text.size();
          std::string t = cut(text.substr(p, f - p));
          p = f + 1;
          ln++;
          size_t a = t.find_first_not_of(" \t");
          if (a == std::string::npos) continue;
          t = t.substr(a);
          if (closes(t)) { closed = true; t = t.substr(0, t.size() - 1); a = t.find_last_not_of(" \t"); t = a == std::string::npos ? "" : t.substr(0, a + 1); }
          if (!t.empty()) joined += " " + t;
        }
        if (!closed) throw Error(at, "the '(' is not closed");
        s = joined;
      }
    }
    for (auto& it : split(s, ind, at)) {
      if (it.empty()) continue;
      if (it[0].q || !it[0].key.empty()) throw Error(at, "a name was expected at the start of the item");
      bool popped = false;
      while (st.back().first > (int)ind) { st.pop_back(); popped = true; }
      if (popped && st.back().first != (int)ind) throw Error(at, "the indentation does not match any outer level");
      if (st.back().first == (int)ind) st.pop_back();
      Node& par = *st.back().second;
      par.kids.emplace_back();
      Node& n = par.kids.back();
      n.head = it[0].v;
      n.line = at;
      if (n.head == "data") {
        for (size_t k = 1; k < it.size(); k++) {
          if (it[k].q || !it[k].key.empty() || it[k].v.find(',') == std::string::npos) { n.args.push_back(it[k]); continue; }
          size_t a = 0;
          for (;;) {
            size_t c = it[k].v.find(',', a);
            std::string w = it[k].v.substr(a, c == std::string::npos ? c : c - a);
            if (!w.empty()) { Tok t; t.v = w; n.args.push_back(t); }
            if (c == std::string::npos) break;
            a = c + 1;
          }
        }
      } else n.args.assign(it.begin() + 1, it.end());
      st.push_back({(int)ind, &n});
    }
  }
  return root;
}

inline std::string suggest(const std::string& w, const std::vector<std::string>& names) {
  std::string best;
  int bd = 3;
  for (auto& n : names) {
    std::vector<int> d(n.size() + 1);
    for (size_t j = 0; j <= n.size(); j++) d[j] = (int)j;
    for (size_t i = 1; i <= w.size(); i++) {
      int prev = d[0];
      d[0] = (int)i;
      for (size_t j = 1; j <= n.size(); j++) {
        int cur = d[j];
        d[j] = std::min({d[j] + 1, d[j - 1] + 1, prev + (w[i - 1] != n[j - 1])});
        prev = cur;
      }
    }
    if (d[n.size()] < bd && d[n.size()] < (int)w.size()) { bd = d[n.size()]; best = n; }
  }
  return best.empty() ? "" : " (did you mean '" + best + "'?)";
}

inline std::string sub(const std::string& s, const Vars& v) {
  std::string r;
  for (size_t i = 0; i < s.size();) {
    if (s[i] == '$' && i + 1 < s.size() && wc(s[i + 1])) {
      size_t j = i + 1;
      while (j < s.size() && wc(s[j])) j++;
      auto it = v.find(s.substr(i + 1, j - i - 1));
      if (it != v.end()) { r += it->second; i = j; continue; }
    }
    r += s[i++];
  }
  return r;
}

// The expander (def, data, repeat, if, import, content, embedded screens, links) is a separate
// header: including expander_tec.hpp registers it here. Without it a text goes through as it is:
// only an `import` of a library of styles is accepted (its `style` blocks come first).
using Expand = std::function<Node(const Node&, const Resolver&)>;
inline Expand& expander() { static Expand e; return e; }

inline Node plain(const Node& tree, const Resolver& r, std::set<std::string>& seen) {
  static const std::set<std::string> needs{"def", "data", "repeat", "if", "content"};
  Node out;
  Nodes styles, rest;
  for (auto& n : tree.kids) {
    if (n.head == "import") {
      auto w = n.pos();
      std::string name = w.empty() ? "" : w[0];
      if (seen.count(name)) continue;
      seen.insert(name);
      std::string text = r ? r(name) : "";
      if (text.empty()) throw Error(n.line, "import '" + name + "': no such library");
      Node lib;
      try {
        lib = plain(parse(text), r, seen);
      } catch (const Error& e) {
        if (e.text.find("needs the expander") != std::string::npos)
          throw Error(n.line, "import '" + name + "': the library has more than styles, it needs the expander module, which is not loaded");
        throw;
      }
      for (auto& k : lib.kids) {
        if (k.head != "style")
          throw Error(n.line, "import '" + name + "': the library has more than styles, it needs the expander module, which is not loaded");
        styles.push_back(k);
      }
    } else if (needs.count(n.head)) {
      throw Error(n.line, "'" + n.head + "' needs the expander module, which is not loaded");
    } else {
      rest.push_back(n);
    }
  }
  out.kids = styles;
  for (auto& n : rest) out.kids.push_back(n);
  return out;
}

inline Node compile(const std::string& text, const Resolver& r = nullptr) {
  Node t = parse(text);
  if (expander()) return expander()(t, r);
  std::set<std::string> seen;
  return plain(t, r, seen);
}

inline bool bare(const Tok& t) { return t.key.empty() && !t.q; }

inline std::string quote(const std::string& v) {
  std::string r = "\"";
  for (char c : v) r += c == '"' || c == '\\' ? std::string("\\") + c : c == '\n' ? "\\n" : std::string(1, c);
  return r + "\"";
}

inline std::string val(const Tok& t) { return t.q || t.v.empty() || t.v.find_first_of(" ;\"#") != std::string::npos ? quote(t.v) : t.v; }

inline std::string tok(const Tok& t) { return (t.key.empty() ? "" : t.key + "=") + val(t); }

inline std::vector<std::string> list(const std::string& s, char sep = ',') {
  std::vector<std::string> r;
  std::string cur;
  for (char c : s) { if (c == sep) { if (!cur.empty()) r.push_back(cur); cur.clear(); } else cur += c; }
  if (!cur.empty()) r.push_back(cur);
  return r;
}

inline void once(std::vector<std::string>& v, const std::string& s) {
  if (std::find(v.begin(), v.end(), s) == v.end()) v.push_back(s);
}

// The plain text that the expander hands to the next level: like show(), with a
// `#line N` wherever a line is not the one that follows the last.
inline std::string emit(const Node& n, int d = 0, int* next = nullptr) {
  int local = 1;
  if (!next) next = &local;
  std::string r;
  for (auto& k : n.kids) {
    if (k.line > 0 && k.line != *next) r += "#line " + std::to_string(k.line) + "\n";
    *next = (k.line > 0 ? k.line : *next) + 1;
    r += std::string(d * 2, ' ') + k.head;
    for (auto& t : k.args) r += " " + tok(t);
    r += "\n" + emit(k, d + 1, next);
  }
  return r;
}

inline std::string show(const Node& n, int d = 0) {
  std::string r;
  for (auto& k : n.kids) {
    r += std::string(d * 2, ' ') + k.head;
    for (auto& t : k.args) r += " " + tok(t);
    r += "\n" + show(k, d + 1);
  }
  return r;
}

}
