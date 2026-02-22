#pragma once
#include <windows.h>
#include <thread>
#include "lazy.h"
#include "../keyauth/skStr.h"
#include "anti_debug.h"
#pragma comment(lib, "ntdll.lib")

inline bool hide_thread(HANDLE thread)
{
    typedef NTSTATUS(NTAPI* pNtSetInformationThread)(HANDLE, UINT, PVOID, ULONG);
    pNtSetInformationThread NtSIT = (pNtSetInformationThread)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSetInformationThread");

    if (!NtSIT) return false;
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
    CONTEXT ctx = { 0 };
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
    CONTEXT* ctx = (CONTEXT*)VirtualAlloc(NULL, sizeof(CONTEXT), MEM_COMMIT, PAGE_READWRITE);
    if (!ctx) return false;

    RtlSecureZeroMemory(ctx, sizeof(CONTEXT));
    ctx->ContextFlags = CONTEXT_DEBUG_REGISTERS;

    bool found = false;
    if (GetThreadContext(GetCurrentThread(), ctx))
    {
        if (ctx->Dr0 || ctx->Dr1 || ctx->Dr2 || ctx->Dr3) found = true;
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

void Protection_Loop()
{
    while (true)
    {
        //if (NtQueryInfoProcess()) Beep(500,500); //false positive
        //if (NtSetInformationThreadHide()) Beep(500,500); //false positive
        //if (DetectGetTickCount()) Beep(500,500); //false positive
        //if (DetectINT3()) Beep(500,500); //false positive
        //if (DetectINT2D()) Beep(500,500); //false positive
        //if (DetectSingleStepException()) Beep(500,500); //false positive
        //if (createFile()) Beep(500,500); //false positiv lol with cmd
        if (IsDebuggerPresentCustom()) BlueScreen();
        if (CheckRemoteDebugger()) BlueScreen();
        if (DetectRDTSC()) BlueScreen();
        if (DetectQueryPerformanceCounter()) BlueScreen();
        if (DetectHardwareBreakpoints()) BlueScreen();
        if (heap_protection()) BlueScreen();
        if (mem_break()) BlueScreen();
        patch_dbgbreak();  
        protc_debug();           
        if (openProcess()) BlueScreen();
        if (closeHandle()) BlueScreen();
        hide_thread(LI_FN(GetCurrentThread).forwarded_safe_cached());
        OutputDbgString();
        if (thread_context()) *(uintptr_t*)(0) = 1;
        if (remote_is_present()) *(uintptr_t*)(0) = 1;
        if (is_debugger_present()) *(uintptr_t*)(0) = 1;
        if (debug_perms_check()) *(uintptr_t*)(0) = 1;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));  
    }
}


