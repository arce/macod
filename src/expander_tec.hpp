#pragma once
// The expander: def, data, repeat, if, import, content, embedded screens and links.
// Including this header makes DSL::compile expand; without it the text of a diagram goes
// through as it is (see dsl_tec.hpp).
#include "dsl_tec.hpp"

namespace DSL {

class Expander {
  Resolver res;
  std::map<std::string, Node> defs, data;
  std::set<std::string> loaded;
  Nodes styles;
  int seq = 0, steps = 0, inc = 0;
  std::string prefix;

  static int num(const std::string& s) {
    if (s.empty() || s.size() > 9) return -1;
    int r = 0;
    for (char c : s) {
      if (c < '0' || c > '9') return -1;
      r = r * 10 + (c - '0');
    }
    return r;
  }
  template <class M> static std::vector<std::string> names(const M& m) {
    std::vector<std::string> r;
    for (auto& kv : m) r.push_back(kv.first);
    return r;
  }

  void lib(const std::string& name, int ln, std::vector<std::string> chain) {
    if (loaded.count(name)) return;
    if (std::find(chain.begin(), chain.end(), name) != chain.end())
      throw Error(ln, "import '" + name + "': the libraries import each other");
    std::string text = res ? res(name) : "";
    if (text.empty()) throw Error(ln, "import '" + name + "': no such library");
    chain.push_back(name);
    try { load(parse(text).kids, nullptr, chain); }
    catch (const Error& e) {
      throw Error(ln, "library '" + name + "'" + (e.line ? ", line " + std::to_string(e.line) : "") + ": " + e.text);
    }
    loaded.insert(name);
  }

  void fromFile(Node& n, int ln) {
    if (!n.has("file")) return;
    std::string name = n.attr("file");
    std::string text = res ? res(name) : "";
    if (text.empty()) throw Error(ln, "file '" + name + "': not found or empty");
    Nodes rows;
    try { rows = csvRows(text, name); }
    catch (const Error& x) { throw Error(ln, x.text); }
    n.kids.insert(n.kids.begin(), rows.begin(), rows.end());
    n.args.erase(std::remove_if(n.args.begin(), n.args.end(), [](const Tok& t) { return t.key == "file"; }), n.args.end());
  }

  void load(const Nodes& in, Nodes* rest, const std::vector<std::string>& chain) {
    static const std::set<std::string> reserved{"repeat", "content", "if", "import", "def", "data", "style", "alias", "include"};
    for (auto& n : in) {
      auto w = n.pos();
      if (n.head == "import") {
        if (w.empty()) throw Error(n.line, "import: the library name is missing");
        lib(w[0], n.line, chain);
      } else if (n.head == "def" || n.head == "data") {
        if (w.empty() || n.args[0].q || !n.args[0].key.empty()) throw Error(n.line, n.head + ": the name is missing");
        if (reserved.count(w[0])) throw Error(n.line, n.head + ": '" + w[0] + "' is a reserved word");
        Node stored = n;
        if (n.head == "data") {
          fromFile(stored, n.line);
          if (n.has("file") && w.size() == 1 && !stored.kids.empty()) {
            for (auto& t : stored.kids[0].args) { Tok h; h.v = t.v; stored.args.push_back(h); }
            stored.kids.erase(stored.kids.begin());
          }
        }
        (n.head == "def" ? defs : data)[w[0]] = stored;
      } else if (n.head == "alias") {
        if (w.size() != 2 || w[0] != "prefix") throw Error(n.line, "alias: write  alias prefix <text>");
        prefix = w[1];
      } else if (n.head == "style") styles.push_back(n);
      else if (rest) rest->push_back(n);
      else throw Error(n.line, "only 'def' and 'import' go in a library");
    }
  }

  bool test(const Node& n, const Vars& v, int ln) {
    std::vector<std::string> w;
    for (auto& s : n.pos()) w.push_back(sub(s, v));
    size_t i = 0;
    bool neg = !w.empty() && w[0] == "not";
    if (neg) i = 1;
    if (i >= w.size()) throw Error(ln, "if: the name is missing");
    auto it = v.find(w[i]);
    std::string val = it == v.end() ? w[i] : it->second;
    if (i + 1 < w.size() && w[i + 1] == "is") {
      bool isnot = i + 2 < w.size() && w[i + 2] == "not";
      size_t k = i + 2 + (isnot ? 1 : 0);
      if (k >= w.size()) throw Error(ln, "if: the word to compare with is missing");
      return (val == w[k]) != isnot;
    }
    return (!val.empty() && val != "0") != neg;
  }

  void repeat(const Node& n, const Vars& v, Nodes& out, int depth, const Nodes* slot, int at) {
    int ln = at ? at : n.line;
    auto w = n.pos();
    const Node* d = nullptr;
    if (!w.empty()) {
      std::string name = sub(w[0], v);
      auto it = data.find(name);
      if (it == data.end()) throw Error(ln, "repeat: there is no data named '" + name + "'" + suggest(name, names(data)));
      d = &it->second;
    } else if (!n.has("n")) throw Error(ln, "repeat: give a data name or n=");
    int count = n.has("n") ? num(sub(n.attr("n"), v)) : (int)d->kids.size();
    if (count < 0) throw Error(ln, "repeat: n must be a whole number, 0 or more");
    std::string var = n.attr("var", "i");
    for (int i = 0; i < count; i++) {
      Vars s = v;
      s[var] = std::to_string(i);
      if (d) {
        auto f = d->pos();
        auto row = d->kids.empty() ? std::vector<std::string>() : d->kids[i % d->kids.size()].pos();
        for (size_t k = 1; k < f.size(); k++)
          s[f[k]] = k - 1 < row.size() ? row[k - 1] : f[k] + " " + std::to_string(i + 1);
      }
      expand(n.kids, s, out, depth, slot, at);
    }
  }

  static void prune(Nodes& list, const Vars& P) {
    for (auto& k : list) {
      k.args.erase(std::remove_if(k.args.begin(), k.args.end(), [&](const Tok& t) {
        if (t.key.empty() || t.v.size() < 2 || t.v[0] != '$') return false;
        for (size_t i = 1; i < t.v.size(); i++) if (!wc(t.v[i])) return false;
        return !P.count(t.v.substr(1));
      }), k.args.end());
      prune(k.kids, P);
    }
  }

  void include(const Node& n, const Vars& v, Nodes& out, int depth, const Nodes* slot, int at) {
    int ln = at ? at : n.line;
    auto w = n.pos();
    if (w.empty()) throw Error(ln, "include: the file name is missing");
    std::string name = sub(w[0], v);
    Vars P;
    for (auto& t : n.args) if (!t.key.empty()) P[t.key] = sub(t.v, v);
    std::string text = res ? res(name) : "";
    if (text.empty()) throw Error(ln, "include '" + name + "': not found or empty");
    if (++inc > 200) throw Error(ln, "include: too many files (do they include each other?)");
    Nodes kids = parse(text).kids;
    prune(kids, P);
    expand(kids, P, out, depth, slot, ln);
  }

  // Links `-> destination` placed in a use of a def (button, link, card...) and
  // diagrams already seen, to embed them with `screen id=NAME`.
  struct Link { std::string src, dest, label, from; int line; };
  std::vector<Link> links;
  std::vector<std::string> scr;  // the embedded screens being expanded: where a link starts
  std::map<std::string, Nodes> seen;

  static bool hasId(const Node& n, const std::string& id) {
    if (n.attr("id") == id) return true;
    for (auto& k : n.kids) if (hasId(k, id)) return true;
    return false;
  }
  static bool hasText(const Node& n) {
    for (auto& t : n.args) if (t.q) return true;
    return false;
  }
  static std::string diagramId(const Node& d) {
    if (d.has("id")) return d.attr("id");
    for (auto& t : d.args) if (t.key.empty() && !t.q) return t.v;
    return "";
  }
  // The ids of the earlier diagrams that sit in this one, in the order they appear.
  void screensIn(const Node& n, std::vector<std::string>& out) const {
    if (n.has("id") && seen.count(n.attr("id")) && std::find(out.begin(), out.end(), n.attr("id")) == out.end()) out.push_back(n.attr("id"));
    for (auto& k : n.kids) screensIn(k, out);
  }
  std::vector<std::string> screensIn(const Node& d) const {
    std::vector<std::string> o;
    for (auto& k : d.kids) screensIn(k, o);
    return o;
  }
  // A `screen id=OTHER` with no content or text, where OTHER is an earlier diagram,
  // is replaced by the content of that diagram.
  void embed(Nodes& kids) {
    for (auto& k : kids) {
      std::string id = k.attr("id");
      auto it = id.empty() ? seen.end() : seen.find(id);
      if (it != seen.end() && k.kids.empty() && !hasText(k) && k.head != "arrow" && k.head != "callout") {
        Node r;
        if (it->second.size() == 1) r = it->second[0];
        else { r.head = "col"; r.kids = it->second; }
        r.args.erase(std::remove_if(r.args.begin(), r.args.end(), [](const Tok& t) { return t.key == "id"; }), r.args.end());
        r.args.push_back({"id", id, false});
        for (auto& t : k.args) if (!t.key.empty() && t.key != "id" && !r.has(t.key)) r.args.push_back(t);
        r.line = k.line;
        k = r;
      } else embed(k.kids);
    }
  }

  void use(const Node& n, const Vars& v, Nodes& out, int depth, const Nodes* slot, int at) {
    const Node& d = defs.at(n.head);
    int ln = at ? at : n.line;
    Vars vars;
    std::vector<std::string> params;
    bool open = false;
    for (size_t i = 1; i < d.args.size(); i++) {
      auto& t = d.args[i];
      if (t.key.empty() && t.v == "*") { open = true; continue; }
      params.push_back(t.key.empty() ? t.v : t.key);
      vars[params.back()] = t.key.empty() ? "" : t.v;
    }
    auto isParam = [&](const std::string& s) { return std::find(params.begin(), params.end(), s) != params.end(); };
    std::vector<Tok> extra;
    std::string id;
    size_t next = 0;
    std::string dest, label;
    bool hidden = false;
    for (size_t ai = 0; ai < n.args.size(); ai++) {
      auto& t = n.args[ai];
      std::string s = sub(t.v, v);
      if (t.key.empty() && !t.q && (t.v == "->" || t.v == "->#") && ai + 1 < n.args.size() && n.args[ai + 1].key.empty() && !n.args[ai + 1].q) {
        hidden = t.v == "->#";  // `->#` documents the link but the flow does not draw it
        dest = sub(n.args[++ai].v, v);
        continue;
      }
      if (t.q && label.empty() && t.key.empty()) label = s;
      if (t.key == "id" || (t.key.empty() && !t.q && t.v == "$id")) id = s;
      else if (!t.key.empty()) {
        if (isParam(t.key)) vars[t.key] = s;
        else if (!open) extra.push_back({t.key, s, t.q});
      } else if (!t.q && isParam(t.v)) vars[t.v] = "1";
      else {
        if (open && !t.q) continue;
        if (next >= params.size()) {
          if (open) continue;
          throw Error(ln, "'" + n.head + "' takes " + std::to_string(params.size()) + " parameter(s), no more");
        }
        vars[params[next++]] = s;
      }
    }
    vars["id"] = id.empty() ? "_" + std::to_string(++seq) : id;
    Nodes inner, body;
    bool screen = seen.count(vars["id"]) > 0;
    if (screen) scr.push_back(vars["id"]);
    expand(n.kids, v, inner, depth, slot, at);
    expand(d.kids, vars, body, depth + 1, &inner, ln);
    if (screen) scr.pop_back();
    if (!body.empty())
      for (auto& t : extra) if (!body[0].has(t.key)) body[0].args.push_back(t);
    if (!dest.empty() && !hidden)
      for (auto& b : body) if (hasId(b, vars["id"])) { links.push_back({vars["id"], dest, label, scr.empty() ? "" : scr.back(), ln}); break; }
    out.insert(out.end(), body.begin(), body.end());
  }

  void expand(const Nodes& in, const Vars& v, Nodes& out, int depth, const Nodes* slot, int at) {
    if (depth > 40) throw Error(at, "the components nest too deep");
    for (auto& n : in) {
      int ln = at ? at : n.line;
      if (++steps > 50000) throw Error(ln, "the expansion is too large");
      if (n.head == "content" && slot) out.insert(out.end(), slot->begin(), slot->end());
      else if (n.head == "if") { if (test(n, v, ln)) expand(n.kids, v, out, depth, slot, at); }
      else if (n.head == "repeat") repeat(n, v, out, depth, slot, at);
      else if (n.head == "include") include(n, v, out, depth, slot, at);
      else if (depth == 0 && !prefix.empty() && defs.count(prefix + n.head)) {
        Node m = n;
        m.head = prefix + n.head;
        use(m, v, out, depth, slot, at);
      } else if (defs.count(n.head)) use(n, v, out, depth, slot, at);
      else if (n.head == "def" || n.head == "data" || n.head == "import")
        throw Error(ln, "'" + n.head + "' only goes at the top level");
      else {
        Node c;
        c.head = sub(n.head, v);
        c.line = ln;
        c.indent = n.indent;
        for (auto& t : n.args) {
          std::string s = sub(t.v, v);
          if (!t.q && s.empty() && (t.key.empty() || t.v.find('$') != std::string::npos)) continue;
          if (!t.q && t.key.empty() && t.v == "$id") c.args.push_back({"id", s, false});
          else {
            // a value that came from a parameter and has spaces is a text, not a word: the expanded tree keeps that
            bool text = t.q || (t.key.empty() && t.v.find('$') != std::string::npos &&
                                s.find_first_of(" \t") != std::string::npos);
            c.args.push_back({t.key, s, text});
          }
        }
        expand(n.kids, v, c.kids, depth, slot, at);
        if (raws().count(c.head)) fromFile(c, ln);
        auto m = macros().find(c.head);
        if (m == macros().end()) out.push_back(std::move(c));
        else expand(m->second(c, data, defs), {}, out, depth + 1, slot, at);
      }
    }
  }

public:
  explicit Expander(Resolver r = nullptr) : res(std::move(r)) {}

  Node run(const Node& tree) {
    Node out;
    Nodes rest;
    load(tree.kids, &rest, {});
    out.kids = styles;
    for (auto& n : rest) {
      Node cur = n;
      bool dg = cur.head == "diagram";
      if (dg) { embed(cur.kids); links.clear(); }
      Nodes one{cur};
      size_t from = out.kids.size();
      expand(one, {}, out.kids, 0, nullptr, 0);
      if (!dg) continue;
      seen[diagramId(cur)] = cur.kids;
      bool forward = cur.attr("links") == "forward";  // only the links to a screen that comes later in the flow
      bool none = cur.attr("links") == "none";
      for (size_t i = from; i < out.kids.size(); i++) {
        if (out.kids[i].head != "diagram") continue;
        std::vector<std::string> order;
        if (forward) order = screensIn(out.kids[i]);
        auto pos = [&](const std::string& id) { auto f = std::find(order.begin(), order.end(), id); return f == order.end() ? -1 : static_cast<int>(f - order.begin()); };
        for (auto& l : links) {
          if (none || !hasId(out.kids[i], l.dest)) continue;
          if (forward && pos(l.from) >= 0 && pos(l.dest) >= 0 && pos(l.dest) <= pos(l.from)) continue;
          Node a;
          a.head = "arrow";
          a.line = l.line;
          a.args = {{"", l.src, false}, {"", "->", false}, {"", l.dest, false}, {"", "avoid", false}};
          if (!l.label.empty()) a.args.push_back({"", l.label, true});
          out.kids[i].kids.push_back(a);
        }
      }
    }
    return out;
  }
};

inline Node expand(const Node& tree, const Resolver& r = nullptr) { return Expander(r).run(tree); }

inline const bool expanderRegistered = (expander() = [](const Node& t, const Resolver& r) { return expand(t, r); }, true);

}
