#pragma once
#include "dsl_tec.hpp"

namespace TREE {

inline void emit(const DSL::Node& n, bool right, const std::string& gap, const std::string& sep, const std::string& pad, std::string& out, std::string& edges) {
  std::string label = n.head, extra;
  bool named = false;
  for (auto& t : n.args) {
    if (t.key.empty() && t.q && !named) { label = t.v; named = true; }
    else extra += " " + DSL::tok(t);
  }
  out += pad + (right ? "row" : "col") + " gap=" + gap + " align=center\n";
  out += pad + "  box " + n.head + " \"" + label + "\"" + extra + "\n";
  if (n.kids.empty()) return;
  out += pad + "  " + (right ? "col" : "row") + " gap=" + sep + " align=start\n";
  for (auto& k : n.kids) {
    emit(k, right, gap, sep, pad + "    ", out, edges);
    edges += n.head + (right ? ".right" : ".bottom") + " -> " + k.head + (right ? ".left" : ".top") + " elbow\n";
  }
}

inline std::vector<DSL::Node> tree(const DSL::Node& n, std::map<std::string, DSL::Node>&, const std::map<std::string, DSL::Node>&) {
  if (n.kids.size() != 1) throw DSL::Error(n.line, "tree: it needs exactly one root (the other nodes go indented under it)");
  std::string dir = n.attr("direction", "down");
  if (dir != "down" && dir != "right") throw DSL::Error(n.line, "tree: direction is down or right");
  std::string out, edges;
  emit(n.kids[0], dir == "right", n.attr("gap", "36"), n.attr("sep", "20"), "", out, edges);
  return DSL::parse(out + edges).kids;
}

inline const bool registered = (DSL::macros()["tree"] = tree, true);

}
