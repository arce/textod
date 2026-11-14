// textod: the plain language to SVG. It has no macro expander: its input must be plain language.
#include "cli_common.hpp"
#include "embedded_lib.hpp"
#include "figure_tec.hpp"

using namespace std;
using cli::read; using cli::dirOf; using cli::stem;

static const char* VERSION = "0.1";

static void usage(ostream& o) {
  o << "uso: " << PROGRAM << " ENTRADA.dg [-o SALIDA.svg] [-L DIRECTORIO]... [-T TEMA]\n"
    << "  ENTRADA     archivo .dg, o - para leer de la entrada estándar\n"
    << "  -o SALIDA   archivo de salida; - para la salida estándar (por defecto ENTRADA.svg)\n"
    << "  -L DIR      directorio extra con librerías (import); se puede repetir\n"
    << "  -T TEMA     tema CSS (TEMA.css) para los diagramas que no traen theme=\n"
    << "  --libs      lista las librerías incluidas en el programa\n"
    << "  -h, --help  esta ayuda      --version  versión\n"
    << "Si el archivo tiene varios diagramas, cada uno sale en SALIDA-ID.svg.\n"
    << "Las librerías se buscan en: el directorio del archivo, los -L y las incluidas.\n";
}

int main(int argc, char** argv) {
  string in, out, theme;
  vector<string> dirs;
  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    if (a == "-h" || a == "--help") { usage(cout); return 0; }
    if (a == "--version") { cout << PROGRAM << " " << VERSION << "\n"; return 0; }
    if (a == "--libs") { for (auto& l : EMBEDDED) cout << l.first << "\n"; return 0; }
    if ((a == "-o" || a == "-L" || a == "-T") && i + 1 >= argc) { cerr << PROGRAM << ": falta el valor de " << a << "\n"; return 2; }
    if (a == "-o") out = argv[++i];
    else if (a == "-L") dirs.push_back(argv[++i]);
    else if (a == "-T") theme = argv[++i];
    else if (a.size() > 1 && a[0] == '-') { cerr << PROGRAM << ": opción desconocida " << a << "\n"; usage(cerr); return 2; }
    else if (in.empty()) in = a;
    else { cerr << PROGRAM << ": solo se admite un archivo de entrada\n"; return 2; }
  }
  if (in.empty()) { usage(cerr); return 2; }
  string src;
  if (in == "-") {
    src.assign(istreambuf_iterator<char>(cin), istreambuf_iterator<char>());
    if (out.empty()) out = "-";
  } else if (!read(in, src)) {
    cerr << PROGRAM << ": no se puede leer " << in << "\n";
    return 1;
  }
  string base = in == "-" ? "." : dirOf(in);
  dirs.insert(dirs.begin(), base);
  DSL::Resolver res = cli::resolver(dirs, EMBEDDED);
  try {
    auto diagrams = FIGURE::render(src, res, theme);
    if (diagrams.empty()) { cerr << PROGRAM << ": " << in << ": no hay ningún diagrama\n"; return 1; }
    if (out.empty()) out = (in == "-" ? string("salida") : dirOf(in) + "/" + stem(in)) + ".svg";
    for (auto& d : diagrams) {
      string path = out;
      if (diagrams.size() > 1) {
        if (out == "-") { cout << d.svg; continue; }
        path = (out.size() > 4 && out.substr(out.size() - 4) == ".svg" ? out.substr(0, out.size() - 4) : out) + "-" + d.id + ".svg";
      }
      if (path == "-") cout << d.svg;
      else {
        ofstream f(path, ios::binary);
        if (!f) { cerr << PROGRAM << ": no se puede escribir " << path << "\n"; return 1; }
        f << d.svg;
        cerr << path << "\n";
      }
    }
  } catch (const exception& e) {
    string m = e.what();
    cerr << (in == "-" ? "entrada" : in) << ": " << m << "\n";
    if (m.find("needs the expander") != string::npos || m.find("no such library") != string::npos)
      cerr << PROGRAM << ": esto necesita expandir macros, que " << PROGRAM << " no incluye: la entrada debe estar en lenguaje plano\n";
    return 1;
  }
  return 0;
}
