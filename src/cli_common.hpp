#pragma once
// What the command-line programs share.
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "dsl_tec.hpp"

namespace cli {

inline bool read(const std::string& path, std::string& out) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return false;
  std::stringstream s;
  s << f.rdbuf();
  out = s.str();
  return true;
}

// a library by name in a directory or in one of its subdirectories
inline bool readLib(const std::string& dir, const std::string& n, std::string& out) {
  if (read(dir + "/" + n, out)) return true;
  std::vector<std::string> subs;
  std::error_code ec;
  for (auto& e : std::filesystem::directory_iterator(dir, ec)) {
    std::string s = e.path().filename().string();
    if (s[0] != '.' && s.find('.') == std::string::npos) subs.push_back(s);
  }
  std::sort(subs.begin(), subs.end());
  for (auto& s : subs) if (read(dir + "/" + s + "/" + n, out)) return true;
  return false;
}

inline std::string dirOf(const std::string& p) {
  size_t i = p.find_last_of('/');
  return i == std::string::npos ? "." : p.substr(0, i);
}

inline std::string stem(const std::string& p) {
  std::string f = p.substr(p.find_last_of('/') == std::string::npos ? 0 : p.find_last_of('/') + 1);
  size_t d = f.rfind('.');
  return d == std::string::npos || d == 0 ? f : f.substr(0, d);
}

// Libraries are looked for in the directory of the file, the -L ones and the ones built into the program.
inline DSL::Resolver resolver(const std::vector<std::string>& dirs, const std::map<std::string, const char*>& embedded) {
  return [&dirs, &embedded](const std::string& n) {
    std::string t;
    bool css = n.size() > 4 && n.substr(n.size() - 4) == ".css";
    bool file = n.find('.') != std::string::npos && !css;
    for (auto& d : dirs) if (file ? read(d + "/" + n, t) : readLib(d, css ? n : n + ".dg", t)) return t;
    if (file) return std::string();
    auto e = embedded.find(n);
    return e == embedded.end() ? std::string() : std::string(e->second);
  };
}

}
