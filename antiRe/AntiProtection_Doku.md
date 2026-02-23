# Anti-Debug, Anti-Dump und Anti-Reverse Maßnahmen – Dokumentation

Diese Dokumentation beschreibt die im Projekt implementierten Schutzmechanismen gegen Debugging, Reverse Engineering und Dumping. Sie erklärt, wie die einzelnen Techniken funktionieren, was sie bewirken und welche Risiken/Kompatibilitätsprobleme auftreten können.

## Übersicht der Schutzmechanismen

### 1. Anti-Debugging (Laufzeit-Erkennung)
- **IsDebuggerPresentCustom, CheckRemoteDebugger, CheckNtGlobalFlag, CheckHeapFlags, CheckProcessDebugPort, CheckProcessDebugObjectHandle, CheckProcessDebugFlags, CheckKernelDebugger, CheckDebuggerWindows, CheckDebuggerProcesses, CheckAnalysisDLLs**
  - Prüfen, ob ein Debugger am Prozess hängt (verschiedene Windows-APIs und Heuristiken).
  - Ziel: Sofortige Erkennung von klassischen Debuggern (x64dbg, OllyDbg, WinDbg, Visual Studio etc.).
  - Wirkung: Erkennt Debugger und kann je nach Modus Crash, Exit, BSOD oder nur Logging auslösen.
  - Risiko: Sehr stabil, kaum False Positives.

- **Timing-Checks (DetectRDTSC, DetectQueryPerformanceCounter, AdvancedTimingCheck)**
  - Prüfen, ob Zeitmanipulationen (Breakpoints, Step-by-Step) stattfinden.
  - Ziel: Erkennung von Breakpoint-Stepping und Zeitmanipulation durch Debugger.
  - Wirkung: Erhöht die Erkennungsrate von "stillen" Debuggern.
  - Risiko: Kann auf sehr langsamen Systemen False Positives liefern.

- **Breakpoint-Detection (DetectHardwareBreakpoints, heap_protection, mem_break)**
  - Prüfen, ob Hardware-Breakpoints oder Speicher-Manipulationen aktiv sind.
  - Ziel: Erkennung von fortgeschrittenen Debugging-Techniken.
  - Wirkung: Schutz gegen HW-Breakpoints und Heap-Manipulation.
  - Risiko: Sehr stabil.

- **Integrity-Check (VerifyIntegrity)**
  - Prüft, ob das eigene Binary oder Speicherbereiche verändert wurden.
  - Ziel: Schutz gegen Patching und Code-Injection.
  - Wirkung: Erkennt Manipulationen am eigenen Code.
  - Risiko: Kann bei legitimen Änderungen (z.B. durch AV) anschlagen.

### 2. Anti-Dump & Anti-Reverse Maßnahmen
- **EraseIAT**
  - Löscht die Import Address Table (IAT) im Speicher.
  - Ziel: Erschwert das Dumpen und spätere Reversen, da keine Imports mehr sichtbar sind.
  - Wirkung: Tools wie Scylla oder x64dbg können keine Imports mehr rekonstruieren.
  - Risiko: Stabil, solange keine dynamischen Imports nachgeladen werden.

- **EraseEAT**
  - Löscht die Export Address Table (EAT) im Speicher.
  - Ziel: Erschwert das Auffinden von exportierten Funktionen.
  - Wirkung: Dump-Tools und Reverser finden keine Exports mehr.
  - Risiko: Stabil, solange keine Exports benötigt werden.

- **CorruptSectionHeaders**
  - Manipuliert die Section-Header der PE-Datei im Speicher.
  - Ziel: Erschwert das Dumpen und spätere Analysieren der Datei.
  - Wirkung: Dump-Tools können keine korrekten Sections mehr rekonstruieren.
  - Risiko: Stabil, solange keine Tools auf die Section-Header zugreifen müssen.

- **EraseRelocations**
  - Löscht die Relocation-Tabelle im Speicher.
  - Ziel: Erschwert das Remapping und Dumpen auf anderen Adressen.
  - Wirkung: Dump-Tools können keine Relocations mehr rekonstruieren.
  - Risiko: Stabil, solange keine Relocations benötigt werden.

### 3. Instabile/optionale Maßnahmen (nicht aktiviert)
- **ErasePEHeader, UnlinkFromPEB, HookNtReadVirtualMemory, SetGuardPagesOnHeader**
  - Sehr aggressive Techniken, die das Binary oder die Windows-Strukturen so verändern, dass viele Tools/Loader/Debugger sofort crashen.
  - Wirkung: Maximale Erschwernis für Reverse Engineering, aber hohe Instabilität.
  - Risiko: Führt auf vielen Systemen zu Abstürzen oder verhindert das Starten des Programms.

### 4. Zentrale Steuerung
- **ProtectionMode**
  - Ermöglicht die Auswahl, wie auf eine Erkennung reagiert wird:
    - BSOD (BlueScreen)
    - Exit (sauberes Beenden)
    - Crash (absichtlicher Speicherfehler)
    - Silent (nur Logging, keine Aktion)
  - Vorteil: Flexibles Testen und Anpassen der Reaktion auf Debugging.

