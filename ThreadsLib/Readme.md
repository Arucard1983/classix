# ThreadsLib - Classic Thread Manager Implementation

This module ties up the loose ends left in the original 2013 codebase, fully implementing the 22-function API of the classic Mac OS / Carbon **Thread Manager** for the **ClassiX** ecosystem. 

## Hybrid Architecture: Mach Threads & Linux Core
The implementation takes direct advantage of the **Darling** compatibility layer, allowing us to leverage native macOS kernel abstractions directly on Linux. Each thread spawned by the legacy environment maps to a real, modern execution flow:
* **True Concurrent Execution:** Driven by native Mach kernel primitives (`<mach/thread_act.h>`) and standard Linux `pthreads`.
* **State Isolation:** Each thread runs its own isolated instance of the PowerPC interpreter (`PPCVM::Execution::Interpreter`) paired with its own virtual register file (`PPCVM::MachineState`), enabling genuine multi-core hardware parallelism.

## Preemptive Execution with Cooperative Safeguards
The Classic Mac OS relied heavily on a strict cooperative concurrency model, where guest applications assumed no other thread could mutate state or interrupt execution outside of explicit yield points. To run this software safely inside a modern, highly preemptive OS without causing race conditions or confusing legacy programs, the library implements critical safety measures:

1. **Aggressive Multi-Thread Suspension (`ThreadBeginCritical` / `ThreadEndCritical`)**: 
   When a thread enters a Toolbox critical section, our `NativeThreadManager` acts at the microkernel level using `thread_suspend`. All other active guest threads registered in the environment are physically frozen on the spot, guaranteeing the strict atomicity expected by vintage code.
2. **Directed Context Switching (`YieldToThread`)**: 
   Instead of relying on a generic POSIX `sched_yield()`, we utilize the Mach-specific `thread_switch()` primitive. This passes a direct hint to the kernel scheduler to yield the remaining time slice exactly to the targeted guest thread, accurately mimicking original cooperative thread switching.
3. **RAII Lifecycle & Cleanup (`ExecutionMarker`)**: 
   Native thread lifecycles are wrapped in execution markers. If the virtual machine encounters a guest panic or throws an exception, the thread automatically unregisters itself from the global map, preventing dangling references, memory leaks, or cascading deadlocks.

With this implementation, the ClassiX thread concurrency subsystem is fully stabilized, secure, and production-ready for modern Linux environments via Darling.