//
// Patches.cpp
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
// Copyright (C) 2026 Arucard1983


#include "Prototypes.h"
#include "NotSupportedException.h"

void CarbonLib_GetOSTrapAddress(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "Traps is a legacy feature of 68k and unsafe, therefore is not supported!");
}

void CarbonLib_GetToolboxTrapAddress(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "Traps is a legacy feature of 68k and unsafe, therefore is not supported!");
}

void CarbonLib_GetToolTrapAddress(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "Traps is a legacy feature of 68k and unsafe, therefore is not supported!");
}

void CarbonLib_GetTrapVector(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "Traps is a legacy feature of 68k and unsafe, therefore is not supported!");
}

void CarbonLib_NGetTrapAddress(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "Traps is a legacy feature of 68k and unsafe, therefore is not supported!");
}

void CarbonLib_NSetTrapAddress(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "Traps is a legacy feature of 68k and unsafe, therefore is not supported!");
}

void CarbonLib_SetOSTrapAddress(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "Traps is a legacy feature of 68k and unsafe, therefore is not supported!");
}

void CarbonLib_SetToolboxTrapAddress(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "Traps is a legacy feature of 68k and unsafe, therefore is not supported!");
}

void CarbonLib_SetToolTrapAddress(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "Traps is a legacy feature of 68k and unsafe, therefore is not supported!");
}

