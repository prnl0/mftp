#include "common.h"

#include <vector>
#include <string>
#include <cstddef>

std::vector<std::string> mftp::split(
  std::string str, const std::string &delim, bool ignore_empty_tokens
) {
  std::vector<std::string> ret;
  std::string sub;
  std::size_t beg = 0;
  std::size_t end = 0;
  while ((end = str.find(delim, beg)) != std::string::npos) {
    if (!ignore_empty_tokens || !(sub = str.substr(beg, end - beg)).empty()) {
      ret.push_back(sub);
    }
    beg = end + 1;
  }
  if (!ignore_empty_tokens || !(sub = str.substr(beg)).empty()) {
    ret.push_back(sub);
  }
  return ret;
}

std::string &mftp::trim(std::string &str)
{
  str.erase(str.find_last_not_of(" \t\r\n\f\v") + 1);
  str.erase(0, str.find_first_not_of(" \t\r\n\f\v"));
  return str;
}
