#include "anti_debug.h"
#include <intrin.h>
#include <windows.h>
#include <winternl.h>
#include <imagehlp.h>
#include <iostream>
#include <tlhelp32.h>

#pragma comment(lib, "ntdll.lib")

// Custom PEB_LDR_DATA with all module lists (winternl.h hides some)
typedef struct _MY_PEB_LDR_DATA
{
    ULONG Length;
    BOOLEAN Initialized;
    HANDLE SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
} MY_PEB_LDR_DATA, *PMY_PEB_LDR_DATA;

typedef NTSTATUS(WINAPI *pNtQueryInformationProcess)(
    HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

bool IsDebuggerPresentCustom()
{
    return IsDebuggerPresent() != 0;
}

bool CheckRemoteDebugger()
{
    BOOL isDebugger = FALSE;
    CheckRemoteDebuggerPresent(GetCurrentProcess(), &isDebugger);
    return isDebugger;
}

bool NtQueryInfoProcess()
{
    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (!hNtDll)
        return false;

    pNtQueryInformationProcess NtQueryInformationProcess =
        (pNtQueryInformationProcess)GetProcAddress(hNtDll, "NtQueryInformationProcess");

    if (!NtQueryInformationProcess)
        return false;

    PROCESS_BASIC_INFORMATION pbi;
    ULONG returnLength;
    NTSTATUS status = NtQueryInformationProcess(GetCurrentProcess(), (PROCESSINFOCLASS)ProcessBasicInformation, &pbi, sizeof(pbi), &returnLength);

    if (status != 0)
        return false;

    BOOL isDebugged = FALSE;
    SIZE_T bytesRead;
    if (ReadProcessMemory(GetCurrentProcess(), &(pbi.PebBaseAddress->BeingDebugged), &isDebugged, sizeof(BOOL), &bytesRead))
    {
        return isDebugged;
    }

    return false;
}

bool NtSetInformationThreadHide()
{
    typedef NTSTATUS(NTAPI * pNtSetInformationThread)(HANDLE, THREAD_INFORMATION_CLASS, PVOID, ULONG);
    pNtSetInformationThread NtSIT = (pNtSetInformationThread)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSetInformationThread");
    if (NtSIT)
    {
        return NtSIT(GetCurrentThread(), (THREAD_INFORMATION_CLASS)0x11, NULL, 0) == 0;
    }
    return false;
}

bool DetectRDTSC()
{
    unsigned __int64 t1, t2;
    t1 = __rdtsc();
    Sleep(10);
    t2 = __rdtsc();
    return (t2 - t1) < 100000;
}

bool DetectQueryPerformanceCounter()
{
    LARGE_INTEGER t1, t2, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t1);
    Sleep(10);
    QueryPerformanceCounter(&t2);
    return (t2.QuadPart - t1.QuadPart) < (freq.QuadPart / 100);
}

bool DetectGetTickCount()
{
    DWORD t1 = GetTickCount();
    Sleep(10);
    DWORD t2 = GetTickCount();
    return (t2 - t1) < 10;
}

bool DetectHardwareBreakpoints()
{
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext(GetCurrentThread(), &ctx))
    {
        return (ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3);
    }
    return false;
}

bool DetectINT3()
{
    __try
    {
        __debugbreak();
        return false;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return true;
    }
}

bool mem_break()
{
    DWORD dwOldProtect = 0;
    SYSTEM_INFO SysInfo = {0};

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
        auto func = reinterpret_cast<void (*)()>(pPage);
        func();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        detected = false;
    }

    VirtualFree(pPage, NULL, MEM_RELEASE);
    return detected;
}

bool DetectINT2D()
{
    __try
    {
        RaiseException(0x2D, 0, 0, NULL);
        return false;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return true;
    }
}
typedef DWORD(WINAPI *TCsrGetProcessId)(VOID);

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
        CloseHandle((HANDLE)(ULONG_PTR)0xDEADBEEF);
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
    PROCESS_HEAP_ENTRY HeapEntry = {0};
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
bool DetectSingleStepException()
{
    __try
    {
        CONTEXT ctx = {0};
        ctx.ContextFlags = CONTEXT_CONTROL;
        GetThreadContext(GetCurrentThread(), &ctx);

        ctx.EFlags |= 0x100;
        SetThreadContext(GetCurrentThread(), &ctx);

        return false;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return true;
    }
}
bool DetectStackPointerManipulation()
{
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_CONTROL;

    if (!GetThreadContext(GetCurrentThread(), &ctx))
    {
        return false;
    }

#ifdef _WIN64
    uintptr_t stackPointer = ctx.Rsp;
    if (stackPointer < 0x100000000 || stackPointer > 0x7FFFFFFFFFFF)
    {
        return true;
    }
#else
    uintptr_t stackPointer = ctx.Esp;
    if (stackPointer < 0x10000 || stackPointer > 0x7FFFFFFF)
    {
        return true;
    }
#endif

    return false;
}
void ProtectMemory(LPVOID lpAddress, SIZE_T dwSize)
{
    DWORD oldProtect;
    VirtualProtect(lpAddress, dwSize, PAGE_NOACCESS, &oldProtect);
}

void UnprotectMemory(LPVOID lpAddress, SIZE_T dwSize)
{
    DWORD oldProtect;
    VirtualProtect(lpAddress, dwSize, PAGE_READWRITE, &oldProtect);
}
typedef NTSTATUS(WINAPI *pNtQuerySystemInformation)(
    SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

bool DetectMemoryDumping()
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    if (sysInfo.dwPageSize < 0x1000)
    {
        return true;
    }

    return false;
}
LONG WINAPI MyUnhandledExceptionFilter(EXCEPTION_POINTERS *pExceptionInfo)
{

    return EXCEPTION_EXECUTE_HANDLER;
}

void SetupAntiDump()
{
    SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
}

// PE Header im Speicher löschen - verhindert einfaches Dumpen
void ErasePEHeader()
{
    DWORD oldProtect;
    char *baseAddr = (char *)GetModuleHandle(NULL);
    if (VirtualProtect(baseAddr, 4096, PAGE_READWRITE, &oldProtect))
    {
        SecureZeroMemory(baseAddr, 4096);
        VirtualProtect(baseAddr, 4096, oldProtect, &oldProtect);
    }
}

// SizeOfImage in PEB verfälschen - Dump-Tools bekommen falsche Größe
void CorruptSizeOfImage()
{
#ifdef _WIN64
    PPEB pPeb = (PPEB)__readgsqword(0x60);
#else
    PPEB pPeb = (PPEB)__readfsdword(0x30);
#endif

    PLIST_ENTRY head = &pPeb->Ldr->InMemoryOrderModuleList;
    PLIST_ENTRY current = head->Flink;

    // LDR_DATA_TABLE_ENTRY für unser Modul
    PLDR_DATA_TABLE_ENTRY entry = CONTAINING_RECORD(current, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

    // SizeOfImage verfälschen
    entry->Reserved3[1] = (PVOID)((ULONG_PTR)entry->Reserved3[1] ^ 0xDEADBEEF);
}

// Section Headers korrumpieren
void CorruptSectionHeaders()
{
    HMODULE hModule = GetModuleHandle(NULL);
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return;

    PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((BYTE *)dosHeader + dosHeader->e_lfanew);

    if (ntHeader->Signature != IMAGE_NT_SIGNATURE)
        return;

    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeader);
    DWORD sectionSize = ntHeader->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);

    DWORD oldProtect;
    if (VirtualProtect(section, sectionSize, PAGE_READWRITE, &oldProtect))
    {
        // Section Namen und Charakteristiken randomisieren
        for (int i = 0; i < ntHeader->FileHeader.NumberOfSections; i++)
        {
            SecureZeroMemory(section[i].Name, 8);
            section[i].VirtualAddress ^= 0x1337;
            section[i].SizeOfRawData ^= 0x1337;
        }
        VirtualProtect(section, sectionSize, oldProtect, &oldProtect);
    }
}

// NtReadVirtualMemory hooken - verhindert externes Auslesen des Speichers
void HookNtReadVirtualMemory()
{
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll)
        return;

    LPVOID addr = (LPVOID)GetProcAddress(hNtdll, "NtReadVirtualMemory");
    if (!addr)
        return;

    DWORD oldProtect;
    // ret 0x28 für x64, ret 0x14 für x86
#ifdef _WIN64
    BYTE patch[] = {0xC2, 0x28, 0x00};
#else
    BYTE patch[] = {0xC2, 0x14, 0x00};
#endif

    if (VirtualProtect(addr, sizeof(patch), PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        memcpy(addr, patch, sizeof(patch));
        VirtualProtect(addr, sizeof(patch), oldProtect, &oldProtect);
    }
}

// Modul aus PEB unlinking - versteckt das Modul in der Modulliste
void UnlinkFromPEB()
{
#ifdef _WIN64
    PPEB pPeb = (PPEB)__readgsqword(0x60);
#else
    PPEB pPeb = (PPEB)__readfsdword(0x30);
#endif

    // Cast to our custom structure that has all lists
    PMY_PEB_LDR_DATA pLdr = (PMY_PEB_LDR_DATA)pPeb->Ldr;

    // Aus InMemoryOrderModuleList entfernen
    PLIST_ENTRY head = &pLdr->InMemoryOrderModuleList;
    PLIST_ENTRY current = head->Flink;
    current->Blink->Flink = current->Flink;
    current->Flink->Blink = current->Blink;

    // Auch aus InLoadOrderModuleList entfernen
    head = &pLdr->InLoadOrderModuleList;
    current = head->Flink;
    current->Blink->Flink = current->Flink;
    current->Flink->Blink = current->Blink;

    // Auch aus InInitializationOrderModuleList entfernen
    head = &pLdr->InInitializationOrderModuleList;
    current = head->Flink;
    current->Blink->Flink = current->Flink;
    current->Flink->Blink = current->Blink;
}

// Guard Pages auf PE Header setzen
void SetGuardPagesOnHeader()
{
    DWORD oldProtect;
    LPVOID baseAddr = GetModuleHandle(NULL);

    // PAGE_GUARD löst Exception beim ersten Zugriff aus
    VirtualProtect(baseAddr, 0x1000, PAGE_READONLY | PAGE_GUARD, &oldProtect);
}

// ==================== EXTENDED ANTI-DEBUG ====================

// Check NtGlobalFlag in PEB - gesetzt wenn debugger attached
bool CheckNtGlobalFlag()
{
#ifdef _WIN64
    PPEB pPeb = (PPEB)__readgsqword(0x60);
    DWORD NtGlobalFlag = *(PDWORD)((PBYTE)pPeb + 0xBC);
#else
    PPEB pPeb = (PPEB)__readfsdword(0x30);
    DWORD NtGlobalFlag = *(PDWORD)((PBYTE)pPeb + 0x68);
#endif
    // FLG_HEAP_ENABLE_TAIL_CHECK | FLG_HEAP_ENABLE_FREE_CHECK | FLG_HEAP_VALIDATE_PARAMETERS
    return (NtGlobalFlag & 0x70) != 0;
}

// Check Heap Flags - Debugger setzt spezielle Flags
bool CheckHeapFlags()
{
#ifdef _WIN64
    PPEB pPeb = (PPEB)__readgsqword(0x60);
    PVOID pHeap = (PVOID)(*(PDWORD_PTR)((PBYTE)pPeb + 0x30));
    DWORD dwHeapFlagsOffset = 0x70;
    DWORD dwHeapForceFlagsOffset = 0x74;
#else
    PPEB pPeb = (PPEB)__readfsdword(0x30);
    PVOID pHeap = (PVOID)(*(PDWORD_PTR)((PBYTE)pPeb + 0x18));
    DWORD dwHeapFlagsOffset = 0x40;
    DWORD dwHeapForceFlagsOffset = 0x44;
#endif
    DWORD HeapFlags = *(PDWORD)((PBYTE)pHeap + dwHeapFlagsOffset);
    DWORD HeapForceFlags = *(PDWORD)((PBYTE)pHeap + dwHeapForceFlagsOffset);

    return (HeapFlags & ~HEAP_GROWABLE) != 0 || HeapForceFlags != 0;
}

// ProcessDebugPort check via NtQueryInformationProcess
bool CheckProcessDebugPort()
{
    typedef NTSTATUS(NTAPI * pNtQueryInformationProcess)(HANDLE, UINT, PVOID, ULONG, PULONG);
    pNtQueryInformationProcess NtQIP = (pNtQueryInformationProcess)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");

    if (!NtQIP)
        return false;

    DWORD_PTR debugPort = 0;
    NTSTATUS status = NtQIP(GetCurrentProcess(), 7, &debugPort, sizeof(debugPort), NULL); // ProcessDebugPort = 7

    return (status == 0 && debugPort != 0);
}

// ProcessDebugObjectHandle check
bool CheckProcessDebugObjectHandle()
{
    typedef NTSTATUS(NTAPI * pNtQueryInformationProcess)(HANDLE, UINT, PVOID, ULONG, PULONG);
    pNtQueryInformationProcess NtQIP = (pNtQueryInformationProcess)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");

    if (!NtQIP)
        return false;

    HANDLE debugObject = NULL;
    NTSTATUS status = NtQIP(GetCurrentProcess(), 0x1E, &debugObject, sizeof(debugObject), NULL); // ProcessDebugObjectHandle = 0x1E

    return (status == 0 && debugObject != NULL);
}

// ProcessDebugFlags check - returns 0 if being debugged
bool CheckProcessDebugFlags()
{
    typedef NTSTATUS(NTAPI * pNtQueryInformationProcess)(HANDLE, UINT, PVOID, ULONG, PULONG);
    pNtQueryInformationProcess NtQIP = (pNtQueryInformationProcess)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");

    if (!NtQIP)
        return false;

    DWORD debugFlags = 0;
    NTSTATUS status = NtQIP(GetCurrentProcess(), 0x1F, &debugFlags, sizeof(debugFlags), NULL); // ProcessDebugFlags = 0x1F

    return (status == 0 && debugFlags == 0);
}

// Check for kernel debugger via NtQuerySystemInformation
bool CheckKernelDebugger()
{
    typedef NTSTATUS(NTAPI * pNtQuerySystemInformation)(UINT, PVOID, ULONG, PULONG);
    pNtQuerySystemInformation NtQSI = (pNtQuerySystemInformation)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "NtQuerySystemInformation");

    if (!NtQSI)
        return false;

    struct SYSTEM_KERNEL_DEBUGGER_INFORMATION
    {
        BOOLEAN KernelDebuggerEnabled;
        BOOLEAN KernelDebuggerNotPresent;
    } info = {0};

    NTSTATUS status = NtQSI(0x23, &info, sizeof(info), NULL); // SystemKernelDebuggerInformation = 0x23

    return (status == 0 && info.KernelDebuggerEnabled && !info.KernelDebuggerNotPresent);
}

// Check parent process - should be explorer.exe
bool CheckParentProcess()
{
    typedef NTSTATUS(NTAPI * pNtQueryInformationProcess)(HANDLE, UINT, PVOID, ULONG, PULONG);
    pNtQueryInformationProcess NtQIP = (pNtQueryInformationProcess)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess");

    if (!NtQIP)
        return false;

    PROCESS_BASIC_INFORMATION pbi = {0};
    NTSTATUS status = NtQIP(GetCurrentProcess(), 0, &pbi, sizeof(pbi), NULL);

    if (status != 0)
        return false;

    // Reserved3 contains InheritedFromUniqueProcessId - cast properly for x64
    DWORD parentPID = (DWORD)(ULONG_PTR)pbi.Reserved3;
    HANDLE hParent = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, parentPID);
    if (!hParent)
        return false;

    char parentName[MAX_PATH] = {0};
    DWORD size = MAX_PATH;
    QueryFullProcessImageNameA(hParent, 0, parentName, &size);
    CloseHandle(hParent);

    // Check if parent is NOT explorer or cmd
    _strlwr_s(parentName, MAX_PATH);
    return (strstr(parentName, "explorer.exe") == NULL &&
            strstr(parentName, "cmd.exe") == NULL &&
            strstr(parentName, "powershell.exe") == NULL);
}

// Check for debugger windows
bool CheckDebuggerWindows()
{
    const char *debuggerWindows[] = {
        "OLLYDBG", "WinDbgFrameClass", "ID", "Immunity Debugger",
        "x64dbg", "x32dbg", "[CPU", "IDA", "disassembly",
        "Cheat Engine", "Process Hacker", "Process Monitor"};

    for (const char *wndName : debuggerWindows)
    {
        if (FindWindowA(NULL, wndName) || FindWindowA(wndName, NULL))
            return true;
    }
    return false;
}

// Check for debugger/analysis processes
bool CheckDebuggerProcesses()
{
    const char *debuggerProcesses[] = {
        "ollydbg.exe", "x64dbg.exe", "x32dbg.exe", "ida.exe", "ida64.exe",
        "idaq.exe", "idaq64.exe", "windbg.exe", "immunity debugger.exe",
        "cheatengine-x86_64.exe", "cheatengine.exe", "processhacker.exe",
        "procmon.exe", "procmon64.exe", "procexp.exe", "procexp64.exe",
        "dnspy.exe", "de4dot.exe", "fiddler.exe", "wireshark.exe",
        "httpdebugger.exe", "regmon.exe", "filemon.exe",
        "scylla.exe", "scylla_x64.exe", "scylla_x86.exe",
        "protection_id.exe", "reshacker.exe", "importrec.exe",
        "pe-sieve64.exe", "pe-sieve32.exe", "hollows_hunter.exe",
        "dumpcap.exe", "HookExplorer.exe", "SysInspector.exe",
        "apimonitor-x86.exe", "apimonitor-x64.exe"};

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE)
        return false;

    PROCESSENTRY32 pe = {sizeof(pe)};
    bool found = false;

    if (Process32First(hSnap, &pe))
    {
        do
        {
            char procName[MAX_PATH];
            strcpy_s(procName, pe.szExeFile);
            _strlwr_s(procName);

            for (const char *dbgProc : debuggerProcesses)
            {
                if (strcmp(procName, dbgProc) == 0)
                {
                    found = true;
                    break;
                }
            }
        } while (!found && Process32Next(hSnap, &pe));
    }

    CloseHandle(hSnap);
    return found;
}

// Check for analysis tool DLLs loaded
bool CheckAnalysisDLLs()
{
    const char *analysisDLLs[] = {
        "sbiedll.dll",   // Sandboxie
        "dbghelp.dll",   // Debugger helper
        "api_log.dll",   // API Monitor
        "dir_watch.dll", // Directory watcher
        "pstorec.dll",   // Password stealer detector
        "vmcheck.dll",   // VM check
        "wpespy.dll",    // WPE Pro
        "SbieDll.dll",   // Sandboxie
        "SxIn.dll",      // SX packet sniffer
        "Sf2.dll",       // Syser
        "snxhk.dll",     // Avast sandbox
        "cmdvrt32.dll",  // Comodo sandbox
        "cuckoomon.dll"  // Cuckoo sandbox
    };

    for (const char *dll : analysisDLLs)
    {
        if (GetModuleHandleA(dll) != NULL)
            return true;
    }
    return false;
}

// Anti-attach: macht es schwieriger den Prozess zu attachen
void AntiAttach()
{
    typedef NTSTATUS(NTAPI * pNtSetInformationProcess)(HANDLE, UINT, PVOID, ULONG);
    pNtSetInformationProcess NtSIP = (pNtSetInformationProcess)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "NtSetInformationProcess");

    if (!NtSIP)
        return;

    DWORD value = 1;
    // ProcessBreakOnTermination = 0x1D, macht Prozess kritisch
    NtSIP(GetCurrentProcess(), 0x1D, &value, sizeof(value));
}

// Patch DbgUiRemoteBreakin to prevent attaching
void PatchDbgUiRemoteBreakin()
{
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll)
        return;

    FARPROC pDbgUiRemoteBreakin = GetProcAddress(hNtdll, "DbgUiRemoteBreakin");
    if (!pDbgUiRemoteBreakin)
        return;

    DWORD oldProtect;
    if (!VirtualProtect(pDbgUiRemoteBreakin, 16, PAGE_EXECUTE_READWRITE, &oldProtect))
        return;

    // Patch to jump to ExitProcess
    FARPROC pExitProcess = GetProcAddress(GetModuleHandleA("kernel32.dll"), "ExitProcess");

#ifdef _WIN64
    // mov rax, ExitProcess; xor ecx, ecx; jmp rax
    BYTE patch[] = {
        0x48, 0xB8,                                     // mov rax, imm64
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // placeholder for address
        0x33, 0xC9,                                     // xor ecx, ecx (exit code 0)
        0xFF, 0xE0                                      // jmp rax
    };
    *(FARPROC *)&patch[2] = pExitProcess;
    memcpy(pDbgUiRemoteBreakin, patch, sizeof(patch));
#else
    // xor eax, eax; push eax; call ExitProcess (relative)
    BYTE patch[] = {
        0x33, 0xC0,                  // xor eax, eax
        0x50,                        // push eax
        0xE8, 0x00, 0x00, 0x00, 0x00 // call rel32
    };
    DWORD relAddr = (DWORD)(ULONG_PTR)pExitProcess - ((DWORD)(ULONG_PTR)pDbgUiRemoteBreakin + sizeof(patch));
    *(DWORD *)&patch[4] = relAddr;
    memcpy(pDbgUiRemoteBreakin, patch, sizeof(patch));
#endif

    VirtualProtect(pDbgUiRemoteBreakin, 16, oldProtect, &oldProtect);
}

// ==================== ANTI-VM / ANTI-SANDBOX ====================

// Check for VMware
bool DetectVMware()
{
#ifndef _WIN64
    // Only works in 32-bit - inline asm not supported in x64 MSVC
    __try
    {
        unsigned int vmware_magic = 0x564D5868; // "VMXh"
        unsigned int vmware_port = 0x5658;      // "VX"
        unsigned int result = 0;

        __asm
        {
            push edx
            push ecx
            push ebx
            
            mov eax, vmware_magic
            mov ebx, 0
            mov ecx, 10
            mov edx, vmware_port
            
            in eax, dx
            
            cmp ebx, vmware_magic
            jne not_vmware
            mov result, 1
            
not_vmware:
            pop ebx
            pop ecx
            pop edx
        }

        return result != 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
#else
    // x64: Check via registry and other methods
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\VMware, Inc.\\VMware Tools", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return true;
    }

    // Check for VMware SCSI controller
    if (GetFileAttributesA("\\\\.\\HGFS") != INVALID_FILE_ATTRIBUTES)
        return true;
    if (GetFileAttributesA("\\\\.\\vmci") != INVALID_FILE_ATTRIBUTES)
        return true;

    // Check for VMware DLLs
    if (GetModuleHandleA("vmGuestLib.dll") || GetModuleHandleA("vmbusres.dll"))
        return true;

    return false;
#endif
}

// Check for VirtualBox
bool DetectVirtualBox()
{
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Oracle\\VirtualBox Guest Additions", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return true;
    }

    // Check for VBox devices
    if (GetFileAttributesA("\\\\.\\VBoxMiniRdrDN") != INVALID_FILE_ATTRIBUTES)
        return true;
    if (GetFileAttributesA("\\\\.\\VBoxGuest") != INVALID_FILE_ATTRIBUTES)
        return true;

    // Check for VBox DLLs
    if (GetModuleHandleA("VBoxHook.dll") || GetModuleHandleA("VBoxMRXNP.dll"))
        return true;

    return false;
}

// Check for Hyper-V
bool DetectHyperV()
{
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Virtual Machine\\Guest\\Parameters", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

// Generic VM detection via CPUID
bool DetectVMViaCPUID()
{
    int cpuInfo[4] = {0};
    __cpuid(cpuInfo, 1);

    // Hypervisor bit (bit 31 of ECX)
    return (cpuInfo[2] >> 31) & 1;
}

// Check for sandbox indicators
bool DetectSandbox()
{
    // Check for common sandbox usernames
    char username[256] = {0};
    DWORD size = sizeof(username);
    GetUserNameA(username, &size);
    _strlwr_s(username);

    const char *sandboxUsers[] = {
        "sandbox", "virus", "malware", "sample", "test", "john doe",
        "user", "currentuser", "admin", "administrator", "vmware",
        "virtual", "sandbox", "walletfocus", "johnDoe"};

    for (const char *user : sandboxUsers)
    {
        if (strstr(username, user))
            return true;
    }

    // Check computer name
    char compName[256] = {0};
    size = sizeof(compName);
    GetComputerNameA(compName, &size);
    _strlwr_s(compName);

    const char *sandboxComputers[] = {
        "sandbox", "virus", "malware", "sample", "test", "virtual",
        "vmware", "vbox", "analysis", "cuckoo"};

    for (const char *comp : sandboxComputers)
    {
        if (strstr(compName, comp))
            return true;
    }

    return false;
}

// Check screen resolution (VMs often have small/odd resolutions)
bool CheckScreenResolution()
{
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    // Common VM/sandbox resolutions
    if (width <= 800 && height <= 600)
        return true;
    if (width == 1024 && height == 768)
        return true; // Very common VM default

    return false;
}

// Check for human interaction (mouse movement)
bool CheckMouseMovement()
{
    POINT p1, p2;
    GetCursorPos(&p1);
    Sleep(500);
    GetCursorPos(&p2);

    // No mouse movement = possibly automated
    return (p1.x == p2.x && p1.y == p2.y);
}

// Check disk size (VMs often have small disks)
bool CheckDiskSize()
{
    ULARGE_INTEGER totalBytes;
    if (GetDiskFreeSpaceExA("C:\\", NULL, &totalBytes, NULL))
    {
        // Less than 60GB is suspicious
        if (totalBytes.QuadPart < 60ULL * 1024 * 1024 * 1024)
            return true;
    }
    return false;
}

// Check number of processors
bool CheckProcessorCount()
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    // VMs often have 1-2 cores
    return sysInfo.dwNumberOfProcessors < 2;
}

// Check RAM size
bool CheckRAMSize()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    GlobalMemoryStatusEx(&memInfo);

    // Less than 2GB RAM is suspicious
    return memInfo.ullTotalPhys < 2ULL * 1024 * 1024 * 1024;
}

// Check for wine
bool DetectWine()
{
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll)
        return false;

    return GetProcAddress(hNtdll, "wine_get_version") != NULL;
}

// ==================== ADDITIONAL ANTI-DUMP ====================

// Erase Import Address Table
void EraseIAT()
{
    HMODULE hModule = GetModuleHandle(NULL);
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return;

    PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((BYTE *)dosHeader + dosHeader->e_lfanew);

    if (ntHeader->Signature != IMAGE_NT_SIGNATURE)
        return;

    DWORD importDirRVA = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (importDirRVA == 0)
        return;

    PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)((BYTE *)hModule + importDirRVA);

    DWORD oldProtect;
    VirtualProtect(importDesc, 0x1000, PAGE_READWRITE, &oldProtect);

    // Zero out import descriptors
    while (importDesc->Name)
    {
        importDesc->OriginalFirstThunk = 0;
        importDesc->TimeDateStamp = 0xDEADBEEF;
        importDesc->ForwarderChain = 0xDEADBEEF;
        importDesc->Name = 0;
        importDesc->FirstThunk = 0;
        importDesc++;
    }

    VirtualProtect(importDesc, 0x1000, oldProtect, &oldProtect);
}

// Erase Export Address Table (falls vorhanden)
void EraseEAT()
{
    HMODULE hModule = GetModuleHandle(NULL);
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return;

    PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((BYTE *)dosHeader + dosHeader->e_lfanew);

    DWORD exportDirRVA = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    DWORD exportDirSize = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

    if (exportDirRVA == 0 || exportDirSize == 0)
        return;

    DWORD oldProtect;
    VirtualProtect((BYTE *)hModule + exportDirRVA, exportDirSize, PAGE_READWRITE, &oldProtect);
    SecureZeroMemory((BYTE *)hModule + exportDirRVA, exportDirSize);
    VirtualProtect((BYTE *)hModule + exportDirRVA, exportDirSize, oldProtect, &oldProtect);
}

// Destroy relocation table
void EraseRelocations()
{
    HMODULE hModule = GetModuleHandle(NULL);
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return;

    PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((BYTE *)dosHeader + dosHeader->e_lfanew);

    DWORD relocDirRVA = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
    DWORD relocDirSize = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;

    if (relocDirRVA == 0 || relocDirSize == 0)
        return;

    DWORD oldProtect;
    VirtualProtect((BYTE *)hModule + relocDirRVA, relocDirSize, PAGE_READWRITE, &oldProtect);
    SecureZeroMemory((BYTE *)hModule + relocDirRVA, relocDirSize);

    // Also clear the data directory entry
    VirtualProtect(&ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC],
                   sizeof(IMAGE_DATA_DIRECTORY), PAGE_READWRITE, &oldProtect);
    ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = 0;
    ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = 0;
}

// Erase debug directory
void EraseDebugDirectory()
{
    HMODULE hModule = GetModuleHandle(NULL);
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return;

    PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((BYTE *)dosHeader + dosHeader->e_lfanew);

    DWORD debugDirRVA = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
    DWORD debugDirSize = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;

    if (debugDirRVA == 0)
        return;

    DWORD oldProtect;
    if (debugDirSize > 0)
    {
        VirtualProtect((BYTE *)hModule + debugDirRVA, debugDirSize, PAGE_READWRITE, &oldProtect);
        SecureZeroMemory((BYTE *)hModule + debugDirRVA, debugDirSize);
    }

    // Clear data directory entry
    VirtualProtect(&ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG],
                   sizeof(IMAGE_DATA_DIRECTORY), PAGE_READWRITE, &oldProtect);
    ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress = 0;
    ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size = 0;
}

// Clear TLS callbacks
void EraseTLSCallbacks()
{
    HMODULE hModule = GetModuleHandle(NULL);
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return;

    PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((BYTE *)dosHeader + dosHeader->e_lfanew);

    DWORD tlsDirRVA = ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;

    if (tlsDirRVA == 0)
        return;

    DWORD oldProtect;
    VirtualProtect(&ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS],
                   sizeof(IMAGE_DATA_DIRECTORY), PAGE_READWRITE, &oldProtect);
    ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = 0;
    ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = 0;
}

// Wipe all data directories
void WipeAllDataDirectories()
{
    HMODULE hModule = GetModuleHandle(NULL);
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return;

    PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((BYTE *)dosHeader + dosHeader->e_lfanew);

    DWORD oldProtect;
    VirtualProtect(&ntHeader->OptionalHeader.DataDirectory,
                   sizeof(ntHeader->OptionalHeader.DataDirectory), PAGE_READWRITE, &oldProtect);

    // Keep import dir alive for runtime, wipe rest
    for (int i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++)
    {
        if (i == IMAGE_DIRECTORY_ENTRY_IMPORT)
            continue; // Keep imports
        if (i == IMAGE_DIRECTORY_ENTRY_IAT)
            continue; // Keep IAT

        ntHeader->OptionalHeader.DataDirectory[i].VirtualAddress = 0;
        ntHeader->OptionalHeader.DataDirectory[i].Size = 0;
    }
}

// ==================== TIMING & INTEGRITY CHECKS ====================

// Self-checksum verification
DWORD g_originalChecksum = 0;

DWORD CalculateChecksum(PVOID base, SIZE_T size)
{
    DWORD checksum = 0;
    PBYTE ptr = (PBYTE)base;

    for (SIZE_T i = 0; i < size; i++)
    {
        checksum = _rotl(checksum, 3);
        checksum ^= ptr[i];
    }

    return checksum;
}

void InitIntegrityCheck()
{
    HMODULE hModule = GetModuleHandle(NULL);
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((BYTE *)dosHeader + dosHeader->e_lfanew);

    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeader);

    // Find .text section
    for (int i = 0; i < ntHeader->FileHeader.NumberOfSections; i++)
    {
        if (memcmp(section[i].Name, ".text", 5) == 0)
        {
            PVOID sectionBase = (BYTE *)hModule + section[i].VirtualAddress;
            g_originalChecksum = CalculateChecksum(sectionBase, section[i].Misc.VirtualSize);
            break;
        }
    }
}

bool VerifyIntegrity()
{
    if (g_originalChecksum == 0)
        return false;

    HMODULE hModule = GetModuleHandle(NULL);
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((BYTE *)dosHeader + dosHeader->e_lfanew);

    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(ntHeader);

    for (int i = 0; i < ntHeader->FileHeader.NumberOfSections; i++)
    {
        if (memcmp(section[i].Name, ".text", 5) == 0)
        {
            PVOID sectionBase = (BYTE *)hModule + section[i].VirtualAddress;
            DWORD currentChecksum = CalculateChecksum(sectionBase, section[i].Misc.VirtualSize);
            return currentChecksum != g_originalChecksum;
        }
    }

    return false;
}

// Advanced timing check with multiple methods
bool AdvancedTimingCheck()
{
    // Method 1: RDTSC
    unsigned __int64 t1 = __rdtsc();

    // Do some work
    volatile int x = 0;
    for (int i = 0; i < 1000; i++)
        x += i;

    unsigned __int64 t2 = __rdtsc();

    // If took too long, likely debugging
    if ((t2 - t1) > 0x100000)
        return true;

    // Method 2: QueryPerformanceCounter
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    x = 0;
    for (int i = 0; i < 1000; i++)
        x += i;

    QueryPerformanceCounter(&end);

    // More than 1ms for simple loop = suspicious
    if ((end.QuadPart - start.QuadPart) > (freq.QuadPart / 1000))
        return true;

    return false;
}

// ==================== OBFUSCATION HELPERS ====================

// Anti-disassembly: fügt ungültigen Code ein
#ifndef _WIN64
__declspec(naked) void AntiDisasm()
{
    __asm
    {
        jmp real_code
        __emit 0xE8 // Fake CALL opcode, verwirrt Disassembler
        
real_code:
        ret
    }
}
#else
// x64: inline asm not supported, use alternative
void AntiDisasm()
{
    // No-op in x64, inline assembly not supported
    volatile int x = 0;
    x++;
}
#endif

// Opaque predicates - immer wahr, aber schwer statisch zu analysieren
bool OpaquePredicateTrue()
{
    volatile int x = 0;
    volatile int y = 0;

    for (int i = 0; i < 10; i++)
    {
        x += i;
        y += i * i;
    }

    // (sum 0-9)² = 45² = 2025
    // sum of squares 0-9 = 285
    // 45² > 285 immer wahr
    return (x * x) > y;
}