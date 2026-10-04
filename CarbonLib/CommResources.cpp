//
// CommResources.cpp
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
#include "CarbonLib.h"

// Erros clássicos da Communications Toolbox
enum {
	crmNoErr = 0,
	crmBadQueueErr = -4101
};

void CarbonLib_CRMFindCommunications(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = crmBadQueueErr;
}

void CarbonLib_CRMGet1IndResource(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_CRMGet1NamedResource(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_CRMGet1Resource(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_CRMGetCRMVersion(CarbonLib::Globals* globals, MachineState* state)
{
	// Devolve uma versão fictícia baixa (ex: 2 para Communications Toolbox 2.0)
	state->r3 = 2;
}

void CarbonLib_CRMGetHeader(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_CRMGetIndex(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = crmBadQueueErr; 
}

void CarbonLib_CRMGetIndResource(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_CRMGetIndToolName(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_CRMGetNamedResource(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_CRMGetResource(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_CRMGetToolNamedResource(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_CRMGetToolResource(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_CRMInstall(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = crmBadQueueErr;
}

void CarbonLib_CRMIsDriverOpen(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 de saída = 0 (false), o driver série/modem não está aberto
	state->r3 = 0;
}

void CarbonLib_CRMLocalToRealID(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_CRMParseCAPSResource(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = crmBadQueueErr;
}

void CarbonLib_CRMRealToLocalID(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_CRMReleaseResource(CarbonLib::Globals* globals, MachineState* state)
{
        state->r3 = crmNoErr;
}

void CarbonLib_CRMReleaseRF(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = crmNoErr;
}

void CarbonLib_CRMReleaseToolResource(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = crmNoErr;
}

void CarbonLib_CRMRemove(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = crmBadQueueErr;
}

void CarbonLib_CRMReserveRF(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = crmNoErr;
}

void CarbonLib_CRMSearch(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = crmBadQueueErr;
}

void CarbonLib_InitCRM(CarbonLib::Globals* globals, MachineState* state)
{
	// Sucesso simulado: diz à app que o gestor de comunicações iniciou bem
	state->r3 = crmNoErr;
}

