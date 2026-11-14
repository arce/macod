#pragma once
#include "dsl_tec.hpp"

namespace FORM {

inline bool starts(const std::string& s, const std::string& p) { return s.compare(0, p.size(), p) == 0; }

inline std::string rest(const std::string& s, size_t n) {
  size_t i = s.find_first_not_of(' ', n);
  return i == std::string::npos ? "" : s.substr(i);
}

inline std::string trim(const std::string& s) {
  size_t a = s.find_first_not_of(' ');
  if (a == std::string::npos) return "";
  return s.substr(a, s.find_last_not_of(' ') - a + 1);
}

struct Line {
  int indent;
  std::string text;
  int line;
};

inline std::string element(const std::string& s, int line);

inline std::string cells(const std::string& s, int line) {
  std::string out = "row gap=8\n", cur;
  std::vector<std::string> parts;
  for (char c : s.substr(1)) {
    if (c == '|') { parts.push_back(trim(cur)); cur.clear(); } else cur += c;
  }
  parts.push_back(trim(cur));
  for (auto& p : parts) {
    if (p.empty()) throw DSL::Error(line, "form: empty cell in a '|' row");
    out += "  " + element(p, line) + "\n";
  }
  out.pop_back();
  return out;
}

inline std::string element(const std::string& s, int line) {
  if (s == "--") return "divider";
  if (s == "[img]") return "image";
  if (s == "[thumb]") return "thumb";
  if (s == "~~~") return "lines";
  if (s[0] == '|' && (s.size() == 1 || s[1] == ' ')) return cells(s, line);
  if (starts(s, "[img ") && s.back() == ']') return "image h=" + trim(s.substr(5, s.size() - 6));
  if (starts(s, "~~~ ")) return "lines n=" + rest(s, 4);
  if (s[0] == '+') {
    size_t e = s.find(' ');
    std::string tone = s.substr(1, e == std::string::npos ? e : e - 1), title = e == std::string::npos ? "" : rest(s, e);
    return "card" + (title.empty() ? "" : " " + DSL::quote(title)) + (tone.empty() ? "" : " tone=" + tone);
  }
  if (s.size() > 2 && s[0] == '(' && s.back() == ')') return "badge " + DSL::quote(s.substr(1, s.size() - 2));
  struct Rule {
    const char* prefix;
    const char* def;
    const char* flags;
  };
  static const Rule rules[] = {{">< ", "button", " center"}, {">> ", "button", " outline right"}, {"> ", "button", " right"}, {"~ ", "note", ""},
                               {"= ", "title", ""}, {"@ ", "link", ""}, {"[x] ", "checkbox", " checked"}, {"[ ] ", "checkbox", ""},
                               {"v ", "select", ""}, {"? ", "field", " search"}, {"- ", "text", " grow h=14"}};
  for (auto& r : rules)
    if (starts(s, r.prefix)) {
      std::string body = rest(s, std::string(r.prefix).size());
      if (body.empty()) throw DSL::Error(line, std::string("form: '") + r.prefix + "' needs a text after it");
      std::string def = r.def;
      return def + " " + DSL::quote(body) + r.flags;
    }
  bool secret = s.size() > 2 && s.compare(s.size() - 2, 2, " *") == 0;
  return "field " + DSL::quote(secret ? s.substr(0, s.size() - 2) : s) + (secret ? " secret" : "");
}

inline void emit(const std::vector<Line>& ls, size_t& i, int indent, const std::string& pad, std::string& out) {
  while (i < ls.size() && ls[i].indent >= indent) {
    if (ls[i].indent > indent) throw DSL::Error(ls[i].line, "form: unexpected indentation (only a '+' card has lines under it)");
    std::string e = element(ls[i].text, ls[i].line);
    std::string indented;
    for (size_t k = 0; k < e.size(); k++) {
      indented += e[k];
      if (e[k] == '\n') indented += pad;
    }
    out += pad + indented + "\n";
    bool card = ls[i].text[0] == '+';
    int line = ls[i].line;
    i++;
    if (i < ls.size() && ls[i].indent > indent) {
      if (!card) throw DSL::Error(ls[i].line, "form: only a '+' card can have lines under it (line " + std::to_string(line) + ")");
      emit(ls, i, ls[i].indent, pad + "  ", out);
    }
  }
}

inline std::vector<DSL::Node> form(const DSL::Node& n, std::map<std::string, DSL::Node>&, const std::map<std::string, DSL::Node>&) {
  std::string out = "screen";
  for (auto& t : n.args) out += " " + DSL::tok(t);
  out += "\n";
  std::vector<Line> ls;
  for (auto& row : n.kids) {
    std::string s;
    for (size_t i = 0; i < row.args.size(); i++) s += (i ? ", " : "") + row.args[i].v;
    ls.push_back({row.indent, s, row.line});
  }
  size_t i = 0;
  if (!ls.empty()) emit(ls, i, ls[0].indent, "  ", out);
  return DSL::parse(out).kids;
}

inline const bool registered = (DSL::raws().insert("form"), DSL::macros()["form"] = form, true);

}
