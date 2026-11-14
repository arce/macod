#pragma once
#include <map>
#include "dsl_tec.hpp"
#include "nodes_tec.hpp"

namespace SEQUENCE {

struct Message {
  std::string from, to, text;
  bool dashed = false;
};

struct Spec : NODES::Set {
  std::vector<Message> messages;
};

using NODES::declare;
using DSL::bare;

inline void line(Spec& s, const DSL::Node& n) {
  const auto& a = n.args;
  if (!a.empty() && bare(a[0]) && (a[0].v == "->" || a[0].v == "-->")) {
    if (a.size() < 2 || !bare(a[1])) throw DSL::Error(n.line, "sequence: write 'from -> to \"text\"'");
    if (n.head == a[1].v) throw DSL::Error(n.line, "sequence: a message needs two different participants");
    declare(s, n.head, "");
    declare(s, a[1].v, "");
    std::string text;
    if (a.size() > 2) {
      if (a.size() != 3 || a[2].q == false || !a[2].key.empty()) throw DSL::Error(n.line, "sequence: after the target only the \"text\" of the message goes");
      text = a[2].v;
    }
    s.messages.push_back({n.head, a[1].v, text, a[0].v == "-->"});
    return;
  }
  if (a.size() > 1 || (a.size() == 1 && (a[0].q == false || !a[0].key.empty()))) throw DSL::Error(n.line, "sequence: a participant is 'id \"Label\"'; a message is 'a -> b \"text\"'");
  declare(s, n.head, a.empty() ? "" : a[0].v);
}

inline std::vector<DSL::Node> sequence(const DSL::Node& n, std::map<std::string, DSL::Node>&, const std::map<std::string, DSL::Node>&) {
  static int seq = 0;
  std::string p = "_q" + std::to_string(++seq);
  Spec s;
  for (auto& k : n.kids) line(s, k);
  if (s.order.empty()) throw DSL::Error(n.line, "sequence: it has no participants");
  size_t longest = 0;
  for (auto& m : s.messages) longest = std::max(longest, m.text.size());
  std::vector<long> width;
  long widest = 0;
  for (auto& id : s.order) {
    width.push_back(std::max<long>(90, (long)s.label[id].size() * 9 + 28));
    widest = std::max(widest, width.back());
  }
  long pitch = std::max<long>({160, widest + 40, (long)longest * 9 + 40}), step = std::stol(n.attr("step", "44"));
  long bottom = 36 + step * (long)(s.messages.size() + 1);
  std::map<std::string, long> cx;
  std::string out, arrows;
  for (size_t i = 0; i < s.order.size(); i++) {
    cx[s.order[i]] = widest / 2 + (long)i * pitch;
    out += "box " + p + s.order[i] + " \"" + s.label[s.order[i]] + "\" at=" + std::to_string(cx[s.order[i]] - width[i] / 2) + ",0 w=" + std::to_string(width[i]) + "\n";
    out += "spacer " + p + "end" + s.order[i] + " at=" + std::to_string(cx[s.order[i]]) + "," + std::to_string(bottom) + " w=0 h=0\n";
    arrows += p + s.order[i] + ".bottom -- " + p + "end" + s.order[i] + " dash=\"5 4\"\n";
  }
  for (size_t i = 0; i < s.messages.size(); i++) {
    auto& m = s.messages[i];
    std::string a = p + "a" + std::to_string(i), b = p + "b" + std::to_string(i), y = std::to_string(36 + step * (long)(i + 1));
    out += "spacer " + a + " at=" + std::to_string(cx[m.from]) + "," + y + " w=0 h=0\n";
    out += "spacer " + b + " at=" + std::to_string(cx[m.to]) + "," + y + " w=0 h=0\n";
    arrows += a + " -> " + b + (m.text.empty() ? "" : " \"" + m.text + "\"") + (m.dashed ? " dash=\"6 4\"" : "") + "\n";
  }
  return DSL::parse(out + arrows).kids;
}

inline const bool registered = (DSL::macros()["sequence"] = sequence, true);

}
