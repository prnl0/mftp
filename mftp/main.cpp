#include <iostream>

#include "mftpi.h"

int main()
{
  mftpi ftp;
  try {
    ftp.connect(L"host");
    if (!ftp.login("user", "pass")) {
      std::cerr << "failed to login\n";
    }
    ftp.working_dir("/home/user/ftp/upload/");
    ftp.transfer_file(
      "local", "remote", mftpi::transfer_direction::up, 10_MiB,
      [](std::streampos cur, std::streampos end) {
        std::cout << (cur / 1024 / 1024) << "MiB/" << (end / 1024 / 1024) << "MiB\n";
      }
    );
    ftp.transfer_file(
      "local", "remote", mftpi::transfer_direction::down, 10_MiB,
      [](std::streampos cur, std::streampos end) {
        std::cout << (cur / 1024 / 1024) << "MiB/" << (end / 1024 / 1024) << "MiB\n";
      }
    );
  } catch (std::exception e) {
    std::cerr << e.what() << '\n';
  }
  return 0;
}