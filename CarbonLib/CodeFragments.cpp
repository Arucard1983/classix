//
// CodeFragments.cpp
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

#include "Prototypes.h"
#include "CarbonLib.h"

// Constantes de erro clássicas do CFM
enum {
	noErr = 0,
	paramErr = -50,
	fragConnectionIDNotFoundErr = -2801,
	fragSymbolNotFoundErr = -2802,
	fragLibNotFoundErr = -2804
};

// No CFM do Mac OS, a ConnectionID é o identificador opaco da biblioteca carregada
typedef uint32_t ConnectionID;

void CarbonLib_CloseConnection(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = ConnectionID *connID
	uint32_t connIDAddress = state->r3;

	if ((connIDAddress & 0x3) != 0 || !globals->allocator.IsValidAddress(connIDAddress, sizeof(ConnectionID)))
	{
		state->r3 = paramErr;
		return;
	}

	// Notificar o ClassiXCore para decrementar o refcount do fragmento
	state->r3 = noErr;
}

void CarbonLib_CountSymbols(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = ConnectionID connID
	// r4 = uint32_t *symCount (Saída)
	uint32_t symCountAddress = state->r4;

	if ((symCountAddress & 0x3) != 0 || !globals->allocator.IsValidAddress(symCountAddress, sizeof(uint32_t)))
	{
		state->r3 = paramErr;
		return;
	}

	uint32_t* symCount = globals->allocator.ToPointer<uint32_t>(symCountAddress);
	
	// Interrogar o ClassiXCore pelo número de símbolos do fragmento
	*symCount = 0; 
	state->r3 = noErr;
}

void CarbonLib_FindSymbol(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = ConnectionID connID
	// r4 = const Str255Param symbolName
	// r5 = Ptr *symAddr (Saída)
	// r6 = SymClass *symClass (Saída)
	
	uint32_t connID = state->r3;
	uint32_t symbolNameAddr = state->r4;
	uint32_t symAddrOutput = state->r5;

	if ((symAddrOutput & 0x3) != 0 || !globals->allocator.IsValidAddress(symAddrOutput, sizeof(uint32_t)))
	{
		state->r3 = paramErr;
		return;
	}

	// O ClassiXCore deve procurar o símbolo na tabela de exportação da 'connID' correspondente
	state->r3 = fragSymbolNotFoundErr;
}

void CarbonLib_GetDiskFragment(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = const FSSpec *fileSpec
	// r4 = uint32_t fileOffset
	// r5 = uint32_t length
	// r6 = const Str255Param fragName
	// r7 = LoadFlags flags
	// r8 = ConnectionID *connID (Saída)
	// r9 (passado via stack se necessário, ou registo adicional dependendo da convenção da ABI)
	
	state->r3 = fragLibNotFoundErr;
}

void CarbonLib_GetIndSymbol(CarbonLib::Globals* globals, MachineState* state)
{
	// Usado para introspeção de símbolos por índice
	state->r3 = fragSymbolNotFoundErr;
}

void CarbonLib_GetMemFragment(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = Ptr memAddress
	// r4 = uint32_t memLength
	// r5 = const Str255Param fragName
	// r6 = LoadFlags loadFlags
	// r7 = ConnectionID *connID (Saída)
	// r8 = Ptr *mainAddr (Saída)
	
	uint32_t connIDAddress = state->r7;

	if ((connIDAddress & 0x3) != 0 || !globals->allocator.IsValidAddress(connIDAddress, sizeof(ConnectionID)))
	{
		state->r3 = paramErr;
		return;
	}

	// Encaminhar os ponteiros de memória emulada para o parser de PEF do ClassiXCore
	state->r3 = fragLibNotFoundErr;
}

void CarbonLib_GetSharedLibrary(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = const Str255Param libName (Ponteiro Pascal String no espaço emulado)
	// r4 = OSType architecture
	// r5 = LoadFlags loadFlags
	// r6 = ConnectionID *connID (Saída)
	// r7 = Ptr *mainAddr (Saída)
	
	uint32_t libNameAddress = state->r3;
	uint32_t connIDAddress = state->r6;
	uint32_t mainAddrAddress = state->r7;

	if ((connIDAddress & 0x3) != 0 || !globals->allocator.IsValidAddress(connIDAddress, sizeof(ConnectionID)))
	{
		state->r3 = paramErr;
		return;
	}

	// TODO: Fazer a ponte com o ClassiXCore
	// Exemplo conceitual:
	// const char* libName = globals->allocator.ToPascalString(libNameAddress);
	// state->r3 = globals->core.GetCFMManager().LoadSharedLibrary(libName, ...);
	
	state->r3 = fragLibNotFoundErr;
}

