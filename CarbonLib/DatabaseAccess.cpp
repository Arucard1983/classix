//
// DatabaseAccess.cpp
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

void CarbonLib_DBBreak(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50;
}

void CarbonLib_DBDisposeQuery(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_DBEnd(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_DBExec(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50;
}

void CarbonLib_DBGetConnInfo(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50;
}

void CarbonLib_DBGetErr(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50;
}

void CarbonLib_DBGetItem(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50;
}

void CarbonLib_DBGetNewQuery(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50;
}

void CarbonLib_DBGetQueryResults(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50;
}

void CarbonLib_DBGetResultHandler(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50;
}

void CarbonLib_DBGetSessionNum(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50;
}

void CarbonLib_DBInit(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_DBInstallResultHandler(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50;
}

void CarbonLib_DBKill(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_DBRemoveResultHandler(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50;
}

void CarbonLib_DBResultsToText(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50;
}

void CarbonLib_DBSend(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50;
}

void CarbonLib_DBSendItem(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50;
}

void CarbonLib_DBStartQuery(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50;
}

void CarbonLib_DBState(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50;
}

void CarbonLib_DBUnGetItem(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50;
}

void CarbonLib_InitDBPack(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

