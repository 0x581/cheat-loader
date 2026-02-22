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

bool mem_break()
{
    DWORD dwOldProtect = 0;
    SYSTEM_INFO SysInfo = { 0 };

    GetSystemInfo(&SysInfo);
    PVOID pPage = VirtualAlloc(NULL, SysInfo.dwPageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (NULL == pPage)
        return false;

    PBYTE pMem = (PBYTE)pPage;
    *pMem = 0xC3;  

   
    if (!VirtualProtect(pPage, SysInfo.dwPageSize, PAGE_EXECUTE_READWRITE | PAGE_GUARD, &dwOldProtect))
    {
        VirtualFree(pPage, NULL, MEM_RELEASE);
        return false;
    }

    bool detected = false;
    __try
    {
        auto func = reinterpret_cast<void(*)()>(pPage);
        func(); 
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        detected = false;
    }

    VirtualFree(pPage, NULL, MEM_RELEASE);
    return detected;
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
typedef DWORD(WINAPI* TCsrGetProcessId)(VOID);

bool openProcess()
{
    HMODULE hNtdll = LoadLibraryA("ntdll.dll");
    if (!hNtdll)
        return false;

    TCsrGetProcessId pfnCsrGetProcessId = (TCsrGetProcessId)GetProcAddress(hNtdll, "CsrGetProcessId");
    if (!pfnCsrGetProcessId)
        return false;

    HANDLE hCsr = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pfnCsrGetProcessId());
    if (hCsr != NULL)
    {
        CloseHandle(hCsr);
        return true;
    }
    else
        return false;
}
bool closeHandle()
{
    __try
    {
        CloseHandle((HANDLE)0xDEADBEEF);
        return false;
    }
    __except (EXCEPTION_INVALID_HANDLE == GetExceptionCode()
        ? EXCEPTION_EXECUTE_HANDLER
        : EXCEPTION_CONTINUE_SEARCH)
    {
        return true;
    }
}
bool heap_protection()
{
    PROCESS_HEAP_ENTRY HeapEntry = { 0 };
    do
    {
        if (!HeapWalk(GetProcessHeap(), &HeapEntry))
            return false;
    } while (HeapEntry.wFlags != PROCESS_HEAP_ENTRY_BUSY);

    PVOID pOverlapped = (PBYTE)HeapEntry.lpData + HeapEntry.cbData;
    return ((DWORD)(*(PDWORD)pOverlapped) == 0xABABABAB);
}
bool createFile()
{
    CHAR szFileName[MAX_PATH];
    if (0 == GetModuleFileNameA(NULL, szFileName, sizeof(szFileName)))
        return false;

    return INVALID_HANDLE_VALUE == CreateFileA(szFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
}
void protc_debug()
{
#ifndef _WIN64
    // 32-Bit-Architektur
    PPEB pPeb = (PPEB)__readfsdword(0x30);
    *(PDWORD)((PBYTE)pPeb + 0x68) = 0;
#else
    // 64-Bit-Architektur
    PPEB pPeb = (PPEB)__readgsqword(0x60);
    *(PDWORD)((PBYTE)pPeb + 0xBC) = 0;
#endif // _WIN64

    // Entfernen der Debuggerkennung
    pPeb->BeingDebugged = 0;
}
void patch_dbgbreak()
{
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll)
        return;

    FARPROC pDbgBreakPoint = GetProcAddress(hNtdll, "DbgBreakPoint");
    if (!pDbgBreakPoint)
        return;

    DWORD dwOldProtect;
    if (!VirtualProtect(pDbgBreakPoint, 1, PAGE_EXECUTE_READWRITE, &dwOldProtect))
        return;

    *(PBYTE)pDbgBreakPoint = (BYTE)0xC3; // ret
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