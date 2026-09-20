//
// OSUtils.cpp
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
// Patches by Arucard1983, Copyright (C)2026

#include "Prototypes.h"
#include "CarbonLib.h"
#include "NotImplementedException.h"

using namespace CarbonLib;

void CarbonLib_Delay(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_Dequeue(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DTInstall(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_Enqueue(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_FlushCodeCacheRange(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetSysPPtr(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_InitUtil(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_IsMetric(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_MakeDataExecutable(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0; //Obsolete and can be returned by OK.
}

void CarbonLib_ReadLocation(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0; //Not needed.
}

void CarbonLib_SetA5(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "A5 world management is obsolete in Carbon.");
}

void CarbonLib_SetCurrentA5(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "A5 world management is obsolete in Carbon.");
}

void CarbonLib_SysEnvirons(CarbonLib::Globals* globals, MachineState* state)
{
	if (state->r3 < 1)
	{
		state->r3 = 0xea83;
		return;
	}
	
	if (state->r3 > 2)
	{
		state->r3 = 0xea82;
		return;
	}
	
	// mostly based on the output I get from running this in SheepShaver, I should probably
	// run that on hardware at some point
	SysEnvRec& record = *globals->allocator.ToPointer<SysEnvRec>(state->r4);
	record.environsVersion = static_cast<int16_t>(state->r3);
	record.machineType = 0x41;
	record.systemVersion = static_cast<int16_t>(globals->managers.Gestalt().GetValue("sysv"));
	record.processor = 3;
	record.hasFPU = false; // we don't have FPU support yet
	record.hasColorQD = true;
	record.keyBoardType = 9;
	record.atDrvrVersNum = 0;
	record.sysVRefNum = 0x80c3;
	state->r3 = 0;
}

void CarbonLib_TickCount(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_WriteLocation(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_WriteParam(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

