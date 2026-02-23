#pragma once
#include <windows.h>
#include <thread>
#include "lazy.h"
#include "../keyauth/skStr.h"
#include "anti_debug.h"
#pragma comment(lib, "ntdll.lib")

// ==================== PROTECTION MODE CONFIG ====================
// Modus für Reaktion bei Detection:
// 0 = BSOD (BlueScreen)
// 1 = Exit (sauberes Beenden)
// 2 = Crash (nullptr access)
// 3 = Silent (nur loggen, nicht beenden - NUR FÜR TESTS!)
enum ProtectionMode
{
    PROTECTION_BSOD = 0,
    PROTECTION_EXIT = 1,
    PROTECTION_CRASH = 2,
    PROTECTION_SILENT = 3
};

inline ProtectionMode g_ProtectionMode = PROTECTION_EXIT; // Default: sauberes Exit

// Setzt den Protection-Modus
inline void SetProtectionMode(ProtectionMode mode)
{
    g_ProtectionMode = mode;
}

inline bool hide_thread(HANDLE thread)
{
    typedef NTSTATUS(NTAPI * pNtSetInformationThread)(HANDLE, UINT, PVOID, ULONG);
    pNtSetInformationThread NtSIT = (pNtSetInformationThread)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSetInformationThread");

    if (!NtSIT)
        return false;
    NTSTATUS status = (thread == NULL) ? NtSIT(GetCurrentThread(), 0x11, 0, 0) : NtSIT(thread, 0x11, 0, 0);

    return status == 0x00000000;
}

inline void OutputDbgString()
{
    SetLastError(0);
    OutputDebugStringA("Debugger detected!");
    if (GetLastError() != 0)
    {
        exit(1);
    }
}

inline int thread_context()
{
    int found = 0;
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    if (GetThreadContext(GetCurrentThread(), &ctx))
    {
        if (ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3 || ctx.Dr6 || ctx.Dr7)
        {
            found = 1;
        }
    }
    return found;
}

inline int remote_is_present()
{
    int debugger_present = 0;
    CheckRemoteDebuggerPresent(GetCurrentProcess(), &debugger_present);
    return debugger_present;
}

int is_debugger_present()
{
    return IsDebuggerPresent();
}

inline bool debug_perms_check()
{
    CONTEXT *ctx = (CONTEXT *)VirtualAlloc(NULL, sizeof(CONTEXT), MEM_COMMIT, PAGE_READWRITE);
    if (!ctx)
        return false;

    RtlSecureZeroMemory(ctx, sizeof(CONTEXT));
    ctx->ContextFlags = CONTEXT_DEBUG_REGISTERS;

    bool found = false;
    if (GetThreadContext(GetCurrentThread(), ctx))
    {
        if (ctx->Dr0 || ctx->Dr1 || ctx->Dr2 || ctx->Dr3)
            found = true;
    }

    VirtualFree(ctx, 0, MEM_RELEASE);
    return found;
}

extern "C" NTSTATUS NTAPI RtlAdjustPrivilege(ULONG, BOOLEAN, BOOLEAN, PBOOLEAN);
extern "C" NTSTATUS NTAPI NtRaiseHardError(LONG, ULONG, ULONG, PULONG_PTR, ULONG, PULONG);

void BlueScreen()
{
    BOOLEAN bl;
    ULONG Response;
    RtlAdjustPrivilege(19, TRUE, FALSE, &bl); // SeShutdownPrivilege aktivieren
    NtRaiseHardError(STATUS_ASSERTION_FAILURE, 0, 0, NULL, 6, &Response);
}

// Zentrale Funktion für Detection-Handling
inline void HandleDetection(const char *reason = nullptr)
{
    switch (g_ProtectionMode)
    {
    case PROTECTION_BSOD:
        BlueScreen();
        break;

    case PROTECTION_EXIT:
        ExitProcess(0);
        break;

    case PROTECTION_CRASH:
        *(volatile int *)(0) = 0xDEAD;
        break;

    case PROTECTION_SILENT:
        // Nur für Tests - logge Detection
        if (reason)
            OutputDebugStringA(reason);
        break;
    }
}

// Anti-Dump Setup - einmalig beim Start aufrufen
// ACHTUNG: Einige Funktionen sind sehr aggressiv und können Crashes verursachen!
void InitAntiDump()
{
    // AGGRESSIVE ANTI-DUMP/PE-FUNKTIONEN - EINZELN TESTEN!
    // ErasePEHeader();
    // UnlinkFromPEB();
    // HookNtReadVirtualMemory();
    // ===== STABILER BUILD: Nur getestete, stabile Funktionen aktiv =====
    EraseIAT();
    EraseEAT();
    CorruptSectionHeaders();
    EraseRelocations();
    // Alle anderen aggressiven Funktionen bleiben deaktiviert!
}

// Anti-Debug Setup - einmalig beim Start aufrufen
void InitAntiDebug()
{
    // ALLES DEAKTIVIERT FÜR DEBUGGING - einzeln aktivieren zum Testen!
    patch_dbgbreak();
    PatchDbgUiRemoteBreakin();
    NtSetInformationThreadHide();
    InitIntegrityCheck(); // KANN PROBLEME VERURSACHEN wenn PE modifiziert
    protc_debug();
    AntiAttach();
}

// VM/Sandbox Check - einmalig beim Start
bool IsVMOrSandbox()
{
    return DetectVMViaCPUID() ||
           DetectVirtualBox() ||
           DetectHyperV() ||
           DetectSandbox() ||
           DetectWine() ||
           CheckDiskSize() ||
           CheckProcessorCount() ||
           CheckRAMSize();
}

void Protection_Loop()
{
    while (true)
    {
        // ==================== INTEGRITY CHECK ====================
        if (VerifyIntegrity())
            HandleDetection("IntegrityFail");

        // ==================== MISC CHECKS ====================
        if (openProcess())
            HandleDetection("OpenProcess");
        if (closeHandle())
            HandleDetection("CloseHandle");

        // ==================== DEBUGGER DETECTION (SAFE) ====================
        if (IsDebuggerPresentCustom())
            HandleDetection("IsDebuggerPresent");
        if (CheckRemoteDebugger())
            HandleDetection("RemoteDebugger");
        if (CheckNtGlobalFlag())
            HandleDetection("NtGlobalFlag");
        if (CheckHeapFlags())
            HandleDetection("HeapFlags");
        if (CheckProcessDebugPort())
            HandleDetection("DebugPort");
        if (CheckProcessDebugObjectHandle())
            HandleDetection("DebugObjectHandle");
        if (CheckProcessDebugFlags())
            HandleDetection("DebugFlags");
        if (CheckKernelDebugger())
            HandleDetection("KernelDebugger");
        if (CheckDebuggerWindows())
            HandleDetection("DebuggerWindow");
        if (CheckDebuggerProcesses())
            HandleDetection("DebuggerProcess");
        if (CheckAnalysisDLLs())
            HandleDetection("AnalysisDLL");

        // ==================== TIMING CHECKS ====================
        if (DetectRDTSC())
            HandleDetection("RDTSC");
        if (DetectQueryPerformanceCounter())
            HandleDetection("QPCounter");
        if (AdvancedTimingCheck())
            HandleDetection("TimingCheck");

        // ==================== BREAKPOINT DETECTION ====================
        if (DetectHardwareBreakpoints())
            HandleDetection("HWBreakpoint");
        if (heap_protection())
            HandleDetection("HeapProtection");
        if (mem_break())
            HandleDetection("MemBreak");

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
