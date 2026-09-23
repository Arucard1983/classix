//
// OpenTransportLibFunctions.cpp
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

#include "OpenTransportLib.h"
#include "OpenTransportLibFunctions.h"

extern "C" const char* LibraryCodeSymbolNames[] = {
	"InitOpenTransport",
	"InitOpenTransportUtilities",
	"CloseOpenTransport",
	"OTOpenEndpoint",
	"OTAsyncOpenEndpoint",
	"OTCloseProvider",
	"OTBind",
	"OTUnbind",
	"OTConnect",
	"OTRcvConnect",
	"OTAccept",
	"OTSnd",
	"OTRcv",
	"OTSndUData",
	"OTRcvUData",
	"OTInstallNotifier",
	"OTRemoveNotifier",
	"OTSetAsynchronous",
	"OTSetSynchronous",
	"OTLook",
	"OTInetStringToAddress",
	"OTInetAddressToName",
	"OTCreateConfiguration",
	"OTRegisterAsClient",
	"OTUnregisterAsClient",
	"OTSndDisconnect",
	"OTRcvDisconnect",
	"OTSndOrderlyDisconnect",
	"OTRcvOrderlyDisconnect",
	"OTRemoveNotifier",
	"OTSetSynchronous",
	"OTAckSends",
	"OTDontAckSends",
	"OTIoctl",
	"OTGetEndpointState",
	"OTSync",
	"OTUnknown",
	nullptr
};

extern "C" const char* LibraryDataSymbolNames[] = {
	nullptr
};
