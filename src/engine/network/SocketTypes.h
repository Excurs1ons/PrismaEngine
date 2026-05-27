#pragma once

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    using SOCKET_HANDLE = SOCKET;
    static constexpr SOCKET_HANDLE INVALID_SOCKET_VALUE = INVALID_SOCKET;
    static constexpr int SOCKET_ERROR_RET = SOCKET_ERROR;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <poll.h>
    #include <cerrno>
    using SOCKET_HANDLE = int;
    static constexpr SOCKET_HANDLE INVALID_SOCKET_VALUE = -1;
    static constexpr int SOCKET_ERROR_RET = -1;
    static constexpr int INVALID_SOCKET = -1;
    #define closesocket(fd) close(fd)
#endif
