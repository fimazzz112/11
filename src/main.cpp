#include <windows.h>
#include <tlhelp32.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

std::string TimeNow()
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
    localtime_s(&tm, &t);

    std::ostringstream out;
    out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

void Log(std::ofstream& file, const std::string& text)
{
    std::string line = "[" + TimeNow() + "] " + text;
    std::cout << line << '\n';
    file << line << std::endl;
}

DWORD FindRust()
{
    HANDLE snapshot =
        CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

if (snapshot == INVALID_HANDLE_VALUE)
{
    return 0;
}

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    DWORD pid = 0;

    if (Process32FirstW(snapshot, &entry))
    {
        do
        {
            if (_wcsicmp(entry.szExeFile, L"RustClient.exe") == 0)
            {
                pid = entry.th32ProcessID;
                break;
            }
        }
        while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return pid;
}

std::string Narrow(const std::wstring& value)
{
    if (value.empty())
        return {};

    int size = WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0,
        nullptr,
        nullptr);

    if (size <= 0)
        return {};

    std::string result(size, '\0');

    WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        result.data(),
        size,
        nullptr,
        nullptr);

    return result;
}

std::set<std::wstring> GetModules(
    DWORD pid,
    std::ofstream& log)
{
    std::set<std::wstring> modules;

    SetLastError(ERROR_SUCCESS);

    HANDLE snapshot =
        CreateToolhelp32Snapshot(
            TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
            pid);

    if (snapshot == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();

        Log(
            log,
            "Module enumeration FAILED. "
            "GetLastError = " +
            std::to_string(error));

        return modules;
    }

    MODULEENTRY32W module{};
    module.dwSize = sizeof(module);

    if (!Module32FirstW(snapshot, &module))
    {
        DWORD error = GetLastError();

        Log(
            log,
            "Module32FirstW FAILED. "
            "GetLastError = " +
            std::to_string(error));

        CloseHandle(snapshot);
        return modules;
    }

    do
    {
        modules.insert(module.szModule);

    }
    while (Module32NextW(snapshot, &module));

    DWORD finalError = GetLastError();

    CloseHandle(snapshot);

    Log(
        log,
        "Module enumeration SUCCESS. "
        "Module count = " +
        std::to_string(modules.size()) +
        ", final error = " +
        std::to_string(finalError));

    return modules;
}

int main()
{
    std::ofstream log(
        "rust_monitor.log",
        std::ios::app);

    if (!log)
    {
        std::cerr
            << "Cannot open rust_monitor.log\n";
        return 1;
    }

    Log(log, "==============================");
    Log(log, "Rust Monitor 2.0 started.");
    Log(log, "==============================");

    DWORD pid = 0;

    while ((pid = FindRust()) == 0)
    {
        Log(
            log,
            "Waiting for RustClient.exe...");

        std::this_thread::sleep_for(
            std::chrono::seconds(2));
    }

    Log(
        log,
        "RustClient.exe detected. PID = " +
        std::to_string(pid));

    HANDLE process = OpenProcess(
        PROCESS_QUERY_LIMITED_INFORMATION,
        FALSE,
        pid);

    if (!process)
    {
        Log(
            log,
            "OpenProcess FAILED. GetLastError = " +
            std::to_string(GetLastError()));
    }
    else
    {
        Log(log, "OpenProcess SUCCESS.");
        CloseHandle(process);
    }

    auto previousModules =
        GetModules(pid, log);

    for (const auto& module : previousModules)
    {
        Log(
            log,
            "Initial module: " +
            Narrow(module));
    }

    while (true)
    {
        HANDLE checkProcess = OpenProcess(
            PROCESS_QUERY_LIMITED_INFORMATION,
            FALSE,
            pid);

        if (!checkProcess)
        {
            Log(
                log,
                "Rust process is no longer accessible. "
                "GetLastError = " +
                std::to_string(GetLastError()));

            break;
        }

        CloseHandle(checkProcess);

        auto currentModules =
            GetModules(pid, log);

        for (const auto& module : currentModules)
        {
            if (!previousModules.contains(module))
            {
                Log(
                    log,
                    "Module appeared: " +
                    Narrow(module));
            }
        }

        for (const auto& module : previousModules)
        {
            if (!currentModules.contains(module))
            {
                Log(
                    log,
                    "Module disappeared: " +
                    Narrow(module));
            }
        }

        previousModules =
            std::move(currentModules);

        std::this_thread::sleep_for(
            std::chrono::seconds(2));
    }

    Log(log, "Rust Monitor stopped.");

    return 0;
}
