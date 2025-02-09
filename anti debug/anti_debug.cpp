#include "anti_debug.h"
#include <intrin.h>
#include <windows.h>
#include <winternl.h> 
#include <imagehlp.h>
#include <iostream>

#pragma comment(lib, "ntdll.lib")

typedef NTSTATUS(WINAPI* pNtQueryInformationProcess)(
    HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG
    );


bool IsDebuggerPresentCustom() {
    return IsDebuggerPresent() != 0;
}

bool CheckRemoteDebugger() {
    BOOL isDebugger = FALSE;
    CheckRemoteDebuggerPresent(GetCurrentProcess(), &isDebugger);
    return isDebugger;
}

bool NtQueryInfoProcess() {
    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (!hNtDll) return false;

    pNtQueryInformationProcess NtQueryInformationProcess =
        (pNtQueryInformationProcess)GetProcAddress(hNtDll, "NtQueryInformationProcess");

    if (!NtQueryInformationProcess) return false;

    PROCESS_BASIC_INFORMATION pbi;
    ULONG returnLength;
    NTSTATUS status = NtQueryInformationProcess(GetCurrentProcess(), (PROCESSINFOCLASS)ProcessBasicInformation, &pbi, sizeof(pbi), &returnLength);

    if (status != 0) return false;

    BOOL isDebugged = FALSE;
    SIZE_T bytesRead;
    if (ReadProcessMemory(GetCurrentProcess(), &(pbi.PebBaseAddress->BeingDebugged), &isDebugged, sizeof(BOOL), &bytesRead)) {
        return isDebugged;
    }

    return false;
}


bool NtSetInformationThreadHide() {
    typedef NTSTATUS(NTAPI* pNtSetInformationThread)(HANDLE, THREAD_INFORMATION_CLASS, PVOID, ULONG);
    pNtSetInformationThread NtSIT = (pNtSetInformationThread)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSetInformationThread");
    if (NtSIT) {
        return NtSIT(GetCurrentThread(), (THREAD_INFORMATION_CLASS)0x11, NULL, 0) == 0;
    }
    return false;
}

bool DetectRDTSC() {
    unsigned __int64 t1, t2;
    t1 = __rdtsc();
    Sleep(10);
    t2 = __rdtsc();
    return (t2 - t1) < 100000;
}

bool DetectQueryPerformanceCounter() {
    LARGE_INTEGER t1, t2, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t1);
    Sleep(10);
    QueryPerformanceCounter(&t2);
    return (t2.QuadPart - t1.QuadPart) < (freq.QuadPart / 100);
}

bool DetectGetTickCount() {
    DWORD t1 = GetTickCount();
    Sleep(10);
    DWORD t2 = GetTickCount();
    return (t2 - t1) < 10;
}

bool DetectHardwareBreakpoints() {
    CONTEXT ctx = { 0 };
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext(GetCurrentThread(), &ctx)) {
        return (ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3);
    }
    return false;
}

bool DetectINT3() {
    __try {
        __debugbreak();
        return false;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return true;
    }
}

bool DetectINT2D() {
    __try {
        RaiseException(0x2D, 0, 0, NULL);
        return false;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return true;
    }
}

bool DetectSingleStepException() {
    __try {
        CONTEXT ctx = { 0 };
        ctx.ContextFlags = CONTEXT_CONTROL;
        GetThreadContext(GetCurrentThread(), &ctx);

        ctx.EFlags |= 0x100; 
        SetThreadContext(GetCurrentThread(), &ctx);

        return false;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return true;
    }
}
bool DetectStackPointerManipulation() {
    CONTEXT ctx = { 0 };
    ctx.ContextFlags = CONTEXT_CONTROL;

   
    if (!GetThreadContext(GetCurrentThread(), &ctx)) {
        return false; 
    }

    uintptr_t stackPointer = ctx.Rsp;

    if (stackPointer < 0x100000000 || stackPointer > 0x7FFFFFFFFFFF) {
        return true;  
    }

    return false;
}
void ProtectMemory(LPVOID lpAddress, SIZE_T dwSize) {
    DWORD oldProtect;
    VirtualProtect(lpAddress, dwSize, PAGE_NOACCESS, &oldProtect); 
}

void UnprotectMemory(LPVOID lpAddress, SIZE_T dwSize) {
    DWORD oldProtect;
    VirtualProtect(lpAddress, dwSize, PAGE_READWRITE, &oldProtect); 
}
typedef NTSTATUS(WINAPI* pNtQuerySystemInformation)(
    SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

bool DetectMemoryDumping() {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);  


    if (sysInfo.dwPageSize < 0x1000) {
        return true;  
    }

    return false;  
}
LONG WINAPI MyUnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo) {

    return EXCEPTION_EXECUTE_HANDLER; 
}

void SetupAntiDump() {
    SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
}