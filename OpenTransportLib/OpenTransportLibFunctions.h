//
// OpenTransportLibFunctions.h
// Classix
//
// Copyright (C) 2012 Félix Cloutier
//
// Copyright (C) 2026 Arucard1983
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

#ifndef __Classix__OpenTransportLibFunctions__
#define __Classix__OpenTransportLibFunctions__

namespace PPCVM
{
	struct MachineState;
}

namespace OpenTransportLib
{
	struct Globals;
}

extern "C"
{
	void OpenTransportLib_InitOpenTransport(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_InitOpenTransportUtilities(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_CloseOpenTransport(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTOpenEndpoint(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTAsyncOpenEndpoint(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTCloseProvider(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTBind(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTUnbind(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTConnect(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTRcvConnect(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTAccept(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTSnd(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTRcv(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTSndUData(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTRcvUData(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTInstallNotifier(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTRemoveNotifier(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTSetAsynchronous(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTSetSynchronous(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTLook(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
	void OpenTransportLib_OTUnknown(OpenTransportLib::Globals* globals, PPCVM::MachineState* state);
}

#endif /* defined(__Classix__OpenTransportLibFunctions__) */
