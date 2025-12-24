#include "Protocol.hpp"

namespace protocol
{
    bool Client::connect(const std::string& host, int port)
    {
        return _client.connect(host, port);
    }

    void Client::disconnect()
    {
        _client.disconnect();
    }

    bool Client::send_packet(PacketType type, const std::vector<char>& payload)
    {
        int packet_size = static_cast<int>(sizeof(int) * 2 + payload.size());
        int type_int = static_cast<int>(type);

        if (!_client.send_int(packet_size))
            return false;
        if (!_client.send_int(type_int))
            return false;
        if (!payload.empty() && !_client.send_bytes(payload))
            return false;

        return true;
    }

    bool Client::recv_packet(PacketType& out_type, std::vector<char>& out_payload)
    {
        int packet_size;
        if (!_client.recv_int(packet_size))
            return false;

        int type_int;
        if (!_client.recv_int(type_int))
            return false;

        out_type = static_cast<PacketType>(type_int);

        int payload_size = packet_size - sizeof(int) * 2;
        if (payload_size > 0)
        {
            if (!_client.recv_bytes(out_payload, payload_size))
                return false;
        }
        else
        {
            out_payload.clear();
        }

        return true;
    }

    std::vector<ProcessInfo> Client::get_process_list()
    {
        if (!send_packet(PacketType::GetProcessListReq, {}))
            return {};

        PacketType response_type;
        std::vector<char> payload;
        if (!recv_packet(response_type, payload))
            return {};

        if (response_type != PacketType::GetProcessListRes)
            return {};

        return Serializer::deserialize_process_list(payload);
    }

    std::vector<char> Client::read_memory(int process_id, uint64_t address, int size)
    {
        auto payload = Serializer::serialize_read_memory_req(process_id, address, size);
        if (!send_packet(PacketType::ReadMemoryReq, payload))
            return {};

        PacketType response_type;
        std::vector<char> response_payload;
        if (!recv_packet(response_type, response_payload))
            return {};

        if (response_type != PacketType::ReadMemoryRes)
            return {};

        return Serializer::deserialize_memory_data(response_payload);
    }

    bool Client::write_memory(int process_id, uint64_t address, const char* data, int size)
    {
        auto payload = Serializer::serialize_write_memory_req(process_id, address, data, size);
        if (!send_packet(PacketType::WriteMemoryReq, payload))
            return false;

        PacketType response_type;
        std::vector<char> response_payload;
        if (!recv_packet(response_type, response_payload))
            return false;

        return response_type == PacketType::WriteMemoryRes && !response_payload.empty();
    }

    std::vector<char> Serializer::serialize_read_memory_req(int process_id, uint64_t address, int size)
    {
        std::vector<char> data(sizeof(int) + sizeof(uint64_t) + sizeof(int));
        char* ptr = data.data();

        std::memcpy(ptr, &process_id, sizeof(process_id));
        ptr += sizeof(process_id);
        std::memcpy(ptr, &address, sizeof(address));
        ptr += sizeof(address);
        std::memcpy(ptr, &size, sizeof(size));

        return data;
    }

    std::vector<char> Serializer::serialize_write_memory_req(int process_id, uint64_t address, const char* write_data, int size)
    {
        std::vector<char> data(sizeof(int) + sizeof(uint64_t) + sizeof(int) + size);
        char* ptr = data.data();

        std::memcpy(ptr, &process_id, sizeof(process_id));
        ptr += sizeof(process_id);
        std::memcpy(ptr, &address, sizeof(address));
        ptr += sizeof(address);
        std::memcpy(ptr, &size, sizeof(size));
        ptr += sizeof(size);
        std::memcpy(ptr, write_data, size);

        return data;
    }

    std::vector<ProcessInfo> Serializer::deserialize_process_list(const std::vector<char>& data)
    {
        std::vector<ProcessInfo> processes;

        if (data.size() < sizeof(int))
            return processes;

        const char* ptr = data.data();
        int count;
        std::memcpy(&count, ptr, sizeof(count));
        ptr += sizeof(count);

        const char* end = data.data() + data.size();

        for (int i = 0; i < count && ptr < end; ++i)
        {
            if (ptr + sizeof(int) * 2 > end)
                break;

            ProcessInfo info;
            std::memcpy(&info.process_id, ptr, sizeof(int));
            ptr += sizeof(int);

            int name_len;
            std::memcpy(&name_len, ptr, sizeof(int));
            ptr += sizeof(int);

            if (name_len < 0 || ptr + name_len > end)
                break;

            info.name.assign(ptr, ptr + name_len);
            ptr += name_len;

            processes.push_back(std::move(info));
        }

        return processes;
    }

    std::vector<char> Serializer::deserialize_memory_data(const std::vector<char>& data)
    {
        return data;
    }
}
