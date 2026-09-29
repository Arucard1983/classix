//
// MacMemory.cpp
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

#include <sstream>
#include <iostream>
#include <cstring>
#include "Prototypes.h"
#include "NotImplementedException.h"
#include "CarbonLib.h"

// Set Global memory handling error
static int16_t g_LastMemError = 0;

void CarbonLib_ApplicationZone(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0; g_LastMemError = 0;
}

void CarbonLib_BlockMove(CarbonLib::Globals* globals, MachineState* state)
{
	uint32_t srcAddr = state->r3;
	uint32_t dstAddr = state->r4;
	size_t size = state->r5;

	if (size == 0) { g_LastMemError = 0; return; }

	// Check if the memory pages are valid before memmove
	if (!globals->allocator.IsValidAddress(srcAddr, size) || !globals->allocator.IsValidAddress(dstAddr, size))
	{
		std::cerr << "[CarbonLib Fatal] Invalid BlockMove! Src: 0x" << std::hex << srcAddr 
		          << " Dst: 0x" << dstAddr << " Size: " << std::dec << size << std::endl;
		g_LastMemError = memWZErr;
		return;
	}

	const uint8_t* source = globals->allocator.ToPointer<uint8_t>(srcAddr);
	uint8_t* destination = globals->allocator.ToPointer<uint8_t>(dstAddr);
	
	std::memmove(destination, source, size);
	g_LastMemError = 0;
}

void CarbonLib_BlockMoveData(CarbonLib::Globals* globals, MachineState* state)
{
	CarbonLib_BlockMove(globals, state); //On Carbon this function are an alias.
}

void CarbonLib_CompactMem(CarbonLib::Globals* globals, MachineState* state)
{
	 state->r3 = 1024 * 1024; g_LastMemError = 0;
}

void CarbonLib_CompactMemSys(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 1024 * 1024; g_LastMemError = 0;
}

void CarbonLib_DebuggerEnter(CarbonLib::Globals* globals, MachineState* state)
{
	//No-op
}

void CarbonLib_DebuggerExit(CarbonLib::Globals* globals, MachineState* state)
{
	//No-op
}

void CarbonLib_DebuggerGetMax(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DebuggerLockMemory(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_DebuggerPoll(CarbonLib::Globals* globals, MachineState* state)
{
	//No-Op
}

void CarbonLib_DebuggerUnlockMemory(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_DeferUserFn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DisposeHandle(CarbonLib::Globals* globals, MachineState* state)
{
	uint32_t handleAddr = state->r3;
	if (handleAddr == 0) { g_LastMemError = -109; return; }

	if (!globals->allocator.IsValidAddress(handleAddr, sizeof(uint32_t)))
	{
		g_LastMemError = -111;
		return;
	}

	uint32_t* masterPointer = globals->allocator.ToPointer<uint32_t>(handleAddr);
	uint32_t dataAddr = *masterPointer;

	if (dataAddr != 0 && globals->allocator.IsValidAddress(dataAddr, 1))
	{
		void* dataBlock = globals->allocator.ToPointer<void>(dataAddr);
		globals->allocator.Deallocate(dataBlock);
	}

	globals->allocator.Deallocate(masterPointer);
	g_LastMemError = 0;
}

void CarbonLib_DisposePtr(CarbonLib::Globals* globals, MachineState* state)
{
	uint32_t ptrAddr = state->r3;
	if (ptrAddr == 0) { g_LastMemError = paramErr; return; }

	if (globals->allocator.IsValidAddress(ptrAddr, 1))
	{
		void* ptr = globals->allocator.ToPointer<void>(ptrAddr);
		globals->allocator.Deallocate(ptr);
		g_LastMemError = 0;
	}
	else
	{
		g_LastMemError = -111; //memWZErr
	}
}

void CarbonLib_EmptyHandle(CarbonLib::Globals* globals, MachineState* state)
{
	g_LastMemError = 0;
}

void CarbonLib_EnterSupervisorMode(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_FlushMemory(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_FreeMem(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 16 * 1024 * 1024; g_LastMemError = 0;
}

void CarbonLib_FreeMemSys(CarbonLib::Globals* globals, MachineState* state)
{
        state->r3 = 16 * 1024 * 1024; g_LastMemError = 0;
}

void CarbonLib_GetApplLimit(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetHandleSize(CarbonLib::Globals* globals, MachineState* state)
{
	uint32_t handleAddr = state->r3; // Binary Handle from PPC binary
	
	// 1. Validar se o Handle é nulo
	if (handleAddr == 0)
	{
		state->r3 = 0; // Retorna tamanho 0
		g_LastMemError = -109;
		return;
	}

	// 2. Validar se o Master Pointer reside numa página de memória legítima
	if (!globals->allocator.IsValidAddress(handleAddr, sizeof(uint32_t)))
	{
		state->r3 = 0;
		g_LastMemError = -111; // Erro de bloco livre/inválido
		return;
	}

	uint32_t* masterPointer = globals->allocator.ToPointer<uint32_t>(handleAddr);
	uint32_t dataAddr = *masterPointer;

	// 3. Se o Master Pointer apontar para 0, o Handle está vazio (purgado/vazio)
	if (dataAddr == 0)
	{
		state->r3 = 0;
		g_LastMemError = 0;
		return;
	}

	// 4. Obter o tamanho real do bloco de dados a partir do alocador da VM
	if (auto details = globals->allocator.GetDetails(dataAddr))
	{
		state->r3 = details->GetSize(); // r3 recebe o tamanho em bytes
		g_LastMemError = 0;
	}
	else
	{
		state->r3 = 0;
		g_LastMemError = -111;
	}
}

void CarbonLib_GetPageState(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetPhysical(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetPtrSize(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetVolumeVirtualMemoryInfo(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetZone(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0; g_LastMemError = 0;
}

void CarbonLib_GZSaveHnd(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_HandAndHand(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_HandleZone(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_HandToHand(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_HClrRBit(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_HGetState(CarbonLib::Globals* globals, MachineState* state)
{
	uint32_t handleAddr = state->r3;
	if (handleAddr == 0) { g_LastMemError = -109; return; }
	g_LastMemError = 0;
}

void CarbonLib_HLock(CarbonLib::Globals* globals, MachineState* state)
{
	// Bogus flag and null return
	state->r3 = 0x80; 
	g_LastMemError = 0;
}

void CarbonLib_HLockHi(CarbonLib::Globals* globals, MachineState* state)
{
	g_LastMemError = 0;
}

void CarbonLib_HNoPurge(CarbonLib::Globals* globals, MachineState* state)
{
	g_LastMemError = 0;
}

void CarbonLib_HoldMemory(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_HPurge(CarbonLib::Globals* globals, MachineState* state)
{
	g_LastMemError = 0;
}

void CarbonLib_HSetRBit(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_HSetState(CarbonLib::Globals* globals, MachineState* state)
{
	g_LastMemError = 0;
}

void CarbonLib_HUnlock(CarbonLib::Globals* globals, MachineState* state)
{
	uint32_t handleAddr = state->r3;
	if (handleAddr == 0) { g_LastMemError = -109; return; }
	g_LastMemError = 0;
}

void CarbonLib_InitApplZone(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_InitZone(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_InlineGetHandleSize(CarbonLib::Globals* globals, MachineState* state)
{
	CarbonLib_GetHandleSize(globals, state);
}

void CarbonLib_LockMemory(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_LockMemoryContiguous(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_LockMemoryForOutput(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_MakeMemoryNonResident(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_MakeMemoryResident(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_MaxApplZone(CarbonLib::Globals* globals, MachineState* state)
{
	g_LastMemError = 0;
}

void CarbonLib_MaxBlock(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_MaxBlockSys(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_MaxMem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_MaxMemSys(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_MemError(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_MoreMasters(CarbonLib::Globals* globals, MachineState* state)
{
	g_LastMemError = 0;
}

void CarbonLib_MoveHHi(CarbonLib::Globals* globals, MachineState* state)
{
	g_LastMemError = 0;
}

void CarbonLib_NewEmptyHandle(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_NewEmptyHandleSys(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_NewHandle(CarbonLib::Globals* globals, MachineState* state)
{
	size_t requestedSize = state->r3;
	
	// Carbon require opaque and clean Handles. 
	// To simulate the Master Pointer (**Handle), allocate the data block
	// and a small 4 bytes block to serve as intermediate pointer
	try {
		std::stringstream handleName;
		handleName << "CarbonLib Handle Data [" << requestedSize << "]";
		uint8_t* dataBlock = globals->allocator.Allocate(handleName.str(), requestedSize);

		// Allocate the Master Pointer (the adress that the program will store and deferenciate)
		uint32_t* masterPointer = globals->allocator.Allocate<uint32_t>("CarbonLib Master Pointer");
		*masterPointer = globals->allocator.ToIntPtr(dataBlock);

		// Return the Master Pointer adress (Handle)
		state->r3 = globals->allocator.ToIntPtr(masterPointer);
		g_LastMemError = 0;
	} catch (...) {
		state->r3 = 0;
		g_LastMemError = -108; //memFullErr
	}
}

void CarbonLib_NewHandleClear(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_NewHandleSys(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_NewHandleSysClear(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_NewPtr(CarbonLib::Globals* globals, MachineState* state)
{
	size_t size = state->r3;
	if (size == 0) { state->r3 = 0; g_LastMemError = 0; return; }

	try {
		void* ptr = globals->allocator.Allocate("Program Allocation (Ptr)", size);
		state->r3 = globals->allocator.ToIntPtr(ptr);
		g_LastMemError = 0;
	} catch (...) {
		state->r3 = 0;
		g_LastMemError = -108; //memFullErr
	}
}

void CarbonLib_NewPtrClear(CarbonLib::Globals* globals, MachineState* state)
{
	// Allocated and clear with zeroes (neede for older structures)
	CarbonLib_NewPtr(globals, state);
	if (state->r3 != 0)
	{
		void* ptr = globals->allocator.ToPointer<void>(state->r3);
		std::memset(ptr, 0, state->r4); // r4 or stored size
	}
}

void CarbonLib_NewPtrSys(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_NewPtrSysClear(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PageFaultFatal(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PtrAndHand(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PtrToHand(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PtrToXHand(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PtrZone(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PurgeMem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PurgeMemSys(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PurgeSpace(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PurgeSpaceContiguous(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PurgeSpaceSysContiguous(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PurgeSpaceSysTotal(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PurgeSpaceTotal(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ReallocateHandle(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ReallocateHandleSys(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_RecoverHandle(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_RecoverHandleSys(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ReleaseMemoryData(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ReserveMem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ReserveMemSys(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetApplBase(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetApplLimit(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetGrowZone(CarbonLib::Globals* globals, MachineState* state)
{
	g_LastMemError = 0;
}

void CarbonLib_SetHandleSize(CarbonLib::Globals* globals, MachineState* state)
{
	uint32_t handleAddr = state->r3;
	size_t newSize = state->r4; // Novo tamanho solicitado

	// 1. Validações básicas de integridade
	if (handleAddr == 0)
	{
		g_LastMemError = -109;
		return;
	}

	if (!globals->allocator.IsValidAddress(handleAddr, sizeof(uint32_t)))
	{
		g_LastMemError = -111;
		return;
	}

	uint32_t* masterPointer = globals->allocator.ToPointer<uint32_t>(handleAddr);
	uint32_t oldDataAddr = *masterPointer;

	// 2. Se o novo tamanho for 0, libertamos o bloco de dados mas mantemos o Master Pointer (vazio)
	if (newSize == 0)
	{
		if (oldDataAddr != 0 && globals->allocator.IsValidAddress(oldDataAddr, 1))
		{
			globals->allocator.Deallocate(globals->allocator.ToPointer<void>(oldDataAddr));
		}
		*masterPointer = 0; // Handle agora aponta para NULL
		g_LastMemError = 0;
		return;
	}

	// 3. Realocação Segura sob a rigidez do Carbon
	try {
		std::stringstream handleName;
		handleName << "CarbonLib Resized Handle Data [" << newSize << "]";
		
		// Aloca um novo bloco com o tamanho atualizado
		uint8_t* newDataBlock = globals->allocator.Allocate(handleName.str(), newSize);

		// Se existia um bloco antigo, copiamos o conteúdo anterior (até ao limite do menor tamanho)
		if (oldDataAddr != 0 && globals->allocator.IsValidAddress(oldDataAddr, 1))
		{
			size_t oldSize = 0;
			if (auto details = globals->allocator.GetDetails(oldDataAddr))
			{
				oldSize = details->GetSize();
			}
			
			size_t copySize = (oldSize < newSize) ? oldSize : newSize;
			if (copySize > 0)
			{
				std::memcpy(newDataBlock, globals->allocator.ToPointer<void>(oldDataAddr), copySize);
			}

			// Liberta o bloco de dados antigo de forma segura
			globals->allocator.Deallocate(globals->allocator.ToPointer<void>(oldDataAddr));
		}

		// Atualiza o Master Pointer para o novo bloco de dados.
		// O binário pré-Carbon nem repara na troca porque o endereço do Handle (r3) mantém-se idêntico!
		*masterPointer = globals->allocator.ToIntPtr(newDataBlock);
		g_LastMemError = 0;

	} catch (...) {
		// Se falhar por falta de memória no Darling/Linux, o Handle antigo permanece intacto
		g_LastMemError = -108;
	}
}

void CarbonLib_SetPtrSize(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetZone(CarbonLib::Globals* globals, MachineState* state)
{
	g_LastMemError = 0;
}

void CarbonLib_StackSpace(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SystemZone(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0; g_LastMemError = 0;
}

void CarbonLib_TempDisposeHandle(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_TempFreeMem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_TempHLock(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_TempHUnlock(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_TempMaxMem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_TempNewHandle(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_TempTopMem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_TopMem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_UnholdMemory(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_UnlockMemory(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

