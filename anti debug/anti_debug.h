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


bool DetectStackPointerManipulation();
