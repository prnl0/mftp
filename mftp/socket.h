#pragma comment(lib, "Ws2_32.lib")
#pragma once

#include <string>

#include <winsock2.h>
#include <ws2tcpip.h>

namespace mftp
{
  class socket
  {
    public:
      socket(int family = AF_UNSPEC, int socktype = SOCK_STREAM, int protocol = IPPROTO_TCP);
      ~socket();

      void connect(const std::wstring &host, unsigned port);
      void disconnect();

      void send(const char *data, int len) const;
      void send(std::string data) const;

      int receive(char *buff, int len, DWORD timeout = 0) const;
      std::string receive(DWORD timeout = 0, int len = 512) const;

      void sendr(const std::string &data) const
      {
        send(data);
        receive();
      }

    private:
      static WSADATA wsa_data;
      static bool wsa_initialized;

      static unsigned sockets_connected;
      static unsigned socket_count;

    protected:
      ADDRINFOW hints;
      SOCKET sock = INVALID_SOCKET;
  };
}
