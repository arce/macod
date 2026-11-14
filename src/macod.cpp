// macod: the macro level. It reads the whole language (def, data, repeat, if, import, the modules
// graph, tree, sequence, datatable and form, embedded screens and links) and writes the plain
// language that textod draws:  macod ENTRADA.dg | textod -
// The charts are left out of this program (-DTEXTOD_NO_CHARTS).
#include "cli_common.hpp"
#include "embedded_lib.hpp"
#include "macros_tec.hpp"

using namespace std;
using cli::read;

static const char* VERSION = "0.1";

static void usage(ostream& o) {
  o << "uso: " << PROGRAM << " ENTRADA.dg [-o SALIDA.dg] [-L DIRECTORIO]...\n"
    << "  ENTRADA     archivo .dg, o - para leer de la entrada estándar\n"
    << "  -o SALIDA   archivo de salida; por defecto la salida estándar (- también)\n"
    << "  -L DIR      directorio extra con librerías (import); se puede repetir\n"
    << "  --libs      lista las librerías incluidas en el programa\n"
    << "  -h, --help  esta ayuda      --version  versión\n"
    << "Escribe el lenguaje plano: expande las macros y los módulos"
#ifndef TEXTOD_NO_CHARTS
    << " y las gráficas"
#endif
    << ". Se dibuja con textod:\n"
    << "  " << PROGRAM << " ENTRADA.dg | textod - -o SALIDA.svg\n"
    << "Las librerías se buscan en: el directorio del archivo, los -L y las incluidas.\n";
}

int main(int argc, char** argv) {
  string in, out;
  vector<string> dirs;
  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    if (a == "-h" || a == "--help") { usage(cout); return 0; }
    if (a == "--version") { cout << PROGRAM << " " << VERSION << "\n"; return 0; }
    if (a == "--libs") { for (auto& l : EMBEDDED) cout << l.first << "\n"; return 0; }
    if ((a == "-o" || a == "-L") && i + 1 >= argc) { cerr << PROGRAM << ": falta el valor de " << a << "\n"; return 2; }
    if (a == "-o") out = argv[++i];
    else if (a == "-L") dirs.push_back(argv[++i]);
    else if (a.size() > 1 && a[0] == '-') { cerr << PROGRAM << ": opción desconocida " << a << "\n"; usage(cerr); return 2; }
    else if (in.empty()) in = a;
    else { cerr << PROGRAM << ": solo se admite un archivo de entrada\n"; return 2; }
  }
  if (in.empty()) { usage(cerr); return 2; }
  string src;
  if (in == "-") src.assign(istreambuf_iterator<char>(cin), istreambuf_iterator<char>());
  else if (!read(in, src)) { cerr << PROGRAM << ": no se puede leer " << in << "\n"; return 1; }
  dirs.insert(dirs.begin(), in == "-" ? "." : cli::dirOf(in));
  DSL::Resolver res = cli::resolver(dirs, EMBEDDED);
  string text;
  try {
    text = DSL::emit(DSL::compile(src, res));
  } catch (const exception& e) {
    cerr << (in == "-" ? "entrada" : in) << ": " << e.what() << "\n";
    return 1;
  }
  if (out.empty() || out == "-") cout << text;
  else {
    ofstream f(out, ios::binary);
    if (!f) { cerr << PROGRAM << ": no se puede escribir " << out << "\n"; return 1; }
    f << text;
    cerr << out << "\n";
  }
  return 0;
}
