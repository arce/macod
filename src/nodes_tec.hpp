#pragma once
#include <map>
#include <string>
#include <vector>

namespace NODES {

struct Set {
  std::vector<std::string> order;
  std::map<std::string, std::string> label, extra;
};

inline void declare(Set& s, const std::string& id, const std::string& label = "", const std::string& extra = "") {
  if (!s.label.count(id)) s.order.push_back(id), s.label[id] = id;
  if (!label.empty()) s.label[id] = label;
  if (!extra.empty()) s.extra[id] += " " + extra;
}

}
