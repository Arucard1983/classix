//
// Power.cpp
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

void CarbonLib_AOff(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_AOn(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_AOnIgnoreModem(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_BatteryStatus(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_BOff(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_BOn(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_DisableIdle(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_DisableWUTime(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_EnableIdle(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_GetCPUSpeed(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 400; //Bogus G3 Speed
}

void CarbonLib_GetWUTime(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_IdleUpdate(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_ModemStatus(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_SetWUTime(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_SleepQInstall(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_SleepQRemove(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

