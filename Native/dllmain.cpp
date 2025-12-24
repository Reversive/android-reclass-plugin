#include <cstdint>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <ReClassNET_Plugin.hpp>
#include "Protocol.hpp"

namespace
{
    constexpr const char* DEFAULT_HOST = "127.0.0.1";
    constexpr int DEFAULT_PORT = 27042;

    protocol::Client g_client;
    std::mutex g_client_mutex;

    struct ProcessHandle
    {
        int process_id;
        std::atomic<bool> valid{true};
    };

    std::unordered_map<RC_Pointer, ProcessHandle*> g_handles;
    std::mutex g_handles_mutex;
    std::atomic<uintptr_t> g_next_handle{1};

    bool ensure_connected()
    {
        if (g_client.is_connected())
            return true;
        return g_client.connect(DEFAULT_HOST, DEFAULT_PORT);
    }
}

extern "C" void RC_CallConv EnumerateProcesses(EnumerateProcessCallback* callbackProcess)
{
    std::lock_guard<std::mutex> lock(g_client_mutex);

    if (!ensure_connected())
        return;

    auto processes = g_client.get_process_list();

    for (const auto& proc : processes)
    {
        EnumerateProcessData data{};
        data.Id = static_cast<RC_Size>(proc.process_id);
        MultiByteToUnicode(proc.name.c_str(), data.Name, PATH_MAXIMUM_LENGTH);
        MultiByteToUnicode(proc.name.c_str(), data.Path, PATH_MAXIMUM_LENGTH);

        callbackProcess(&data);
    }
}

extern "C" void RC_CallConv EnumerateRemoteSectionsAndModules(
    RC_Pointer handle,
    EnumerateRemoteSectionsCallback* callbackSection,
    EnumerateRemoteModulesCallback* callbackModule)
{
}

extern "C" RC_Pointer RC_CallConv OpenRemoteProcess(RC_Size id, ProcessAccess desiredAccess)
{
    int process_id = static_cast<int>(id);

    auto* ph = new ProcessHandle();
    ph->process_id = process_id;
    ph->valid = true;

    RC_Pointer handle = reinterpret_cast<RC_Pointer>(g_next_handle.fetch_add(1));

    {
        std::lock_guard<std::mutex> lock(g_handles_mutex);
        g_handles[handle] = ph;
    }

    return handle;
}

extern "C" bool RC_CallConv IsProcessValid(RC_Pointer handle)
{
    std::lock_guard<std::mutex> lock(g_handles_mutex);

    auto it = g_handles.find(handle);
    if (it == g_handles.end())
        return false;

    return it->second->valid.load();
}

extern "C" void RC_CallConv CloseRemoteProcess(RC_Pointer handle)
{
    std::lock_guard<std::mutex> lock(g_handles_mutex);

    auto it = g_handles.find(handle);
    if (it != g_handles.end())
    {
        delete it->second;
        g_handles.erase(it);
    }
}

extern "C" bool RC_CallConv ReadRemoteMemory(
    RC_Pointer handle,
    RC_Pointer address,
    RC_Pointer buffer,
    int offset,
    int size)
{
    ProcessHandle* ph = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_handles_mutex);
        auto it = g_handles.find(handle);
        if (it == g_handles.end())
            return false;
        ph = it->second;
    }

    if (!ph->valid.load())
        return false;

    std::lock_guard<std::mutex> lock(g_client_mutex);

    if (!ensure_connected())
    {
        ph->valid = false;
        return false;
    }

    uint64_t addr = reinterpret_cast<uint64_t>(address);
    auto data = g_client.read_memory(ph->process_id, addr, size);

    if (data.empty())
    {
        return false;
    }

    std::memcpy(static_cast<char*>(buffer) + offset, data.data(), data.size());
    return true;
}

extern "C" bool RC_CallConv WriteRemoteMemory(
    RC_Pointer handle,
    RC_Pointer address,
    RC_Pointer buffer,
    int offset,
    int size)
{
    ProcessHandle* ph = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_handles_mutex);
        auto it = g_handles.find(handle);
        if (it == g_handles.end())
            return false;
        ph = it->second;
    }

    if (!ph->valid.load())
        return false;

    std::lock_guard<std::mutex> lock(g_client_mutex);

    if (!ensure_connected())
    {
        ph->valid = false;
        return false;
    }

    uint64_t addr = reinterpret_cast<uint64_t>(address);
    const char* src = static_cast<const char*>(buffer) + offset;

    return g_client.write_memory(ph->process_id, addr, src, size);
}

extern "C" void RC_CallConv ControlRemoteProcess(RC_Pointer handle, ControlRemoteProcessAction action)
{
}

extern "C" bool RC_CallConv AttachDebuggerToProcess(RC_Size id)
{
    return false;
}

extern "C" void RC_CallConv DetachDebuggerFromProcess(RC_Size id)
{
}

extern "C" bool RC_CallConv AwaitDebugEvent(DebugEvent* evt, int timeoutInMilliseconds)
{
    return false;
}

extern "C" void RC_CallConv HandleDebugEvent(DebugEvent* evt)
{
}

extern "C" bool RC_CallConv SetHardwareBreakpoint(
    RC_Size id,
    RC_Pointer address,
    HardwareBreakpointRegister reg,
    HardwareBreakpointTrigger type,
    HardwareBreakpointSize size,
    bool set)
{
    return false;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_PROCESS_DETACH:
        g_client.disconnect();
        break;
    }
    return TRUE;
}
