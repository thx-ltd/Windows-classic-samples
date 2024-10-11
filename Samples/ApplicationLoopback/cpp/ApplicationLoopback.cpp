// ApplicationLoopback.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include "LoopbackCapture.h"

#include "thx/windows/tracelogging.hpp"
#include "thx/wstring_conversion.hpp"

void usage()
{
    std::wcout <<
        L"Usage: ApplicationLoopback <pid> <includetree|excludetree> <outputfilename>\n"
        L"\n"
        L"<pid> is the process ID to capture or exclude from capture\n"
        L"includetree includes audio from that process and its child processes\n"
        L"excludetree includes audio from all processes except that process and its child processes\n"
        L"<outputfilename> is the WAV file to receive the captured audio (10 seconds)\n"
        L"\n"
        L"Examples:\n"
        L"\n"
        L"ApplicationLoopback 1234 includetree CapturedAudio.wav\n"
        L"\n"
        L"  Captures audio from process 1234 and its children.\n"
        L"\n"
        L"ApplicationLoopback 1234 excludetree CapturedAudio.wav\n"
        L"\n"
        L"  Captures audio from all processes except process 1234 and its children.\n";
}

namespace
{
    std::atomic_flag g_UserExit;
}

BOOL WINAPI ConsoleHandler(DWORD signal)
{
    if (signal == CTRL_C_EVENT)
    {
        std::wcout << L"CTRL_C_EVENT" << std::endl;
        g_UserExit.test_and_set();
        g_UserExit.notify_all();

        return TRUE;
    }
    return FALSE;
}


int wmain(int argc, wchar_t* argv[])
{
    thx::logging::open();

    if (argc != 4)
    {
        usage();
        return 0;
    }

    // Search for a process with the name in argv[1]. If found, use that process ID.

    DWORD processId = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        std::wcout << L"Failed to create snapshot of processes." << std::endl;
        return 0;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe32))
    {
        do
        {
            if (_wcsicmp(pe32.szExeFile, argv[1]) == 0)
            {
                processId = pe32.th32ProcessID;
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);

    if (processId == 0)
    {
        std::wcout << L"Process not found: " << argv[1] << std::endl;
        processId = wcstoul(argv[1], nullptr, 0);
        if (processId == 0)
        {
            usage();
            return 0;
        }
    }

    bool includeProcessTree;
    if (wcscmp(argv[2], L"includetree") == 0)
    {
        includeProcessTree = true;
    }
    else if (wcscmp(argv[2], L"excludetree") == 0)
    {
        includeProcessTree = false;
    }
    else
    {
        usage();
        return 0;
    }

    PCWSTR outputFile = argv[3];

    CLoopbackCapture loopbackCapture;
    HRESULT hr = loopbackCapture.StartCaptureAsync(processId, includeProcessTree, outputFile);
    if (FAILED(hr))
    {
        wil::unique_hlocal_string message;
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER, nullptr, hr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (PWSTR)&message, 0, nullptr);
        std::wcout << L"Failed to start capture\n0x" << std::hex << hr << L": " << message.get() << L"\n";
    }
    else
    {
        std::wcout << L"Capturing audio from process Id " << processId << " (" << argv[1] << ") until Ctrl - C is pressed." << std::endl;
        if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
            printf("Failed to install console handler!\n");
            return -1;
        }

        g_UserExit.clear();

        bool userRequestedExit = g_UserExit.test();
        while (!(userRequestedExit = g_UserExit.test()))
        {
            g_UserExit.wait(userRequestedExit);
        }

        loopbackCapture.StopCaptureAsync();

        std::wcout << L"Finished.\n";
    }

    return 0;
}
