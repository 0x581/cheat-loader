#pragma once
#include <windows.h>
#include <winternl.h>
#include <intrin.h>

// Debugger Detection
bool IsDebuggerPresentCustom();
bool CheckRemoteDebugger();
bool NtQueryInfoProcess();
bool NtSetInformationThreadHide();

// Timing Attacks
bool DetectRDTSC();
bool DetectQueryPerformanceCounter();
bool DetectGetTickCount();

// Hardware Breakpoint Detection
bool DetectHardwareBreakpoints();

// Software Breakpoint Detection
bool DetectINT3();
bool DetectINT2D();
bool DetectSingleStepException();

// some more detect
bool DetectStackPointerManipulation();

// mem protect
void ProtectMemory(LPVOID lpAddress, SIZE_T dwSize);
void UnprotectMemory(LPVOID lpAddress, SIZE_T dwSize);
bool heap_protection();
bool mem_break();
void patch_dbgbreak();
void protc_debug();

//misc
bool openProcess();
bool createFile();
bool closeHandle();

//anti mem dump
bool DetectMemoryDumping();
void SetupAntiDump();