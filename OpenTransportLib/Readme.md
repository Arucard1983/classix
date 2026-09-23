# OpenTransport Wrapper

## Features
- A fast OpenTransport network stack implementation over the BSD network stack from Mac OS X, that is implemented by Darling (as part of libSystem).
- Only the functions related to the TCP/IP stacks (TCP/UDP), both sync and async are supported, with some safe stubs. Also it provides a safe null function to return an unsupported error feature without crashing the program.
- Fully thread-safe design utilizing native host synchronization (`std::mutex`) to safely handle endpoint contexts across the emulator's execution threads.
- Comprehensive hardware-independent stubs for low-level protocol operations (such as `OTIoctl` and `OTAckSends`), abstracting legacy Apple STREAMS architecture.
- Intelligent dynamic symbol dispatching via `LibraryLookup` that seamlessly routes undocumented or exotic Classic Mac OS API calls to a unified fallback handler.

## Architectural Notes & Scope
This wrapper completely drops support for legacy 68k Motorola architectures via strict `NotSupportedException` boundaries, focusing exclusively on native PowerPC Carbon-era applications (such as Diablo II, StarCraft, and Warcraft III). 

By leveraging native POSIX `poll` mechanics directly inside `OTLook`, it honors the strict frame-based polling loops of high-performance classic titles without requiring heavy asynchronous host interrupt queues. Memory translation safely ensures automated Big-Endian to Little-Endian conversion for all `MacInetAddressLayout` structures mapped from the guest Virtual Machine.

## Deep Technical Details

### 1. Memory Layout & Zero-Copy Translation
- **Host-Guest Boundary:** Instead of allocating mirror buffers, the wrapper uses the emulator's core `allocator.ToPointer<T>()` mapping mechanism. This provides direct execution over the PowerPC virtual memory addresses, achieving near zero-copy overhead during active payload transmissions (`OTSnd`/`OTRcv`).
- **Structure Realignment:** Classic Mac OS 9 structures like `MacTNetbuf` and `MacInetAddressLayout` use strict packing rules. The host alignment constraints are completely abstracted by manually reading fields via virtual pointer offsets, preventing compiler-induced padding mismatches between 32-bit Guest structures and 64-bit Host architectures.

### 2. Bi-Directional Endianness Safe Guarding
- **Automated Inversion:** The wrapper implements strict architectural isolation for network byte ordering. Classic Mac OS operates natively in Big-Endian, matching the network byte order (Wire Format). 
- **The Double-Inversion Trap:** POSIX functions like `htons()` and `htonl()` expect Host-Endian input. Since modern hosts are strictly Little-Endian (x86_64/ARM64), a blind cast would corrupt data. The implementation uses specialized `.Get()` and `.Set()` abstraction wrappers from the emulator's core framework to automatically normalize the PowerPC Big-Endian fields before they interact with the host system calls.

### 3. POSIX Socket Polymorphism (`OTUnbind` Reset Loop)
- **State Hard Reset:** Unlike legacy OS architectures, modern POSIX stacks do not support a clean `unbind()` command without destroying the file descriptor. 
- **The Safe-Recycle Pattern:** To mimic `OTUnbind` without causing `EADDRINUSE` (Address already in use) collisions when games cycle multiplayer rooms, the wrapper queries the active descriptor options using `getsockopt(..., SO_TYPE)`, destroys the old file descriptor safely, instantly binds a fresh native socket of the same family type, and transparently preserves the original cookie reference ID assigned to the Guest application.

---
*Preserving Carbon-era gaming history through native POSIX translation.*
