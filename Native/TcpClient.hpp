#pragma once

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <cstdint>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

namespace network
{
    class TcpClient
    {
    public:
        TcpClient();
        ~TcpClient();

        TcpClient(const TcpClient&) = delete;
        TcpClient& operator=(const TcpClient&) = delete;

        bool connect(const std::string& host, int port);
        void disconnect();
        bool is_connected() const { return _socket != INVALID_SOCKET; }

        bool send_all(const char* data, size_t size);
        bool recv_all(char* buffer, size_t size);

        bool send_int(int value);
        bool recv_int(int& out_value);
        bool send_bytes(const std::vector<char>& data);
        bool recv_bytes(std::vector<char>& out_buffer, size_t size);

    private:
        SOCKET _socket = INVALID_SOCKET;
        static bool _wsa_initialized;
        static bool init_wsa();
    };
}
