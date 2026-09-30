//
// Gestalt.cpp
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
#include "NotImplementedException.h"
#include "CarbonLib.h"

// see http://developer.apple.com/library/mac/#documentation/Carbon/reference/Gestalt_Manager/Reference/reference.html

template<char A1, char A2, char A3, char A4>
struct CharCode
{
	static constexpr uint32_t Value = (A1 << 24) | (A2 << 16) | (A3 << 8) | A4;
};

void CarbonLib_DeleteGestaltValue(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -5553; // gestaltResourceErr
}

void CarbonLib_Gestalt(CarbonLib::Globals* globals, MachineState* state)
{
	// Check output pointer
	uint32_t outputAddress = state->r4;

	// 1. Check pointer alignment (should be multiples of four)
	if ((outputAddress & 0x3) != 0)
	{
		state->r3 = -50; // Pointer failsafe
		return;
	}

	// 2. Is the pointer adress a valid loclation ?
	if (!globals->allocator.IsValidAddress(outputAddress, sizeof(Common::SInt32)))
	{
		state->r3 = -5554; // Return adress Error
		return;
	}

	// Safety conversion after validation 
	Common::SInt32& output = *globals->allocator.ToPointer<Common::SInt32>(outputAddress);
	int32_t result;

	// 3. Running Selector
	if (globals->managers.Gestalt().GetValue(state->r3, result))
	{
		output = result;
		state->r3 = 0; // OK (0)
	}
	else
	{
		// If the selector do not exist (ex: old 'sysv' requesting an unsupported version)
		output = 0;
		state->r3 = -5551; Undefined
	}
}

void CarbonLib_NewGestalt(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -5553; // gestaltResourceErr
}

void CarbonLib_NewGestaltValue(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -5553; // gestaltResourceErr
}

void CarbonLib_ReplaceGestalt(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -5553; // gestaltResourceErr
}

void CarbonLib_ReplaceGestaltValue(CarbonLib::Globals* globals, MachineState* state)
{
	tate->r3 = -5553; // gestaltResourceErr
}

void CarbonLib_SetGestaltValue(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -5553; // gestaltResourceErr
}

