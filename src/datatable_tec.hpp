#pragma once
#include "dsl_tec.hpp"

namespace DATATABLE {

inline std::string cell(const DSL::Tok& t, const std::string& extra) {
  return "  box \"" + t.v + "\" sw=1 radius=0" + extra + "\n";
}

inline std::vector<DSL::Node> datatable(const DSL::Node& n, std::map<std::string, DSL::Node>&, const std::map<std::string, DSL::Node>&) {
  if (n.kids.empty()) throw DSL::Error(n.line, "datatable: the first row is the header");
  size_t cols = n.kids[0].args.size();
  if (!cols) throw DSL::Error(n.line, "datatable: the header is empty");
  std::string out = "grid cols=" + std::to_string(cols) + " gap=0 align=stretch" + (n.has("w") ? " w=" + n.attr("w") : "") + "\n";
  bool zebra = n.flag("zebra");
  for (size_t r = 0; r < n.kids.size(); r++) {
    auto& row = n.kids[r];
    if (row.args.size() != cols) throw DSL::Error(row.line, "datatable: the row has " + std::to_string(row.args.size()) + " cells and the header " + std::to_string(cols));
    for (auto& t : row.args) out += cell(t, r == 0 ? " light bold" : zebra && r % 2 == 0 ? " silver" : "");
  }
  return DSL::parse(out).kids;
}

inline const bool registered = (DSL::raws().insert("datatable"), DSL::macros()["datatable"] = datatable, true);

}
