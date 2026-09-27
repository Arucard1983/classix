//
// SCSI.cpp
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

void CarbonLib_SCSIAction(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

void CarbonLib_SCSICmd(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

void CarbonLib_SCSIComplete(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

void CarbonLib_SCSIDeregisterBus(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

void CarbonLib_SCSIGet(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

void CarbonLib_SCSIKillXPT(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

void CarbonLib_SCSIMsgIn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

void CarbonLib_SCSIMsgOut(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

void CarbonLib_SCSIRBlind(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

void CarbonLib_SCSIRead(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

void CarbonLib_SCSIRegisterBus(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

void CarbonLib_SCSIReregisterBus(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

void CarbonLib_SCSIReset(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

void CarbonLib_SCSISelAtn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

void CarbonLib_SCSISelect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

void CarbonLib_SCSIStat(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

void CarbonLib_SCSIWBlind(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

void CarbonLib_SCSIWrite(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__,"Direct Hardware Access to SCSI devices is forbidden!");
}

