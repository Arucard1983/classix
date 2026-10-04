//
// Processes.cpp
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
#include "NotImplementedException.h"

// Definição de constantes de erro clássicas do Process Manager do Mac OS
enum {
	noErr = 0,
	paramErr = -50,
	procNotFound = -600
};

// Estrutura clássica do Mac OS para identificar processos
struct ProcessSerialNumber {
	uint32_t highLong;
	uint32_t lowLong;
};

// Estrutura simplificada de informação do processo para o GetProcessInformation
struct ProcessInfoRec {
	uint32_t cbSize;
	uint32_t processName; // Ponteiro (Address) para Str255
	ProcessSerialNumber processNumber;
	uint32_t processType;
	uint32_t processSignature;
	uint32_t processMode;
	uint32_t processLocation;
	uint32_t processSize;
	uint32_t processFreeMem;
	ProcessSerialNumber processLauncher;
	uint32_t processLaunchDate;
};

void CarbonLib_ExitToShell(CarbonLib::Globals* globals, MachineState* state)
{
	exit(0);
}

void CarbonLib_GetFrontProcess(CarbonLib::Globals* globals, MachineState* state)
{
	// Entrada: r3 = ponteiro para ProcessSerialNumber de saída
	uint32_t psnAddress = state->r3;

	if ((psnAddress & 0x3) != 0 || !globals->allocator.IsValidAddress(psnAddress, sizeof(ProcessSerialNumber)))
	{
		state->r3 = paramErr;
		return;
	}

	ProcessSerialNumber* psn = globals->allocator.ToPointer<ProcessSerialNumber>(psnAddress);
	
	// Define o processo atual/front como um identificador estático válido (ex: 0, 1)
	psn->highLong = 0;
	psn->lowLong = 1;

	state->r3 = noErr;
}

void CarbonLib_GetNextProcess(CarbonLib::Globals* globals, MachineState* state)
{
	// Entrada: r3 = ponteiro para ProcessSerialNumber (PSN atual)
	// Saída: altera o PSN para o próximo processo da lista do OS
	uint32_t psnAddress = state->r3;

	if ((psnAddress & 0x3) != 0 || !globals->allocator.IsValidAddress(psnAddress, sizeof(ProcessSerialNumber)))
	{
		state->r3 = paramErr;
		return;
	}

	ProcessSerialNumber* psn = globals->allocator.ToPointer<ProcessSerialNumber>(psnAddress);

	// Logica de iteração simulada: se passarem o processo 1, não há mais processos
	if (psn->lowLong == 1)
	{
		state->r3 = procNotFound; // Termina a enumeração de processos
	}
	else
	{
		// Se pedirem o processo "kNoProcess" (0,0), devolvemos o nosso processo principal
		psn->highLong = 0;
		psn->lowLong = 1;
		state->r3 = noErr;
	}
}

void CarbonLib_GetProcessInformation(CarbonLib::Globals* globals, MachineState* state)
{
	// Entrada: r3 = ponteiro para ProcessSerialNumber, r4 = ponteiro para ProcessInfoRec
	uint32_t psnAddress = state->r3;
	uint32_t infoAddress = state->r4;

	if (!globals->allocator.IsValidAddress(psnAddress, sizeof(ProcessSerialNumber)) ||
		!globals->allocator.IsValidAddress(infoAddress, sizeof(ProcessInfoRec)))
	{
		state->r3 = paramErr;
		return;
	}

	ProcessSerialNumber* psn = globals->allocator.ToPointer<ProcessSerialNumber>(psnAddress);
	ProcessInfoRec* info = globals->allocator.ToPointer<ProcessInfoRec>(infoAddress);

	// Validar se estão a pedir informações sobre o nosso processo emulado
	if (psn->lowLong != 1)
	{
		state->r3 = procNotFound;
		return;
	}

	// Preencher o stub de informação (evitando falhas se a app ler os campos)
	info->processNumber.highLong = 0;
	info->processNumber.lowLong = 1;
	info->processType = 0x4150504C;      // 'APPL' em Big Endian se necessário (ajustar conforme endianness do emulador)
	info->processSignature = 0x3F3F3F3F; // '????'
	info->processMode = 0;
	
	state->r3 = noErr;
}

void CarbonLib_LaunchApplication(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = paramErr; // InterfaceLib/Carbon fallback stub
}

void CarbonLib_LaunchControlPanel(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = paramErr;
}

void CarbonLib_LaunchDeskAccessory(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = paramErr;
}

void CarbonLib_SameProcess(CarbonLib::Globals* globals, MachineState* state)
{
	// Entrada: r3 = ponteiro para PSN 1, r4 = ponteiro para PSN 2
	// Saída: r5 = ponteiro para Boolean (resultado)
	uint32_t psn1Address = state->r3;
	uint32_t psn2Address = state->r4;
	uint32_t resultAddress = state->r5;

	if (!globals->allocator.IsValidAddress(psn1Address, sizeof(ProcessSerialNumber)) ||
		!globals->allocator.IsValidAddress(psn2Address, sizeof(ProcessSerialNumber)) ||
		!globals->allocator.IsValidAddress(resultAddress, sizeof(uint8_t)))
	{
		state->r3 = paramErr;
		return;
	}

	ProcessSerialNumber* psn1 = globals->allocator.ToPointer<ProcessSerialNumber>(psn1Address);
	ProcessSerialNumber* psn2 = globals->allocator.ToPointer<ProcessSerialNumber>(psn2Address);
	uint8_t* result = globals->allocator.ToPointer<uint8_t>(resultAddress);

	// Compara as duas estruturas
	if (psn1->highLong == psn2->highLong && psn1->lowLong == psn2->lowLong)
	{
		*result = 1; // true
	}
	else
	{
		*result = 0; // false
	}

	state->r3 = noErr;
}

void CarbonLib_SetFrontProcess(CarbonLib::Globals* globals, MachineState* state)
{
	
	// Stub seguro: como já somos a app principal, apenas retornamos sucesso
	state->r3 = noErr;
}

void CarbonLib_WakeUpProcess(CarbonLib::Globals* globals, MachineState* state)
{
	// Stub seguro: o processo emulado já está ativo
	state->r3 = noErr;
}

