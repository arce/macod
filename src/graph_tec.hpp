#pragma once
#include <algorithm>
#include <map>
#include <set>
#include "dsl_tec.hpp"
#include "nodes_tec.hpp"

namespace GRAPH {

using Attrs = std::vector<std::pair<std::string, std::string>>;

struct Edge {
  std::string from, to, label;
  Attrs attrs;
};

struct Spec : NODES::Set {
  std::vector<Edge> edges;
};

using NODES::declare;

inline void line(Spec& s, const DSL::Node& n) {
  std::vector<DSL::Tok> t{DSL::Tok{"", n.head, false}};
  t.insert(t.end(), n.args.begin(), n.args.end());
  std::string prev, edge;
  Attrs attrs;
  bool arrow = false;
  for (size_t i = 0; i < t.size(); i++) {
    bool bare = t[i].key.empty() && !t[i].q;
    if (bare && t[i].v == "->") {
      if (prev.empty()) throw DSL::Error(n.line, "graph: '->' needs a node before it");
      arrow = true;
      edge.clear();
      attrs.clear();
      while (i + 2 < t.size() && !t[i + 1].key.empty()) { attrs.emplace_back(t[i + 1].key, DSL::tok(t[i + 1])); i++; }
      if (i + 2 < t.size() && t[i + 1].q && t[i + 1].key.empty() && t[i + 2].key.empty() && !t[i + 2].q) edge = t[++i].v;
      continue;
    }
    if (!bare) throw DSL::Error(n.line, "graph: write the node id first, then its \"label\" (found " + DSL::tok(t[i]) + ")");
    std::string id = t[i].v, label, extra;
    while (i + 1 < t.size() && !(t[i + 1].key.empty() && !t[i + 1].q && t[i + 1].v == "->")) {
      i++;
      if (t[i].key.empty() && t[i].q && label.empty()) label = t[i].v;
      else extra += (extra.empty() ? "" : " ") + DSL::tok(t[i]);
    }
    declare(s, id, label, extra);
    if (arrow) s.edges.push_back({prev, id, edge, attrs});
    prev = id;
    arrow = false;
  }
  if (arrow) throw DSL::Error(n.line, "graph: '->' needs a node after it");
}

inline std::map<std::string, int> levels(const Spec& s) {
  std::map<std::string, std::vector<std::string>> out;
  for (auto& e : s.edges) out[e.from].push_back(e.to);
  std::map<std::string, int> state, level;
  std::set<std::pair<std::string, std::string>> back;
  std::function<void(const std::string&)> mark = [&](const std::string& u) {
    state[u] = 1;
    for (auto& v : out[u]) {
      if (state[v] == 1) back.insert({u, v});
      else if (state[v] == 0) mark(v);
    }
    state[u] = 2;
  };
  for (auto& id : s.order) if (!state[id]) mark(id);
  std::function<int(const std::string&)> depth = [&](const std::string& v) {
    if (level.count(v)) return level[v];
    int d = 0;
    for (auto& e : s.edges) if (e.to == v && !back.count({e.from, e.to})) d = std::max(d, depth(e.from) + 1);
    return level[v] = d;
  };
  for (auto& id : s.order) depth(id);
  return level;
}

inline std::string join(const Attrs& a, const std::string& skip1 = "", const std::string& skip2 = "") {
  std::string r;
  for (auto& kv : a) if (kv.first != skip1 && kv.first != skip2) r += " " + kv.second;
  return r;
}

inline std::string pick(const Attrs& a, const std::string& key) {
  for (auto& kv : a) if (kv.first == key) return " " + kv.second;
  return "";
}

using Rows = std::map<int, std::vector<std::string>>;

inline long crossings(const Rows& rows, const std::vector<std::pair<std::string, std::string>>& links, const std::map<std::string, int>& level) {
  std::map<std::string, int> pos;
  for (auto& r : rows) for (size_t i = 0; i < r.second.size(); i++) pos[r.second[i]] = (int)i;
  long total = 0;
  for (size_t i = 0; i < links.size(); i++)
    for (size_t j = i + 1; j < links.size(); j++) {
      if (level.at(links[i].first) != level.at(links[j].first)) continue;
      long a = pos[links[i].first] - pos[links[j].first], b = pos[links[i].second] - pos[links[j].second];
      if (a * b < 0) total++;
    }
  return total;
}

inline void sweep(Rows& rows, int top, const std::vector<std::pair<std::string, std::string>>& links, bool down) {
  std::map<std::string, int> pos;
  auto index = [&]() { for (auto& r : rows) for (size_t i = 0; i < r.second.size(); i++) pos[r.second[i]] = (int)i; };
  index();
  for (int step = 0; step < top; step++) {
    int l = down ? step + 1 : top - 1 - step;
    auto& row = rows[l];
    std::map<std::string, double> key;
    for (size_t i = 0; i < row.size(); i++) {
      double sum = 0;
      int count = 0;
      for (auto& e : links) {
        if (down && e.second == row[i]) sum += pos[e.first], count++;
        if (!down && e.first == row[i]) sum += pos[e.second], count++;
      }
      key[row[i]] = count ? sum / count : (double)i;
    }
    std::stable_sort(row.begin(), row.end(), [&](const std::string& a, const std::string& b) { return key[a] < key[b]; });
    index();
  }
}

inline std::vector<DSL::Node> graph(const DSL::Node& n, std::map<std::string, DSL::Node>&, const std::map<std::string, DSL::Node>&) {
  Spec s;
  for (auto& k : n.kids) line(s, k);
  if (s.order.empty()) throw DSL::Error(n.line, "graph: it has no nodes");
  bool right = n.attr("direction", "down") == "right";
  if (!right && n.attr("direction", "down") != "down") throw DSL::Error(n.line, "graph: direction is down or right");
  auto level = levels(s);
  int top = 0;
  for (auto& l : level) top = std::max(top, l.second);
  size_t wide = 0;
  for (auto& e : s.edges) wide = std::max(wide, e.label.size());
  Rows rows;
  for (int l = 0; l <= top; l++) rows[l];
  for (auto& id : s.order) rows[level[id]].push_back(id);
  std::vector<std::vector<std::string>> chain(s.edges.size());
  std::vector<std::pair<std::string, std::string>> links;
  std::set<std::string> dummy;
  int made = 0;
  for (size_t i = 0; i < s.edges.size(); i++) {
    auto& e = s.edges[i];
    int a = level[e.from], b = level[e.to];
    if (a >= b) continue;
    std::string prev = e.from;
    for (int l = a + 1; l < b; l++) {
      std::string id = "_g" + std::to_string(++made);
      rows[l].push_back(id);
      level[id] = l;
      dummy.insert(id);
      chain[i].push_back(id);
      links.emplace_back(prev, id);
      prev = id;
    }
    links.emplace_back(prev, e.to);
  }
  if (n.attr("order", "auto") == "auto" && top > 0) {
    long best = crossings(rows, links, level);
    Rows keep = rows, work = rows;
    for (int pass = 0; pass < 4 && best > 0; pass++) {
      sweep(work, top, links, true);
      sweep(work, top, links, false);
      long c = crossings(work, links, level);
      if (c < best) best = c, keep = work;
    }
    rows = keep;
  }
  std::string gap = n.attr("gap", std::to_string(right ? std::max<size_t>(40, wide * 10 + 24) : 40)), outer = right ? "row" : "col", inner = right ? "col" : "row";
  std::string node = n.attr("node", "box");
  std::string out = outer + " gap=" + gap + " align=center\n";
  for (int l = 0; l <= top; l++) {
    out += "  " + inner + " gap=" + n.attr("sep", "30") + " align=center\n";
    for (auto& id : rows[l]) {
      if (dummy.count(id)) out += std::string("    spacer ") + id + (right ? " w=0 h=30" : " w=30 h=0") + "\n";
      else if (node == "box") out += "    box " + id + " " + DSL::quote(s.label[id]) + s.extra[id] + "\n";
      else out += "    " + node + " " + DSL::quote(s.label[id]) + " id=" + id + s.extra[id] + "\n";
    }
  }
  std::string from = right ? ".right" : ".bottom", to = right ? ".left" : ".top";
  for (size_t i = 0; i < s.edges.size(); i++) {
    auto& e = s.edges[i];
    int a = level[e.from], b = level[e.to];
    std::string sa = a < b ? from : a > b ? to : right ? ".bottom" : ".right", sb = a < b ? to : a > b ? from : right ? ".top" : ".left";
    std::string tag = (e.label.empty() ? "" : " " + DSL::quote(e.label)) + join(e.attrs);
    if (a > b) out += e.from + (right ? ".top" : ".right") + " -> " + e.to + (right ? ".top" : ".right") + " elbow margin=30" + tag + "\n";
    else if (chain[i].empty()) out += e.from + sa + " -> " + e.to + sb + " elbow" + tag + "\n";
    else {
      std::string common = join(e.attrs, "tip", "tail"), prev = e.from + from;
      std::string first = pick(e.attrs, "tail");
      for (size_t k = 0; k < chain[i].size(); k++) {
        out += prev + " -- " + chain[i][k] + to + " elbow" + common + (k == 0 ? first : "") + "\n";
        prev = chain[i][k] + from;
      }
      out += prev + " -> " + e.to + sb + " elbow" + (e.label.empty() ? "" : " " + DSL::quote(e.label)) + common + pick(e.attrs, "tip") + "\n";
    }
  }
  return DSL::parse(out).kids;
}

inline const bool registered = (DSL::macros()["graph"] = graph, true);

}
