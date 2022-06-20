#include "mftpi.h"

#include <vector>
#include <fstream>
#include <filesystem>
#include <numeric>

#include "common.h"

const static inline auto get_pasv_port = [](std::string pasv_response) -> unsigned long {
  const auto port1_beg = pasv_response.find_last_of(',') + 1;
  const auto port1_end = pasv_response.find_last_of(')');
  const auto port1 = std::stoul(pasv_response.substr(port1_beg, port1_end - port1_beg));
  pasv_response = pasv_response.substr(0, port1_beg - 1);
  const auto port2 = 256 * std::stoul(pasv_response.substr(pasv_response.find_last_of(',') + 1));
  return port1 + port2;
};

void mftpi::connect(const std::wstring &host)
{
  ctrl.connect(this->host = host, 21);
  ctrl.receive();
}

bool mftpi::login(const std::string &user, const std::string &pass) const
{
  ctrl.sendr("USER " + user);
  ctrl.send("PASS " + pass);
  return !ctrl.receive().contains("Login incorrect");
}

std::string mftpi::working_dir() const
{
  ctrl.send("PWD");
  auto resp = ctrl.receive();
  auto quote_beg = resp.find_first_of('"') + 1;
  auto quote_end = resp.find_last_of('"');
  return resp.substr(quote_beg, quote_end - quote_beg);
}

void mftpi::working_dir(const std::string &dir) const
{
  ctrl.send("CWD " + dir);
  if (auto reply = ctrl.receive(); reply[0] != '2') {
    throw std::runtime_error(mftp::trim(reply).substr(4));
  }
}

std::vector<mftpi::directory_entry> mftpi::dir_listing(const std::string &dir) const
{
  if (!dir.empty()) {
    working_dir(dir);
  }
  ctrl.sendr("TYPE A"); // set data transfer type to ASCII
  ctrl.send("PASV"); // set passive mode
  auto xfer = std::make_unique<mftp::socket>();
  xfer->connect(host, get_pasv_port(ctrl.receive()));
  ctrl.sendr("LIST -a"); // list everything
  std::string resp;
  for (std::string recv; !(recv = xfer->receive()).empty();) {
    resp += recv;
  }
  ctrl.receive();
  std::vector<directory_entry> listing;
  for (const auto &str : mftp::split(resp, "\n")) {
    const auto entry = parse_dir_entry(str);
    if (entry.name == "." && is_directory(entry.name)) {
      continue;
    }
    listing.push_back(entry);
  }
  return listing;
}

bool mftpi::is_directory(const std::string &filepath) const
{
  ctrl.send("SIZE " + filepath);
  return ctrl.receive().contains("550");
}

#include <QDebug>
bool mftpi::transfer_file(
  const std::filesystem::path &local_path,
  const std::filesystem::path &remote_path,
  transfer_direction dir,
  std::uint64_t part_sz,
  std::function<void(std::streampos, std::streampos)> callback
) const
{
  assertm(
    part_sz <= static_cast<std::uint64_t>((std::numeric_limits<int>::max)()),
    "mftpi::transfer_file: partition size is too large (max. is INT_MAX)"
  );
  std::fstream fs(
    local_path.wstring(),
    (dir == transfer_direction::up ? std::ios_base::in : std::ios_base::out)
      | std::ios_base::binary
  );
  if (!fs) {
    return false;
  }
  ctrl.sendr("TYPE I"); // set data transfer type to binary
  ctrl.send("PASV"); // set passive mode
  auto xfer = std::make_unique<mftp::socket>();
  xfer->connect(host, get_pasv_port(ctrl.receive()));
  ctrl.send(
    dir == transfer_direction::up
      ? "STOR " + local_path.filename().string()
      : "RETR " + remote_path.filename().string()
  );
  if (auto resp = ctrl.receive(); resp.contains("150 ")) {
    transfering = true;
    const auto end = dir == transfer_direction::up
      ? fs.seekg(0, std::ios_base::end).tellg()
      : std::streampos(std::stoull(
          resp.substr(resp.find_last_of('(') + 1, resp.find_last_of(' ') - resp.find_last_of('('))
        ));
    fs.seekg(0, std::ios::beg);
    std::vector<char> buff(part_sz, '\0');
    while (fs.tellg() != end) {
      auto bytes = static_cast<std::uint64_t>(end - fs.tellg()) >= part_sz
        ? part_sz : end - fs.tellg();
      std::memset(buff.data(), 0, bytes);
      if (dir == transfer_direction::up) {
        fs.read(buff.data(), bytes);
        xfer->send(buff.data(), static_cast<int>(bytes));
      } else {
        bytes = xfer->receive(buff.data(), static_cast<int>(bytes));
        fs.write(buff.data(), bytes);
      }
      if (callback) {
        callback(fs.tellg(), end);
      }
    }
    xfer.reset();
  } else {
    transfering = false;
    return false;
  }
  ctrl.receive();
  transfering = false;
  return true;
}

std::string mftpi::command(const std::string &cmd) const
{
  ctrl.send(cmd);
  return ctrl.receive(10000);
}

mftpi::directory_entry mftpi::parse_dir_entry(const std::string &entry) const
{
  auto split = mftp::split(entry, " ");
  std::string filename;
  for (decltype(split)::size_type i = 8; i != split.size(); ++i) {
    filename += split[i] + " ";
  }
  filename.pop_back();
  return directory_entry{
    .rights = split[0],
    .group = std::stoul(split[2]),
    .owner = std::stoul(split[3]),
    .size = std::stoull(split[4]),
    .last_mod_date = split[5] + ' ' + split[6] + ' ' + split[7],
    .name = mftp::trim(filename)
  };
}
