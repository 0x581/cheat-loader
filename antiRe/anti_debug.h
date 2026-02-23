#pragma once
#include <windows.h>
#include <winternl.h>
#include <intrin.h>

// ==================== DEBUGGER DETECTION ====================
bool IsDebuggerPresentCustom();
bool CheckRemoteDebugger();
bool NtQueryInfoProcess();
bool NtSetInformationThreadHide();

// PEB-based checks
bool CheckNtGlobalFlag();
bool CheckHeapFlags();

// NtQueryInformationProcess checks
bool CheckProcessDebugPort();
bool CheckProcessDebugObjectHandle();
bool CheckProcessDebugFlags();
bool CheckKernelDebugger();
bool CheckParentProcess();

// Debugger window/process checks
bool CheckDebuggerWindows();
bool CheckDebuggerProcesses();
bool CheckAnalysisDLLs();

// Anti-attach
void AntiAttach();
void PatchDbgUiRemoteBreakin();

// ==================== TIMING ATTACKS ====================
bool DetectRDTSC();
bool DetectQueryPerformanceCounter();
bool DetectGetTickCount();
bool AdvancedTimingCheck();

// ==================== BREAKPOINT DETECTION ====================
bool DetectHardwareBreakpoints();
bool DetectINT3();
bool DetectINT2D();
bool DetectSingleStepException();
bool DetectStackPointerManipulation();

// ==================== MEMORY PROTECTION ====================
void ProtectMemory(LPVOID lpAddress, SIZE_T dwSize);
void UnprotectMemory(LPVOID lpAddress, SIZE_T dwSize);
bool heap_protection();
bool mem_break();
void patch_dbgbreak();
void protc_debug();

// ==================== MISC ====================
bool openProcess();
bool createFile();
bool closeHandle();

// ==================== ANTI-DUMP ====================
bool DetectMemoryDumping();
void SetupAntiDump();
void ErasePEHeader();
void CorruptSizeOfImage();
void CorruptSectionHeaders();
void HookNtReadVirtualMemory();
void UnlinkFromPEB();
void SetGuardPagesOnHeader();
void EraseIAT();
void EraseEAT();
void EraseRelocations();
void EraseDebugDirectory();
void EraseTLSCallbacks();
void WipeAllDataDirectories();

// ==================== ANTI-VM / SANDBOX ====================
bool DetectVMware();
bool DetectVirtualBox();
bool DetectHyperV();
bool DetectVMViaCPUID();
bool DetectSandbox();
bool CheckScreenResolution();
bool CheckMouseMovement();
bool CheckDiskSize();
bool CheckProcessorCount();
bool CheckRAMSize();
bool DetectWine();

// ==================== INTEGRITY CHECKS ====================
void InitIntegrityCheck();
bool VerifyIntegrity();
extern DWORD g_originalChecksum;

// ==================== OBFUSCATION ====================
void AntiDisasm();
bool OpaquePredicateTrue();