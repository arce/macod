// macod: the macro level. It reads the whole language (def, data, repeat, if, import, the modules
// graph, tree, sequence, datatable and form, embedded screens and links) and writes the plain
// language that textod draws:  macod INPUT.dg | textod -
#include "cli_common.hpp"
#include "embedded_lib.hpp"
#include "macros_tec.hpp"

using namespace std;
using cli::read;

static const char* VERSION = "0.1";

static void usage(ostream& o) {
  o << "usage: " << PROGRAM << " INPUT.dg [-o OUTPUT.dg] [-L DIR]...\n"
    << "  INPUT       a .dg file, or - to read standard input\n"
    << "  -o OUTPUT   output file; standard output by default (- also works)\n"
    << "  -L DIR      extra directory with libraries (import); repeatable\n"
    << "  --libs      list the libraries built into the program\n"
    << "  -h, --help  this help      --version  version\n"
    << "It writes the plain language: it expands the macros and the modules"
    << ". It is drawn with textod:\n"
    << "  " << PROGRAM << " INPUT.dg | textod - -o OUTPUT.svg\n"
    << "Libraries are searched in: the input file's directory, the -L directories and the built-in ones.\n";
}

int main(int argc, char** argv) {
  string in, out;
  vector<string> dirs;
  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    if (a == "-h" || a == "--help") { usage(cout); return 0; }
    if (a == "--version") { cout << PROGRAM << " " << VERSION << "\n"; return 0; }
    if (a == "--libs") { for (auto& l : EMBEDDED) cout << l.first << "\n"; return 0; }
    if ((a == "-o" || a == "-L") && i + 1 >= argc) { cerr << PROGRAM << ": missing value for " << a << "\n"; return 2; }
    if (a == "-o") out = argv[++i];
    else if (a == "-L") dirs.push_back(argv[++i]);
    else if (a.size() > 1 && a[0] == '-') { cerr << PROGRAM << ": unknown option " << a << "\n"; usage(cerr); return 2; }
    else if (in.empty()) in = a;
    else { cerr << PROGRAM << ": only one input file is accepted\n"; return 2; }
  }
  if (in.empty()) { usage(cerr); return 2; }
  string src;
  if (in == "-") src.assign(istreambuf_iterator<char>(cin), istreambuf_iterator<char>());
  else if (!read(in, src)) { cerr << PROGRAM << ": cannot read " << in << "\n"; return 1; }
  dirs.insert(dirs.begin(), in == "-" ? "." : cli::dirOf(in));
  DSL::Resolver res = cli::resolver(dirs, EMBEDDED);
  string text;
  try {
    text = DSL::emit(DSL::compile(src, res));
  } catch (const exception& e) {
    cerr << (in == "-" ? "stdin" : in) << ": " << e.what() << "\n";
    return 1;
  }
  if (out.empty() || out == "-") cout << text;
  else {
    ofstream f(out, ios::binary);
    if (!f) { cerr << PROGRAM << ": cannot write " << out << "\n"; return 1; }
    f << text;
    cerr << out << "\n";
  }
  return 0;
}
