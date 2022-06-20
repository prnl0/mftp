#include "socket.h"

#include <format>
#include <iostream>
#include <string>
#include <cassert>

#include <winsock2.h>
#include <ws2tcpip.h>

#include "common.h"

using namespace mftp;

WSADATA socket::wsa_data;
bool socket::wsa_initialized = false;

unsigned socket::sockets_connected = 0;
unsigned socket::socket_count = 0;

socket::socket(int family, int socktype, int protocol)
{
  if (!wsa_initialized) {
    assertm(
      ::WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0,
      std::format("socket::socket: WSAStartup failed ({})", ::WSAGetLastError())
    );
    wsa_initialized = true;
  }
  ::ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = family;
  hints.ai_socktype = socktype;
  hints.ai_protocol = protocol;
  ++socket_count;
}

socket::~socket()
{
  disconnect();
  if (--socket_count == 0) {
    ::WSACleanup();
    wsa_initialized = false;
  }
}

void socket::connect(const std::wstring &host, unsigned port)
{
  ADDRINFOW *addr_res = nullptr;
  if (::GetAddrInfoW(host.c_str(), std::to_wstring(port).c_str(), &hints, &addr_res) != 0) {
    throw_system_error(::WSAGetLastError(), "socket::connect: getaddrinfo failed");
  }
  for (auto node = addr_res; node; node = node->ai_next) {
    if (
      (sock = ::socket(node->ai_family, node->ai_socktype, node->ai_protocol)) != INVALID_SOCKET
      && ::connect(sock, node->ai_addr, static_cast<int>(node->ai_addrlen)) == 0
    ) {
      ::FreeAddrInfoW(addr_res);
      ++sockets_connected;
      return;
    }
  }
  ::FreeAddrInfoW(addr_res);
  throw_system_error(::WSAGetLastError(), "socket::connect: could not create/connect socket ({})");
}

void socket::disconnect()
{
  if (sock != INVALID_SOCKET) {
    ::shutdown(sock, SD_BOTH);
    ::closesocket(sock);
    sock = INVALID_SOCKET;
    --sockets_connected;
  }
}

void socket::send(const char *data, int len) const
{
  if (::send(sock, data, len, 0) == SOCKET_ERROR) {
    throw_system_error(::WSAGetLastError(), "socket::send: could not send data ({})");
  }
}

void socket::send(std::string data) const
{
  if (data.size() < 2 || data[data.size() - 2] != '\r') {
    data += '\r';
  }
  if (data.back() != '\n') {
    data += '\n';
  }
  send(data.c_str(), static_cast<int>(data.size()));
}

int socket::receive(char *buff, int len, DWORD timeout) const
{
  ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char *>(&timeout), sizeof(DWORD));
  std::memset(buff, '\0', len);
  len = recv(sock, buff, len, 0);
  if (len < 0) {
    throw_system_error(::WSAGetLastError(), "socket::receive: could not receive data ({})");
  }
  return len;
}

std::string socket::receive(DWORD timeout, int len) const
{
  std::string recv_buff(len, '\0');
  len = receive(recv_buff.data(), len, timeout);
  recv_buff.resize(len);
  return recv_buff;
}
