#pragma once
#include <string>
#include <vector>

namespace TEXT {

static const int HELV[95] = {
  278, 278, 355, 556, 556, 889, 667, 191, 333, 333, 389, 584, 278, 333, 278, 278,
  556, 556, 556, 556, 556, 556, 556, 556, 556, 556, 278, 278, 584, 584, 584, 556,
  1015, 667, 667, 722, 722, 667, 611, 778, 722, 278, 500, 667, 556, 833, 722, 778,
  667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 278, 278, 278, 469, 556,
  333, 556, 556, 500, 556, 556, 278, 556, 556, 222, 222, 500, 222, 833, 556, 556,
  556, 556, 333, 500, 278, 556, 500, 722, 500, 500, 500, 334, 260, 334, 584
};

inline unsigned next(const std::string& s, size_t& i) {
  unsigned char c = s[i++];
  int n = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : 0;
  if (c >= 0xF8 || (c >= 0x80 && c < 0xC0)) return c;
  unsigned cp = n == 0 ? c : c & (0x3F >> n);
  for (int k = 0; k < n; k++) {
    if (i >= s.size() || (static_cast<unsigned char>(s[i]) & 0xC0) != 0x80) return c;
    cp = (cp << 6) | (static_cast<unsigned char>(s[i++]) & 0x3F);
  }
  return cp;
}

inline int ulen(const std::string& s) {
  int n = 0;
  for (size_t i = 0; i < s.size(); n++) next(s, i);
  return n;
}

inline bool est = false;

inline int charUnits(unsigned cp, bool bold = false) {
  if (est) return bold ? 600 : 560;
  int w = (cp >= 32 && cp <= 126) ? HELV[cp - 32]
        : (cp >= 0x2E80 && cp <= 0xFFEF) || cp >= 0x1F000 ? 1000
        : 556;
  return bold ? (w * 107 + 50) / 100 : w;
}

inline int units(const std::string& s, bool bold = false) {
  int u = 0;
  for (size_t i = 0; i < s.size();) u += charUnits(next(s, i), bold);
  return u;
}

inline double width(const std::string& s, double size, bool bold = false) {
  return units(s, bold) * size / 1000.0;
}

inline std::vector<std::string> wrap(const std::string& s, double maxWidth, double size, bool bold = false) {
  std::vector<std::string> out;
  auto fits = [&](int u) { return u * size <= maxWidth * 1000.0; };
  const int sp = charUnits(' ', bold);
  size_t p = 0;
  while (p <= s.size()) {
    size_t e = s.find('\n', p);
    if (e == std::string::npos) e = s.size();
    std::string par = s.substr(p, e - p), line;
    int lu = 0;
    auto flush = [&]() { out.push_back(line); line.clear(); lu = 0; };
    auto place = [&](const std::string& w) {
      int wu = units(w, bold);
      if (!line.empty() && fits(lu + sp + wu)) { line += ' ' + w; lu += sp + wu; return; }
      if (!line.empty()) flush();
      if (fits(wu)) { line = w; lu = wu; return; }
      for (size_t i = 0; i < w.size();) {
        size_t j = i;
        int cu = charUnits(next(w, j), bold);
        if (!line.empty() && !fits(lu + cu)) flush();
        line += w.substr(i, j - i);
        lu += cu;
        i = j;
      }
    };
    for (size_t i = 0; i <= par.size();) {
      size_t j = par.find(' ', i);
      if (j == std::string::npos) j = par.size();
      if (j > i) place(par.substr(i, j - i));
      i = j + 1;
    }
    out.push_back(line);
    p = e + 1;
  }
  return out;
}

}
