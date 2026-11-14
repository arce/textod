// textod: the plain language to SVG. It has no macro expander: its input must be plain language.
// Like a Unix filter: it reads the file (or the standard input) and writes the SVG to the standard output.
#include "cli_common.hpp"
#include "figure_tec.hpp"

using namespace std;
using cli::read; using cli::dirOf;

static const char* VERSION = "0.1";

static void usage(ostream& o) {
  o << "usage: " << PROGRAM << " [-o OUTPUT] [-L DIR]... [-T THEME] [INPUT]\n"
    << "  INPUT       a .dg file; without INPUT (or with -) the standard input is read\n"
    << "  -o OUTPUT   write to OUTPUT instead of the standard output\n"
    << "  -L DIR      extra directory with styles (for import); repeatable\n"
    << "  -T THEME    CSS theme (THEME.css) for the diagrams that do not set theme=\n"
    << "  --libs      list the styles and themes that are found\n"
    << "  -h, --help  this help       --version  version\n"
    << "If the file has several diagrams, with -o each one goes to OUTPUT-ID.svg;\n"
    << "without -o they go one after the other to the standard output.\n"
    << "Styles (import) are looked for in: the directory of the file, the -L directories and the lib/ next to " << PROGRAM << " (or ../lib, ../share/" << PROGRAM << ").\n"
    << "Themes (THEME.css) are looked for in the same places.\n"
    << "Examples:  " << PROGRAM << " a.dg > a.svg     " << PROGRAM << " a.dg -o a.svg     cat a.dg | " << PROGRAM << " > a.svg\n";
}

int main(int argc, char** argv) {
  cli::binaryStdio();
  string in, out, theme;
  vector<string> dirs;
  bool options = true, libs = false;
  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    if (options && a == "--") { options = false; continue; }
    if (!options || a == "-" || a[0] != '-') {
      if (!in.empty()) { cerr << PROGRAM << ": only one input file is accepted\n"; return 2; }
      in = a;
      continue;
    }
    if (a == "-h" || a == "--help") { usage(cout); return 0; }
    if (a == "--version") { cout << PROGRAM << " " << VERSION << "\n"; return 0; }
    if (a == "--libs") { libs = true; continue; }
    char opt = a.size() > 1 && a[1] != '-' ? a[1] : 0;
    if (opt != 'o' && opt != 'L' && opt != 'T') { cerr << PROGRAM << ": unknown option " << a << "\n"; usage(cerr); return 2; }
    string v;
    if (a.size() > 2) v = a.substr(2);               // -ofile
    else if (i + 1 < argc) v = argv[++i];            // -o file
    else { cerr << PROGRAM << ": missing value for " << a << "\n"; return 2; }
    if (opt == 'o') out = v;
    else if (opt == 'L') dirs.push_back(v);
    else theme = v;
  }
  // the libraries that travel with the program: lib/ next to the executable or in its parent (the repository), or share/textod (installed)
  string exe = cli::exeDir();
  if (!exe.empty()) for (const char* rel : {"/lib", "/../lib", "/../share/" PROGRAM}) dirs.push_back(exe + rel);
  if (libs) {
    for (auto& n : cli::libraries(dirs)) cout << n << "\n";
    return 0;
  }
  if (in.empty()) {
    if (cli::isTerminal(0)) { usage(cerr); return 2; }   // no file and a terminal: nothing to read
    in = "-";
  }
  string src;
  if (in == "-") src.assign(istreambuf_iterator<char>(cin), istreambuf_iterator<char>());
  else if (!read(in, src)) { cerr << PROGRAM << ": cannot read " << in << "\n"; return 1; }
  dirs.insert(dirs.begin(), in == "-" ? "." : dirOf(in));
  DSL::Resolver res = cli::resolver(dirs);
  try {
    auto diagrams = FIGURE::render(src, res, theme);
    if (diagrams.empty()) { cerr << PROGRAM << ": " << (in == "-" ? "stdin" : in) << ": no diagram found\n"; return 1; }
    bool toStdout = out.empty() || out == "-";
    for (auto& d : diagrams) {
      if (toStdout) { cout << d.svg; continue; }
      string path = out;
      if (diagrams.size() > 1) path = (out.size() > 4 && out.substr(out.size() - 4) == ".svg" ? out.substr(0, out.size() - 4) : out) + "-" + d.id + ".svg";
      ofstream f(path, ios::binary);
      if (!f || !(f << d.svg).flush()) { cerr << PROGRAM << ": cannot write " << path << "\n"; return 1; }
    }
    if (toStdout && !cout.flush()) { cerr << PROGRAM << ": cannot write the output\n"; return 1; }
  } catch (const exception& e) {
    string m = e.what();
    cerr << (in == "-" ? "stdin" : in) << ": " << m << "\n";
    if (m.find("needs the expander") != string::npos)
      cerr << PROGRAM << ": this needs the macro expander, which " << PROGRAM << " does not include: the input must be plain language\n";
    if (m.find("no such library") != string::npos) {
      cerr << PROGRAM << ": searched in:";
      for (auto& d : dirs) cerr << " " << d;
      cerr << "\n";
    }
    return 1;
  }
  return 0;
}
