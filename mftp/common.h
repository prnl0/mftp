#pragma once

#include <cassert>
#include <cstdint>
#include <system_error>
#include <format>
#include <vector>
#include <string>

#define assertm(exp, msg) assert(((void)msg, exp))

inline auto operator""_MiB(std::uint64_t x) -> std::uint64_t
{
  return 1024ULL * 1024ULL * x;
}

namespace mftp
{
  inline void throw_system_error(int ecode, const std::string &msg)
  {
    throw std::system_error({ecode, std::system_category()}, std::format(msg, ecode));
  }

  std::vector<std::string> split(
    std::string str, const std::string &delim, bool ignore_empty_tokens = true
  );

  std::string &trim(std::string &str);
}
