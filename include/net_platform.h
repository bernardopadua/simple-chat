#pragma once
#ifdef _WIN32
#include <WinSock2.h>
using socket_t = SOCKET;
inline constexpr socket_t invalid_socket = INVALID_SOCKET;
#else
using socket_t = int;
inline constexpr socket_t invalid_socket = -1;
#endif