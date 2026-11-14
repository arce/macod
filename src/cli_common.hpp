#pragma once
// What the command-line programs share.
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "dsl_tec.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define CLI_SEPS "/\\"
#else
#define CLI_SEPS "/"
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

namespace cli {

inline bool read(const std::string& path, std::string& out) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return false;
  std::stringstream s;
  s << f.rdbuf();
  out = s.str();
  return true;
}

// a library by name in a directory or in one of its subdirectories
inline bool readLib(const std::string& dir, const std::string& n, std::string& out) {
  if (read(dir + "/" + n, out)) return true;
  std::vector<std::string> subs;
  std::error_code ec;
  for (auto& e : std::filesystem::directory_iterator(dir, ec)) {
    std::string s = e.path().filename().string();
    if (s[0] != '.' && s.find('.') == std::string::npos) subs.push_back(s);
  }
  std::sort(subs.begin(), subs.end());
  for (auto& s : subs) if (read(dir + "/" + s + "/" + n, out)) return true;
  return false;
}

inline std::string dirOf(const std::string& p) {
  size_t i = p.find_last_of(CLI_SEPS);
  return i == std::string::npos ? "." : i == 0 ? p.substr(0, 1) : p.substr(0, i);
}

// the directory of the running executable (empty if the system does not say)
inline std::string exeDir() {
  std::error_code ec;
  std::filesystem::path p;
#if defined(_WIN32)
  char b[MAX_PATH];
  DWORD n = GetModuleFileNameA(nullptr, b, MAX_PATH);
  if (n > 0 && n < MAX_PATH) p = std::string(b, n);
#elif defined(__APPLE__)
  char b[4096];
  uint32_t n = sizeof b;
  if (_NSGetExecutablePath(b, &n) == 0) p = std::filesystem::weakly_canonical(b, ec);
#else
  p = std::filesystem::read_symlink("/proc/self/exe", ec);
#endif
  return p.empty() ? std::string() : p.parent_path().string();
}

inline std::string stem(const std::string& p) {
  std::string f = p.substr(p.find_last_of('/') == std::string::npos ? 0 : p.find_last_of('/') + 1);
  size_t d = f.rfind('.');
  return d == std::string::npos || d == 0 ? f : f.substr(0, d);
}

// Libraries are files: they are looked for in the directory of the input file, the -L ones and the installed one.
inline DSL::Resolver resolver(const std::vector<std::string>& dirs) {
  return [&dirs](const std::string& n) {
    std::string t;
    bool css = n.size() > 4 && n.substr(n.size() - 4) == ".css";
    bool file = n.find('.') != std::string::npos && !css;
    for (auto& d : dirs) if (file ? read(d + "/" + n, t) : readLib(d, css ? n : n + ".dg", t)) return t;
    return std::string();
  };
}

// the names of the libraries (.dg) and themes (NAME.css) that the directories have, in them or in their subdirectories
inline std::vector<std::string> libraries(const std::vector<std::string>& dirs) {
  std::vector<std::string> names;
  for (auto& d : dirs) {
    std::error_code ec;
    for (std::filesystem::recursive_directory_iterator it(d, ec), end; !ec && it != end; it.increment(ec)) {
      auto p = it->path();
      if (!it->is_regular_file() || p.filename().string()[0] == '.') continue;
      if (p.extension() == ".dg") names.push_back(p.stem().string());
      else if (p.extension() == ".css") names.push_back(p.filename().string());
    }
  }
  std::sort(names.begin(), names.end());
  names.erase(std::unique(names.begin(), names.end()), names.end());
  return names;
}

}
