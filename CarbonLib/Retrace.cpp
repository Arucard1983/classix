//
// Retrace.cpp
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

void CarbonLib_AttachVBL(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -3;
}

void CarbonLib_DoVBLTask(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -3;
}

void CarbonLib_GetVBLQHdr(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -3;
}

void CarbonLib_SlotVInstall(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -3;
}

void CarbonLib_SlotVRemove(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -3;
}

void CarbonLib_VInstall(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -3;
}

void CarbonLib_VRemove(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -3;
}

