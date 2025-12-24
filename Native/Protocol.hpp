#pragma once

#include "TcpClient.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

namespace protocol
{
    enum class PacketType : int
    {
        Error = -1,
        GetProcessListReq = 0,
        ReadMemoryReq = 1,
        WriteMemoryReq = 2,
        GetProcessListRes = 3,
        ReadMemoryRes = 4,
        WriteMemoryRes = 5
    };

    struct ProcessInfo
    {
        int process_id;
        std::string name;
    };

    class Client
    {
    public:
        bool connect(const std::string& host, int port);
        void disconnect();
        bool is_connected() const { return _client.is_connected(); }

        std::vector<ProcessInfo> get_process_list();
        std::vector<char> read_memory(int process_id, uint64_t address, int size);
        bool write_memory(int process_id, uint64_t address, const char* data, int size);

    private:
        bool send_packet(PacketType type, const std::vector<char>& payload);
        bool recv_packet(PacketType& out_type, std::vector<char>& out_payload);

        network::TcpClient _client;
    };

    class Serializer
    {
    public:
        static std::vector<char> serialize_read_memory_req(int process_id, uint64_t address, int size);
        static std::vector<char> serialize_write_memory_req(int process_id, uint64_t address, const char* data, int size);

        static std::vector<ProcessInfo> deserialize_process_list(const std::vector<char>& data);
        static std::vector<char> deserialize_memory_data(const std::vector<char>& data);
    };
}
