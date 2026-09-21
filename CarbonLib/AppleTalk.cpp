//
// AppleTalk.cpp
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

void CarbonLib_AFPCommand(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_ASPAbortOS(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_ASPCloseAll(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_ASPCloseSession(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_ASPGetParms(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_ASPGetStatus(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_ASPOpenSession(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_ASPUserCommand(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_ASPUserWrite(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_ATEvent(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_ATPKillAllGetReq(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_ATPLoad(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -98; //To stub a safe Not Avaliable Network using Apple Talk without crash the program
}

void CarbonLib_ATPreFlightEvent(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_ATPUnload(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_BuildBDS(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_BuildDDPwds(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_BuildLAPwds(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_GetBridgeAddress(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_GetLocalZones(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_GetMyZone(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_GetNodeAddress(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_GetZoneList(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_IsATPOpen(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_IsMPPOpen(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_LAPAddATQ(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_LAPRmvATQ(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_MPPOpen(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -98; //To stub a safe Not Avaliable Network using Apple Talk without crash the program
}

void CarbonLib_NBPExtract(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_NBPSetEntity(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_NBPSetNTE(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_OpenXPP(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_PAddResponse(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_PATalkClosePrep(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_PAttachPH(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_PCloseATPSkt(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_PCloseSkt(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_PConfirmName(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_PDetachPH(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_PGetAppleTalkInfo(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -1022; //To explicity tell to the program that AppleTalk is not avaliable without crash the program.
}

void CarbonLib_PGetRequest(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_PKillGetReq(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_PKillNBP(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_PKillSendReq(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_PLookupName(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_PNSendRequest(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_POpenATPSkt(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_POpenSkt(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_PRegisterName(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_PRelRspCB(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_PRelTCB(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_PRemoveName(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_PSendRequest(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_PSendResponse(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_PSetSelfSend(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "AppleTalk not Supported!");
}

void CarbonLib_PWriteDDP(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

void CarbonLib_PWriteLAP(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = -50; //To stub a safe Not Avaliable Error to the program handle it safely 
}

