#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <filesystem>
#include <functional>
#include <memory>

#include "socket.h"

#include "common.h"

class mftpi
{
  public:
    struct directory_entry
    {
      std::string rights;
      unsigned group;
      unsigned owner;
      std::uint64_t size;
      std::string last_mod_date;
      std::string name;
    };

    enum class transfer_direction
    {
      up,
      down
    };

  public:
    void connect(const std::wstring &host);

    bool login(const std::string &user, const std::string &pass) const;

    std::string working_dir() const;
    void working_dir(const std::string &dir) const;
    std::vector<directory_entry> dir_listing(const std::string &dir = "") const;

    bool is_directory(const std::string &filepath) const;

    bool transfer_file(
      const std::filesystem::path &local_path,
      const std::filesystem::path &remote_path,
      transfer_direction dir,
      std::uint64_t part_sz = 10_MiB,
      std::function<void(std::streampos, std::streampos)> callback = nullptr
    ) const;

    std::string command(const std::string &cmd) const;

    void cleanup() { ctrl.disconnect(); }

    bool is_transfering() const { return transfering; }
    void set_transfering (bool transfering_) const { transfering = transfering_; }

  private:
    directory_entry parse_dir_entry(const std::string &entry) const;
    
    mftp::socket ctrl;
    std::wstring host;

    mutable bool transfering = false;
};
