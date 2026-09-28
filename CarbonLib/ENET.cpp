//
// ENET.cpp
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
#include "NotSupportedException.h"

void CarbonLib_EAddMulti(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Network Adapter management is forbidden!");
}

void CarbonLib_EAttachPH(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Network Adapter management is forbidden!");
}

void CarbonLib_EDelMulti(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Network Adapter management is forbidden!");
}

void CarbonLib_EDetachPH(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Network Adapter management is forbidden!");
}

void CarbonLib_EGetInfo(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Network Adapter management is forbidden!");
}

void CarbonLib_ERdCancel(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Network Adapter management is forbidden!");
}

void CarbonLib_ERead(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Network Adapter management is forbidden!");
}

void CarbonLib_ESetGeneral(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Network Adapter management is forbidden!");
}

void CarbonLib_EWrite(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Network Adapter management is forbidden!");
}

