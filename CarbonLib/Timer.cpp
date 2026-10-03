//
// Timer.cpp
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

#include <chrono>
#include "CarbonLib.h"
#include "Prototypes.h"
#include "BigEndian.h"

using namespace std::chrono;

void CarbonLib_InstallTimeTask(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_InstallXTimeTask(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_InsTime(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_InsXTime(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_Microseconds(CarbonLib::Globals* globals, MachineState* state)
{
	 auto duration = high_resolution_clock::now().time_since_epoch();
    uint64_t micros = duration_cast<microseconds>(duration).count(); 
	*globals->allocator.ToPointer<Common::UInt64>(state->r3) = micros;
}

void CarbonLib_PrimeTime(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_PrimeTimeTask(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_RemoveTimeTask(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_RmvTime(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

