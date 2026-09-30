# Darling Super-Rosetta (WIP) 🚀

This fork is a complete refactoring of the original **ClassiX** emulator (stagnant since 2013), reinventing it as an open-source, high-performance **PowerPC Rosetta translation layer** tailored for the **Darling** environment. 

Our ultimate goal is to enable modern operating systems to execute legacy **PowerPC (PPC) 32-bit applications** seamlessly, acting as a direct parallel to Apple's original Rosetta transition technology. This repository is architected with the long-term vision of becoming an upstream component of the Darling project.

---

## 🎯 Refactored Goals & Architectural Blueprint

### 1. Pure PowerPC Focus
* **Dropping the Legacy Ceiling:** Completely dropped legacy 68k emulation, XCOFF, and obsolete system extensions. 
* **The Real-Memory Reality Check:** Emulating 68k within a Wine-like environment is equivalent to granting direct hardware/real-memory access of an MS-DOS application to a modern Linux kernel. Legacy VISE/Aladdin installers mixed with illegal system extensions and raw 68k assembly will not be emulated; instead, native open-source extraction tools should be used inside Darling to unpack these folders safely.
* **Modern PPC Core:** Focus is restricted exclusively to pure, robust PowerPC 32-bit code emulation, opening a clear runway for potential G4 emulation as a long-term goal.

### 2. Comprehensive Executable Handlers
* **Mach-O Support:** Implemented full Mach-O PowerPC 32-bit parsing and execution, providing an authentic Rosetta-like pass-through experience.
* **Dual-Target PEF Handler:** The Preferred Executable Format (PEF) handler has been decoupled from classic limits. It now supports both legacy OS 9 binaries and early Mac OS X PPC applications that wrapped modern OS X frameworks inside PEF containers.

### 3. Smart Gestalt & Environment Modes
The emulator's built-in **Gestalt** manager now dynamically shifts behaviors on the fly based on runtime analysis:
* **Mac OS 9.2.2 Mode:** Automatically triggered if the binary imports `InterfaceLib` and the Classic Environment active key is flagged.
* **Mac OS X 10.4.11 Mode:** Automatically triggered if the binary targets `CarbonLib` and the Classic Environment is disabled.
* *Note:* To maintain modern compatibility, even under OS 9 mode, calls are intercepted and safely dispatched to the host's native `CarbonLib`/Carbon frameworks.

## 🏗️ How the Execution Framework Works

The `classix` core deploys an anchor binary that reads the file header, checks the active environment constraints, and pipelines the code through a distinct fallback cascade:

Target Executable Binary
1.  Mach-O PPC -> Full Pass-Through (Rosetta Mode) -> Darling Frameworks
  
2. PEF Binary -> Inspect Symbol Tables
 - Start CarbonLibHead for IPC calls with Cocoa -> Native Darling Bridge
   
 a. Match CarbonLib Tree -> Host Carbon Framework

 b. Match ClassiX ToolBox -> In-Emulator Subroutines
 
 c. No Match Found -> Default "Not Supported" / Fatal Stub

 ### The ToolBox Interception Strategy
The built-in ClassiX ToolBox serves as a curated, high-safety fallback layer (similar to Win16 support on Wine or WowExec on Windows NT). Well-behaved applications run smoothly; malicious or raw-hardware-probing calls are safely trapped and terminated. 
* ToolBox structures map directly to an equivalent modern Darling Framework.
* **The Passive Principle:** Subsystems are abstracted cleanly. For instance, if an app requests **Open Transport**, ClassiX wires it into the host's `libSystem with BSD sockets` under a passive model: the app can read active network settings, stream, and receive data, but is strictly forbidden from installing drivers or modifying virtual network adapters.

---

## 🚦 Current Development Status

### 🍏 What is Done & Implemented:
* **Dynamic Universal FFI Engine:** Replaced specialized, static FFI calls with a highly robust runtime parser. It translates virtual 32-bit guest pointer addresses into real 64-bit host memory spaces on the fly, seamlessly supporting mixed-argument signatures (integers and decimals simultaneously) for critical functions like `printf` or `sqrt`.
* **Safe Mach-O Container Mapping:** Integrated a fully boundary-checked Mach-O parser protecting the virtual machine against corrupted segments or truncated `LC_UNIXTHREAD` commands.
* **Preemptive Multi-Threading Engine (`Mach-O Threads`):** Program loops no longer collide. Implemented a fully functional multi-threading trampoline. When a Mach-O program triggers `pthread_create` via `libSystem`, ClassiX intercepts it, captures a snapshot of the parent CPU registers, and spawns a native OS thread with a completely isolated `PPCVM::MachineState`.
* **Open Transport Modernization:** Completely rewritten using the host's native `libSystem` under the hood.
* **Thread Manager:** Legacy OS 9 Thread Manager mechanics are now translated directly into preemptive host `pthreads` via `libSystem`.

### 🛠️ Remaining Scope (The Finish Line):
To fully close the architectural scope of this project, only two main features are left:
* 🟥 **OpenGL Bridging:** Translating PowerPC OpenGL structures into native host GPU calls through Darling.
* 🟥 **Game Sprockets Components:** Implementing essential legacy input/sound abstractions (InputSprocket, SoundSprocket) to restore compatibility with classic 3D apps and games.
* 🟥 **CarbonLib with InterfaceLib shadow:** Work on CarbonLib is only started, with many Managers reduced to stubs or critical exceptions. Implementing the real stuff may face compromisses.

  ---

## 🛠️ Compilation & Testing

TODO: The original XCode compiling scripts need to be ported to CMAKE.
Once made this step, to run any PPC program will require to open a Darling Shell, and then:
```bash
./classix -r /path/to/your/powerpc_binary
```

## Why the changes ?
  - The original ClassiX project, that was focused on OS 9 was focused to run the major number possible of Mac OS Classic programs, but the sheer scale and the bad ToolBox documentation prevent any real progress.
  - Many older Mac OS 9 and earlier used the underlying 68k emulator to handle low level access operating system functionality that is not feasible to a Wine or a NTVDM/WOW implementation. It was like giving vxd driver support for NT or Linux kernel.
  - Focusing on Carbon or CarbonLib, even on runtime, is more feasible and catches the critical transition to Mac OS X era. It contains over 70% of older ToolBox with curated safety handlers. This is like the Win16 support on Wine, or the WowExec on Windows NT. Only well behaved programs works, if it calls DOS (NTVDM) it will be catch and emulated, or simply killed due to fundamental incompatibilities.
  - Like NTVDM (DosBox is more like an Executor for a MacBox), some ToolBox functions could be implemented in the ClassiX.framework as the same functionality exists on some Darling framework. Otherwise, dispatch the Fatal Error.

  ## The removal of any plan to support 68k will not cause issues ?
  - On original OS 9, even using PowerPC programs, any system call will soon or later call several times by a second subroutines written in 68k assembly, using the Mixed Code Manager. This project is intended for pure PPC code.
  - And if you dare to ask, VISE installers, Aladdin Stuff-It Installers are a nightmare of compatibility with illegal system extensions, and 68k code mixed with earlier PowerPC assembly. A real solution is a Darling Installer with opens source tools to unpack this stuff and rebuild the program folder without dangerous and unneeded stuff.
  - Running 68k on a Wine like environment is like giving real memory access of a MS-DOS program to modern Linux. This requires a full system emulator like Executor 2000 or M.A.C.E. if you do not want real Apple System Software.
