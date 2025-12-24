#include "TcpClient.hpp"

namespace network
{
    bool TcpClient::_wsa_initialized = false;

    bool TcpClient::init_wsa()
    {
        if (_wsa_initialized)
            return true;

        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
            return false;

        _wsa_initialized = true;
        return true;
    }

    TcpClient::TcpClient()
    {
        init_wsa();
    }

    TcpClient::~TcpClient()
    {
        disconnect();
    }

    bool TcpClient::connect(const std::string& host, int port)
    {
        if (_socket != INVALID_SOCKET)
            disconnect();

        _socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (_socket == INVALID_SOCKET)
            return false;

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(static_cast<u_short>(port));

        if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) != 1)
        {
            closesocket(_socket);
            _socket = INVALID_SOCKET;
            return false;
        }

        if (::connect(_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR)
        {
            closesocket(_socket);
            _socket = INVALID_SOCKET;
            return false;
        }

        return true;
    }

    void TcpClient::disconnect()
    {
        if (_socket != INVALID_SOCKET)
        {
            closesocket(_socket);
            _socket = INVALID_SOCKET;
        }
    }

    bool TcpClient::send_all(const char* data, size_t size)
    {
        size_t total_sent = 0;
        while (total_sent < size)
        {
            int sent = ::send(_socket, data + total_sent, static_cast<int>(size - total_sent), 0);
            if (sent == SOCKET_ERROR)
                return false;
            total_sent += sent;
        }
        return true;
    }

    bool TcpClient::recv_all(char* buffer, size_t size)
    {
        size_t total_recv = 0;
        while (total_recv < size)
        {
            int received = ::recv(_socket, buffer + total_recv, static_cast<int>(size - total_recv), 0);
            if (received <= 0)
                return false;
            total_recv += received;
        }
        return true;
    }

    bool TcpClient::send_int(int value)
    {
        return send_all(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    bool TcpClient::recv_int(int& out_value)
    {
        return recv_all(reinterpret_cast<char*>(&out_value), sizeof(out_value));
    }

    bool TcpClient::send_bytes(const std::vector<char>& data)
    {
        return send_all(data.data(), data.size());
    }

    bool TcpClient::recv_bytes(std::vector<char>& out_buffer, size_t size)
    {
        out_buffer.resize(size);
        if (!recv_all(out_buffer.data(), size))
        {
            out_buffer.clear();
            return false;
        }
        return true;
    }
}
