//
// ThreadsLib.cpp
// Classix
//
// Copyright (C) 2013 Félix Cloutier
//
// This file is part of Classix.
//
// Classix is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// Classix is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// Classix. If not, see http://www.gnu.org/licenses/.
//


#include <unordered_map>
#include <dlfcn.h>
#include <mach/mach.h>
#include <mach/thread_act.h>
#include <sched.h> // For sched_yield

#include "ThreadsLib.h"
#include "MachineState.h"
#include "Managers.h"


namespace ThreadsLib
{
	struct ThreadInfo
	{
		uint32_t threadID;   // O ID clássico (ex: índice ou endereço)
		thread_act_t machRef; // A thread Mach real correspondente
		uint16_t state;      // 0 = Ready, 1 = Stopped, 2 = Running
	};

        // Auxiliary Struct to pass the arguments for the new native thread
	struct ThreadArgs
	{
		Globals* globals;
		OSEnvironment::Managers* managers;
		uint32_t threadID;
		uint32_t entryPoint;
		uint32_t parameter;
		uint32_t stackSize;
		void* stackMemory;
	};

	struct Globals
	{
		Common::Allocator& allocator;
		OSEnvironment::ThreadManager& threadManager;
		
		// managing the classical threads with the native Mach threads
		std::unordered_map<thread_act_t, ThreadInfo> threadMap;
		uint32_t nextThreadID;
                Globals(Common::Allocator& allocator, OSEnvironment::ThreadManager& threadManager)
		: allocator(allocator), threadManager(threadManager), nextThreadID(100) // IDs starts from 100 by safety
		{ }

		// Helper to regist the current thread if it unknown from the Classical Environment
		uint32_t GetOrCreateCurrentThreadID()
		{
			thread_act_t self = ::mach_thread_self();
			auto it = threadMap.find(self);
			if (it == threadMap.end())
			{
				uint32_t id = nextThreadID++;
				threadMap[self] = { id, self, 2 }; // 2 = kRunningThreadState
				return id;
			}
			return it->second.threadID;
		}
	};
}

using ThreadsLib::Globals;
using PPCVM::MachineState;

extern "C" void* ThreadTrampoline(void* arg)
{
	ThreadsLib::ThreadArgs* tArgs = static_cast<ThreadsLib::ThreadArgs*>(arg);
	ThreadsLib::Globals* globals = tArgs->globals;
	
	{
		// 1. Enable RAII ExecutionMarker on current thread.
		// This will register automatically the current Mach thread on native OS Services ThreadManager from ClassiXCore!
		auto marker = globals->threadManager.CreateExecutionMarker();
		
		// 2. Create/Initialize the Virtual Machine (PPCVM::MachineState) to this thread
		PPCVM::MachineState localState;
		
		// 3. Apply the PowerPC thread calling convention:
		// On PowerPC, stacks grows downwords. register r1 should point to the END of allocated to the top (head of the stack).
		uintptr_t stackTop = reinterpret_cast<uintptr_t>(tArgs->stackMemory) + tArgs->stackSize;
		localState.r1 = static_cast<uint32_t>(stackTop - 16); // Security allignement with 16 bytes
		
		// The function parameter always enter on r3 register
		localState.r3 = tArgs->parameter;
		
		try
		{
			// 3. Start the dedicated interpreter to this thread
			// Using the global allocated with the local state stored before
			PPCVM::Execution::Interpreter interpreter(globals->allocator, localState);
			
			// Convert the typed pointer required by the Interpreter
			const Common::UInt32* startPC = globals->allocator.ToPointer<Common::UInt32>(tArgs->entryPoint);
			
			// Configure the LR to point the adress of the local VM
			localState.lr = globals->allocator.ToIntPtr(interpreter.GetEndAddress());

			// 4. Start the virtual Machine in sync with this native thread!
			interpreter.Execute(startPC);
		}
		catch (const std::exception& ex)
		{
			std::cerr << "*** ClassiX VM Exception on thread: " << ex.what() << std::endl;
		}
		catch (...)
		{
			std::cerr << "*** ClassiX VM Crashed!" << std::endl;
		}
	}
		

         // Change the classic map to stopped (1 = kStoppedThreadState)
	for (auto& pair : globals->threadMap)
	{
		if (pair.second.threadID == tArgs->threadID)
		{
			pair.second.state = 1;
			break;
		}
	}

             // Clean the memory stack allocated to this thread
	     globals->allocator.Deallocate(tArgs->stackMemory);
	     delete tArgs;
	     return nullptr;
}
	
	

extern "C"
{
	ThreadsLib::Globals* LibraryLoad(Common::Allocator* allocator, OSEnvironment::Managers* managers)
	{
		managers->Gestalt().SetValue("thds", 3);
		return allocator->Allocate<Globals>("ThreadsLib Globals", *allocator, managers->ThreadManager());
	}
	
	SymbolType LibraryLookup(Globals* globals, const char* name, void** result)
	{
		char functionName[40] = "ThreadsLib_";
		char* end = stpncpy(functionName + 11, name, 29);
		if (*end == 0)
		{
			if (void* symbol = dlsym(RTLD_SELF, functionName))
			{
				*result = symbol;
				return CodeSymbol;
			}
		}
		
		*result = nullptr;
		return SymbolNotFound;
	}
	
	void LibraryUnload(Globals* globals)
	{
		globals->allocator.Deallocate(globals);
	}
	
#pragma mark -
	void ThreadsLib_GetThreadCurrentTaskRef(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// r4 = Pointer to write the return of TaskRef
		// On ClassiX, mapping  TaskRef 1:1 with ThreadID for simplicity.
		uint32_t* outTaskRef = globals->allocator.ToPointer<uint32_t>(state->r4);
		
		if (outTaskRef != nullptr)
		{
			*outTaskRef = globals->GetOrCreateCurrentThreadID();
			state->r3 = 0; // noErr
		}
		else
		{
			state->r3 = -50; // paramErr
		}
	}
	
	void ThreadsLib_GetDefaultThreadStackSize(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		Common::SInt32& out = *globals->allocator.ToPointer<Common::SInt32>(state->r4);
		out = 512 * 1024;
		state->r3 = 0;
	}
	
	void ThreadsLib_SetThreadTerminator(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// r4 = ThreadID from target thread
		// r5 = pointer to the unsafe classic function (ThreadTerminatorProcPtr)
		// Esta função define a callback when a thread dies.
		// Return noErr for compability, leaving the native Mach threads menaging threads.
		state->r3 = 0; // noErr
	}
	
	void ThreadsLib_DisposeThread(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// r4 = ThreadID to remove
		uint32_t targetID = state->r4;

		thread_act_t targetMach = MACH_PORT_NULL;
		auto it = globals->threadMap.end();

		// Search for classic ID on our global map
		for (auto iter = globals->threadMap.begin(); iter != globals->threadMap.end(); ++iter)
		{
			if (iter->second.threadID == targetID)
			{
				targetMach = iter->first;
				it = iter;
				break;
			}
		}

		if (it == globals->threadMap.end())
		{
			state->r3 = -192; // threadNotFoundErr
			return;
		}

		// If the thread still running(and not itself), kill it on Mach kernel 
                if (targetMach != ::mach_thread_self())
		{
			::thread_terminate(targetMach);
		}
		
		// Remove the local registers from API
		globals->threadMap.erase(it);

		state->r3 = 0; // noErr
	}
	
	void ThreadsLib_GetFreeThreadCount(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// r4 = ThreadStyle (Cooperative or Preemptive)
		// r5 = Pointer to count the number of free threads on global poll.
		// Since Darling/Linux allocates dynamically using pthreads and don't had an artificial 
		// limit, return an artificial higher number (ex: 64) to signal
		// the system can create more threads without constains on static memory.
		uint16_t* outCount = globals->allocator.ToPointer<uint16_t>(state->r5);
		
		if (outCount != nullptr)
		{
			*outCount = 64;
			state->r3 = 0; // noErr
		}
		else
		{
			state->r3 = -50; // paramErr
		}
	}
	
	void ThreadsLib_SetThreadSwitcher(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// r4 = ThreadID
		// r5 = ThreadSwitcherProcPtr (Callback trigger due to cooperative switching)
		// r6 = switchIn (boolean)
		// Since all pthreads run on paralled on Darling/Linux, realtime classic application switches
		// are totally obsolete and dangerous to implement. Return a safe value for compatibility.
		state->r3 = 0; // noErr
	}
	
	void ThreadsLib_YieldToThread(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// r4 = Specific ThreadID where timeslicing is given to
		// On Classic Mac OS, if r4 is 0, it will act like YieldToAnyThread.
		uint32_t targetID = state->r4;
		
		if (targetID == 0 || targetID == 1)
		{
			// call the generic cedence function
			ThreadsLib_YieldToAnyThread(globals, state);
			return;
		}

		ThreadsLib::ThreadInfo* targetInfo = nullptr;
		for (auto& pair : globals->threadMap)
		{
			if (pair.second.threadID == targetID)
			{
				targetInfo = &pair.second;
				break;
			}
		}

		if (!targetInfo)
		{
			state->r3 = -192; // threadNotFoundErr
			return;
		}

                // If the target thread is stop/suspend we cannot give time to it.
		// On classic environement, it return a specific error or simply resumes.
		if (targetInfo->state == 1) // 1 = kStoppedThreadState
		{
			state->r3 = -192; // threadNotFoundErr ou erro de estado inválido
			return;
		}

		// The magic of Mach on Darling: thread_switch enables to abdicate time on CPU 
		// specified exactly to the scheduler which Mach thread should run.
		// Parameters: 
		// 1st: Target thread (machRef)
		// 2nd: Switch Option (SWITCH_OPTION_NONE ot SWITCH_OPTION_DEPRESS to reduce temporarily our priority)
		// 3rd: Depression time (0 = default)
		kern_return_t kr = ::thread_switch(targetInfo->machRef, SWITCH_OPTION_NONE, 0);
		
		if (kr == KERN_SUCCESS)
		{
			state->r3 = 0; // noErr
		}
		else
		{
			// If fails due to some error on native subsistem, make the generic yield
			::sched_yield();
			state->r3 = 0;
		}

	}
	
	void ThreadsLib_GetSpecificFreeThreadCount(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// r4 = Pointer for a struct of thread characteristics (ThreadCharacteristics)
		// r5 = Pointer to write the count.
		// Follow a fixed higher number by safety.
		uint16_t* outCount = globals->allocator.ToPointer<uint16_t>(state->r5);
		
		if (outCount != nullptr)
		{
			*outCount = 64;
			state->r3 = 0; // noErr
		}
		else
		{
			state->r3 = -50; // paramErr
		}
	}
	
	void ThreadsLib_ThreadBeginCritical(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		globals->threadManager.EnterCriticalSection();
		state->r3 = 0;
	}
	
	void ThreadsLib_ThreadCurrentStackSpace(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// r4 = Ponteiro to obtain the remaining free stack (em bytes) of current thread.
		// This function was used on classic PowerPC to avoid Stack Overflow.
		uint32_t* outSpace = globals->allocator.ToPointer<uint32_t>(state->r4);
		
		if (outSpace != nullptr)
		{
			// Since the default stack size is 512KB on GetDefaultThreadStackSize,
			// and since peeking r1 relative to native real  stackTop adds overhead,
			// returning a static value (ex: 256KB livres) is a safe and 
			// function value for Carbon emulators, unless the application makes a thought verification.
			*outSpace = 256 * 1024; 
			state->r3 = 0; // noErr
		}
		else
		{
			state->r3 = -50; // paramErr
		}
	}
	
	void ThreadsLib_CreateThreadPool(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// r4 = ThreadStyle (Cooperative or Preemptive)
		// r5 = How many threads would be allocated.
		// Since the native pthreads are made dynamically, any preallocation is obsolete.
                state->r3 = 0; // noErr   
	}
	
	void ThreadsLib_SetDebuggerNotificationProcs(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// r4 = Pointer to the debugging function (DebuggerNewThreadProcPtr, etc.)
		// It was used by classic debuggers (ex: MacsBug) to intersect the thread life cycle.
		// It should be ignored due to safety
		state->r3 = 0; // noErr
	}
	
	void ThreadsLib_GetThreadStateGivenTaskRef(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// r4 = TaskRef of target thread
		// r5 = Pointer to get the state
		// On ClassiX, is like the normal GetThreadState, since TaskRef is equal to ThreadID.
		ThreadsLib_GetThreadState(globals, state);
	}
	
	void ThreadsLib_SetThreadState(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// r4 = ThreadID to change (0 ou 1 meaning the current thread)
		// r5 = New Estado (0 = kReadyThreadState, 1 = kStoppedThreadState)
		// r6 = ThreadParam (Usually ignored or stored, depending from the necessity to yield later)
		uint32_t targetID = state->r4;
		uint16_t newState = (uint16_t)state->r5;
		
		if (targetID == 0 || targetID == 1)
		{
			targetID = globals->GetOrCreateCurrentThreadID();
		}

		ThreadsLib::ThreadInfo* targetInfo = nullptr;
                for (auto& pair : globals->threadMap)
		{
			if (pair.second.threadID == targetID)
			{
				targetInfo = &pair.second;
				break;
			}
		}

		if (!targetInfo)
		{
			state->r3 = -192; // threadNotFoundErr
			return;
		}

		uint16_t oldState = targetInfo->state;
		if (oldState == newState)
		{
			state->r3 = 0; // noErr, no changes
			return;
		}

                // Update the ste on classical registers
		targetInfo->state = newState;

		// If a target thread is the current thread that is trying to stop itself:
		if (targetInfo->machRef == ::mach_thread_self())
		{
			if (newState == 1) // Current thread decided to stop (kStoppedThreadState)
			{
				state->r3 = 0;
				// Self-suspected. On mach/Darling this fronzen the curent flux immediately
				// When another thread make resume, the flux wake up on right on the next instruction.
				::thread_suspend(targetInfo->machRef);
				return;
			}
		}
		else
		{
			// Change state to an external thread
			if (newState == 1 && oldState == 0) // Ready -> Stopped
			{
				::thread_suspend(targetInfo->machRef);
			}
			else if (newState == 0 && oldState == 1) // Stopped -> Ready
			{
				::thread_resume(targetInfo->machRef);
			}
		}

            state->r3 = 0; // noErr		
	}
	
	void ThreadsLib_GetThreadState(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// r4 = ThreadID found (0 / kCurrentThreadID is the current thread)
		// r5 = Pointer to retrieve the state (uint16_t/uint32_t depending from the classic alignment)
		uint32_t targetID = state->r4;
		uint16_t* outState = globals->allocator.ToPointer<uint16_t>(state->r5);
		
		if (targetID == 0 || targetID == 1) // Canonical Constants for the "Current Thread"
		{
			targetID = globals->GetOrCreateCurrentThreadID();
		}

		// Find on map by ID
		bool found = false;
		for (auto& pair : globals->threadMap)
		{
			if (pair.second.threadID == targetID)
			{
				if (outState) *outState = pair.second.state;
				found = true;
				break;
			}
		}

		if (found)
		{
			state->r3 = 0; // noErr
		}
		else
		{
			state->r3 = -192; // threadNotFoundErr
		}
	}
	
	void ThreadsLib_NewThread(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		/ r4 = ThreadStyle, r5 = ThreadEntryProcPtr, r6 = ThreadParam
		// r7 = StackSize, r8 = Options, r9 = outThreadID, r10 = outTaskRef
		uint32_t entryPoint = state->r5;
		uint32_t parameter = state->r6;
		uint32_t stackSize = state->r7 ? state->r7 : (512 * 1024); // 512KB standard if get zero
		uint32_t options = state->r8;
		uint32_t* outThreadID = globals->allocator.ToPointer<uint32_t>(state->r9);

		if (!outThreadID)
		{
			state->r3 = -50; // paramErr
			return;
		}

		// 1. Create the new classic thread tag
		uint32_t newID = globals->nextThreadID++;
		*outThreadID = newID;
		
		if (state->r10) // If the program request TaskRef, map as 1:1 for simplicity
		{
			uint32_t* outTaskRef = globals->allocator.ToPointer<uint32_t>(state->r10);
			if (outTaskRef) *outTaskRef = newID;
		}

                // 2. Allocate the physical memory that the PPCVM will need as the classical stack
		void* stackMem = globals->allocator.Allocate<uint8_t>("Thread Stack", stackSize);

		// 3. Package the arguments needed to be used by the new native thread
		ThreadsLib::ThreadArgs* args = new ThreadsLib::ThreadArgs{
			globals, newID, entryPoint, parameter, stackSize, stackMem
		};

		// Configure the attributes of the native pthread (native stack with extra room)
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, stackSize + (64 * 1024));

		pthread_t threadNative;
		int result = ::pthread_create(&threadNative, &attr, ThreadTrampoline, args);
		pthread_attr_destroy(&attr);

		if (result != 0)
		{
			globals->allocator.Deallocate(stackMem);
                        delete args;
			state->r3 = -191; // threadCreationErr
			return;
		}

		// 4. Extract the Mach thread from Darling using the recent created pthread
		thread_act_t machRef = ::pthread_mach_thread_np(threadNative);

		// Check if the thread was suspended by argument (kCreateSuspended = 1 on options bit)
		uint16_t initialState = (options & 1) ? 1 : 0; 
		globals->threadMap[machRef] = { newID, machRef, initialState };

		if (initialState == 1)
		{
			::thread_suspend(machRef);
		}

		state->r3 = 0; // noErr (Sucess!)
			
	}
	
	void ThreadsLib_SetThreadStateEndCritical(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// This function is an atomic optimization from Classic Mac OS. 
		// It changes the thread state AND quickly exit the critical section.
		// It save a system call (one transition context from the Toolbox).
		
		// 1. Execute the normal SetThreadState
		ThreadsLib_SetThreadState(globals, state);
		
		// 2. If the SetThreadState fail (r3 != 0), normally it depends that it should or not exit.
		// But usually, the classical ecosystem force the critical section anyway.
		globals->threadManager.ExitCriticalSection();
	}
	
	void ThreadsLib_SetThreadScheduler(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// r4 = ThreadSchedulerProcPtr (Callback to replace the classical scheduler algorithm)
		// Darling always use the the native scheduler from kernel Linux, and Mach one.
		state->r3 = 0; // noErr
	}
	
	void ThreadsLib_GetCurrentThread(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// r4 contains the pointer where it should write the return ThreadID pointer
		uint32_t* outThreadID = globals->allocator.ToPointer<uint32_t>(state->r4);
		
		if (outThreadID != nullptr)
		{
			*outThreadID = globals->GetOrCreateCurrentThreadID();
			state->r3 = 0; // noErr
		}
		else
		{
			state->r3 = -192; // threadNotFoundErr or paramErr generic error
		}
	}
	
	void ThreadsLib_ThreadEndCritical(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		globals->threadManager.ExitCriticalSection();
		state->r3 = 0;
	}
	
	void ThreadsLib_SetThreadReadyGivenTaskRef(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// On Classic/Carbon envrironment, some low level API reference the threads by "TaskRef"
		// instead "ThreadID". In many simplified emulation subsystrems, they are mapped 1:1.
		// r4 = TaskRef/ThreadID of the thread to reativate
		
		uint32_t targetID = state->r4;
		
		ThreadsLib::ThreadInfo* targetInfo = nullptr;
		for (auto& pair : globals->threadMap)
		{
			if (pair.second.threadID == targetID)
			{
				targetInfo = &pair.second;
				break;
			}
		}

		if (!targetInfo)
		{
			state->r3 = -192; // threadNotFoundErr
			return;
		}

		if (targetInfo->state == 1) // If they are Stopped (1), changes to Ready (0)
		{
			targetInfo->state = 0;
			::thread_resume(targetInfo->machRef);
		}

		state->r3 = 0; // noErr
	}

        void ThreadsLib_SetThreadReady(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// r4 = ThreadID of the thread to pass to state "Ready"
		// Note: Some versions of this Threads Toolbox function 'SetThreadReady' just passes 
		// to the function 'SetThreadReadyGivenTaskRef
		uint32_t targetID = state->r4;
		
		ThreadsLib::ThreadInfo* targetInfo = nullptr;
		for (auto& pair : globals->threadMap)
		{
			if (pair.second.threadID == targetID)
			{
				targetInfo = &pair.second;
				break;
			}
		}

		if (!targetInfo)
		{
			state->r3 = -192; // threadNotFoundErr
			return;
		}

		if (targetInfo->state == 1) // If it is Stopped (1), makes them Ready (0)
		{
			targetInfo->state = 0;
			::thread_resume(targetInfo->machRef);
		}

		state->r3 = 0; // noErr
	}
	
	void ThreadsLib_YieldToAnyThread(ThreadsLib::Globals* globals, PPCVM::MachineState* state)
	{
		// On Classical Mac OS this function gives time to another thread.
		// Since the OS Services built-in ThreadManager from ClassiXCore uses agressive thread suspend/resume, all native threads
		// will be running in parallell on Darling if they not be within a critical section.
		// Calling the Mach/POSIX/Linux sched_yield() will warn the Linux kernel that this thread will abdicate the current timesclice.
		
                kern_return_t kr = ::thread_switch(MACH_PORT_NULL, SWITCH_OPTION_NONE, 0);
		
		if (kr != KERN_SUCCESS)
		{
			::sched_yield(); 
	}
}