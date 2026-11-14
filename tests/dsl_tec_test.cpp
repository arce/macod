#include "expander_tec.hpp"
#include <iostream>
using namespace std;
static int fails = 0;
#define EQ(a, b) do { auto x = (a); auto y = (b); if (!(x == y)) { cout << "FAIL line " << __LINE__ << ": " #a "\n--- got\n" << x << "--- expected\n" << y << "\n"; fails++; } } while (0)
static string err(const string& t, DSL::Resolver r = nullptr) {
  try { DSL::compile(t, r); } catch (const DSL::Error& e) { return e.what(); }
  return "ok";
}
int main() {
  EQ(DSL::show(DSL::parse("a 1 \"x y\" k=v q=\"a b\" ==> --> x=#fff # note\n  b\n  c; d\n    e\nf")),
     string("a 1 \"x y\" k=v q=\"a b\" ==> --> x=\"#fff\"\n  b\n  c\n  d\n    e\nf\n"));
  EQ(DSL::show(DSL::parse("a \"q\\\"z\\n\"\n\n  # solo comentario\n  b")), string("a \"q\\\"z\\n\"\n  b\n"));
  EQ(err("a\n\tb"), string("line 2: tabs are not allowed in the indentation"));
  EQ(err("a\n    b\n  c"), string("line 3: the indentation does not match any outer level"));
  EQ(err("a \"abc"), string("line 1: the string is not closed"));
  EQ(err("\"a\" b"), string("line 1: a name was expected at the start of the item"));

  // #line N: the lines that follow are the ones of the file the text came from
  {
    auto n = DSL::parse("a\n#line 10\nb\nc\n#line 3\nd");
    EQ(std::to_string(n.kids[0].line) + "," + std::to_string(n.kids[1].line) + "," + std::to_string(n.kids[2].line) + "," + std::to_string(n.kids[3].line), string("1,10,11,3"));
    // the expander writes it: the error of the plain text names the line of the original file
    string src2 = "def p\n  box x\n\n\np\np\n\np\n";
    string plain = DSL::emit(DSL::compile(src2));
    EQ(plain, string("#line 5\nbox x\nbox x\n#line 8\nbox x\n"));
    EQ(DSL::emit(DSL::parse(plain)), plain);
  }

  string src =
    "data product name price\n"
    "  Keyboard, $45\n"
    "  Mouse, $20\n  \"Monitor, 24\", $210\n"
    "def tile title price featured\n"
    "  card id=$id\n"
    "    text $title bold\n"
    "    text $price\n"
    "    if featured\n"
    "      note \"Best seller\"\n"
    "    content\n"
    "screen catalog\n"
    "  row\n"
    "    repeat product\n"
    "      tile $name $price\n"
    "        button \"Buy\"\n"
    "    tile \"Single\" \"$1\" featured id=z width=50\n";
  EQ(DSL::show(DSL::compile(src)),
     string("screen catalog\n  row\n"
            "    card id=_1\n      text Keyboard bold\n      text $45\n      button \"Buy\"\n"
            "    card id=_2\n      text Mouse bold\n      text $20\n      button \"Buy\"\n"
            "    card id=_3\n      text \"Monitor, 24\" bold\n      text $210\n      button \"Buy\"\n"
            "    card id=z width=50\n      text Single bold\n      text $1\n      note \"Best seller\"\n"));
  EQ(DSL::show(DSL::compile("def p\n  $id-a -> $id-b\np id=x")), string("x-a -> x-b\n"));
  EQ(DSL::show(DSL::compile("repeat n=3 var=k\n  x $k\nrepeat n=2\n  y $i")), string("x 0\nx 1\nx 2\ny 0\ny 1\n"));
  EQ(DSL::show(DSL::compile("data d a b\n  1\nrepeat d n=2\n  r $a $b $i")), string("r 1 \"b 1\" 0\nr 1 \"b 2\" 1\n"));
  EQ(DSL::show(DSL::compile("def w kind\n  if kind is wide\n    A\n  if kind is not wide\n    B\n  if not kind\n    C\nw wide\nw\nw tall")),
     string("A\nB\nC\nB\n"));
  EQ(DSL::show(DSL::compile("def p x=\"def\"\n  t $x\np\np x=1\np \"2\"")), string("t def\nt 1\nt 2\n"));
  EQ(DSL::show(DSL::compile("def outer\n  g\n    content\ndef inner\n  outer\n    content\n    z\ninner\n  y")), string("g\n  y\n  z\n"));

  DSL::Resolver lib = [](const string& n) -> string {
    if (n == "a") return "import b\ndef fa\n  FA";
    if (n == "b") return "def fb\n  FB";
    if (n == "c") return "import d\n";
    if (n == "d") return "import c\n";
    if (n == "bad") return "def f\n  X\nbox\n";
    if (n == "oops") return "def f\n  X\n  data q\n";
    return "";
  };
  EQ(DSL::show(DSL::compile("import a\nfa\nfb", lib)), string("FA\nFB\n"));
  EQ(DSL::show(DSL::compile("style s\n  box sw=2\nbox a", lib)), string("style s\n  box sw=2\nbox a\n"));
  EQ(DSL::show(DSL::compile("import st\nbox a", [](const string&) { return string("style t\n  box radius=0\ndef f\n  X"); })), string("style t\n  box radius=0\nbox a\n"));
  EQ(err("import zz", lib), string("line 1: import 'zz': no such library"));
  EQ(err("import c", lib), string("line 1: library 'c', line 1: library 'd', line 1: import 'c': the libraries import each other"));
  EQ(err("x\nimport bad", lib), string("line 2: library 'bad', line 3: only 'def' and 'import' go in a library"));
  EQ(err("repeat prod\n  x", lib), string("line 1: repeat: there is no data named 'prod'"));
  EQ(err("data product a\nrepeat produt\n  x"), string("line 2: repeat: there is no data named 'produt' (did you mean 'product'?)"));
  EQ(err("repeat n=999\n  x"), string("line 1: repeat: n must be between 0 and 200"));
  EQ(err("repeat\n  x"), string("line 1: repeat: give a data name or n="));
  EQ(err("def a\n  a\na"), string("line 3: the components nest too deep"));
  EQ(err("def a x\n  t\na 1 2"), string("line 3: 'a' takes 1 parameter(s), no more"));
  EQ(err("def if\n  t"), string("line 1: def: 'if' is a reserved word"));
  EQ(err("a\n  def f\n    t"), string("line 2: 'def' only goes at the top level"));
  EQ(err("def f\n  repeat n=200\n    repeat n=200\n      repeat n=200\n        x\nf"), string("line 6: the expansion is too large"));
  EQ(DSL::suggest("bxo", {"box", "row"}), string(" (did you mean 'box'?)"));
  EQ(DSL::suggest("zzzz", {"box", "row"}), string(""));
  {
    DSL::Resolver r = [](const string& n) -> string {
      if (n == "l") return "alias prefix s_\ndef s_btn text *\n  shape \"$text\" h=2\ndef s_box w *\n  box w=$w\n    content\n";
      if (n == "f.swl") return "item \"a\" k=$k j=$j\n  sub \"b\" m=$m\n";
      return "";
    };
    auto show = [&](const string& t) { return DSL::show(DSL::compile(t, r)); };
    EQ(show("import l\ndiagram d\n  btn \"Hi\" id=x.y look=1 flag actionCommand=z\n  box\n"), string("diagram d\n  shape \"Hi\" h=2\n  box\n"));
    EQ(show("import l\ndiagram d\n  box w=3\n    btn \"Q\"\n  include f.swl k=1\n"), string("diagram d\n  box w=3\n    shape \"Q\" h=2\n  item \"a\" k=1\n    sub \"b\"\n"));
    EQ(show("import l\ndiagram d\n  box 1 2\n"), string("diagram d\n  box\n"));
    EQ(err("diagram d\n  include nofile", r), string("line 2: include 'nofile': not found or empty"));
    EQ(err("alias a b c"), string("line 1: alias: write  alias prefix <text>"));
  }
  cout << (fails ? "FAILED" : "all tests passed") << endl;
  return fails;
}
