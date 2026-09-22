//
// OpenTransportLib.cpp
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

#include <dlfcn.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include <atomic>
#include <cstring>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>

#include "OpenTransportLib.h"
#include "OpenTransportLibFunctions.h"
#include "NotImplementedException.h"
#include "NotSupportedException.h"
#include "MachineState.h"
#include "BigEndian.h"
#include "Todo.h"

namespace OpenTransportLib
{
	struct EndpointContext
	{
		int fd;                      // File Discriptor of BSD socket from host
		bool isAsynchronous = false; // Lock status defined by OTSetAsynchronous
		uint32_t notifierUPP = 0;    // Callback adress of PowerPC VM 
		uint32_t contextPtr = 0;     // Guest context pointer from Mac OS 9
	};

    struct Globals
	{
		Common::Allocator& allocator;

        // Set the ficticious ID from Guest 
        std::map<uint32_t, EndpointContext> endpointMap;
		std::atomic<uint32_t> nextEndpointId;
		
		Globals(Common::Allocator& allocator)
		: allocator(allocator)
		{
		  //TODO: Eventual symbols will be placed here
		}
	};
}

using PPCVM::MachineState;
using namespace OpenTransportLib;


extern "C"
{
	Globals* LibraryLoad(Common::Allocator* allocator, OSEnvironment::Managers* managers)
	{
		return allocator->Allocate<Globals>("OpenTransportLib Globals", *allocator);
	}
	
	SymbolType LibraryLookup(Globals* globals, const char* name, void** result)
	{
		
		char functionName[50] = "OpenTransportLib_";
		char* end = stpncpy(functionName + 17, name, 34);
		if (*end != 0)
		{
			*result = nullptr;
			return SymbolNotFound;
		}
		
		if (void* symbol = dlsym(RTLD_SELF, functionName))
		{
			*result = symbol;
			return CodeSymbol;
		}
		
		*result = nullptr;
		return SymbolNotFound;
	}
	
	void LibraryUnload(Globals* globals)
	{
		//Close the open handles before closes
		for (auto const& [id, context] : globals->endpointMap)
		{
			close(context.fd);
		}
		globals->endpointMap.clear();
		globals->allocator.Deallocate(globals);
	}
	
	// Auxilliary structs for IP adress
    struct MacTNetbuf 
    {
     Common::UInt32 maxlen; // buffer max size
     Common::UInt32 len;    // current buffer size
     Common::UInt32 buf;    // virtual pointer from VM to data (where is InetAdress)
    };

    struct MacTBind 
    {
     MacTNetbuf addr;  // Address to bind 
     Common::UInt32 qlen; // conection queue size (servers only)
    };

struct MacTCall 
{
    MacTNetbuf addr;  // Target Adress 
    MacTNetbuf opt;   // Association options (Usually not used) 
    MacTNetbuf udata; // User data (Usually not used) 
    Common::UInt32 sequence;
};

struct MacInetAddressLayout 
{
    Common::UInt16 addressType; // Always 2 (AF_INET on Classic Mac OS)
    Common::UInt16 port;        // Network Port
    Common::UInt32 host;        // IP (32-bit)
};

struct MacTUnitData
{
    MacTNetbuf addr;  // Adress (Where reside InetAddressLayout)
    MacTNetbuf opt;   // Protocol Option (Generally ignored)
    MacTNetbuf udata; // User data short buffer (Payload)
};
	
	void OpenTransportLib_InitOpenTransport(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	  state->r3 = 0; //Mac OS 9 expects kOTNoError to enable OpenTransport
    }

	void OpenTransportLib_InitOpenTransportUtilities(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 state->r3 = 0;
    }
        
	void OpenTransportLib_CloseOpenTransport(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 for (auto const& [id, context] : globals->endpointMap)
		{
			close(context.fd);
		}
	 globals->endpointMap.clear();
	 state->r3 = 0;
    }
        
	void OpenTransportLib_OTOpenEndpoint(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
        uint32_t guestConfigPtr = state->r3; // Config sent by Mac OS 9
		uint32_t guestFlags = state->r4;     // Opening flags (usaully 0)
		uint32_t guestInfoPtr = state->r5;    // Optional pointers to return endpoint info
		uint32_t guestErrPtr = state->r6;     // Optional pointers for OSStatus (error)

		// By default, assume TCP. If the VM config is readed, validate if request UDP. 
		int socketType = SOCK_STREAM; 

		if (guestConfigPtr != 0)
		{
			// On OTConfigurationRef point to a structure that contains the string
			// of protocol ("tcp" or "udp"). The first characters will give the answer.
			// Note: Depending how ClassiX emulate 'OTCreateConfiguration', the guestConfigPtr 
			// can direct cast the pointer to a C string.	
			const char* configStr = globals->allocator.ToPointer<const char>(guestConfigPtr);
		
			if (configStr != nullptr)
			{
				// Find for "udp" on setup string sent by VM
				if (std::strstr(configStr, "udp") != nullptr || std::strstr(configStr, "UDP") != nullptr)
				{
					socketType = SOCK_DGRAM; // Setup the host to use UDP!
				}
			}
		}

		// Create the native socket on BSD ecosystem (Darling/Linux)
		int hostSocketFd = socket(AF_INET, socketType, 0);
		
		if (hostSocketFd < 0)
		{
			// If host fails, return NULL (0) em r3 e and report the error if requested
			if (guestErrPtr != 0)
			{
				auto* outErr = globals->allocator.ToPointer<Common::SInt32>(guestErrPtr);
				outErr->Set(-3211); // kOTSysError
			}
			state->r3 = 0; // EndpointRef null(fail)
			return;
		}

		// Always creates a unique 32 buts ID and registers the new contect on our global table
		uint32_t guestEndpointRef = globals->nextEndpointId++;
		
		EndpointContext context;
		context.fd = hostSocketFd;
		context.isAsynchronous = false; // Start as Synchronous by default
		context.notifierUPP = 0;
		context.contextPtr = 0;

		globals->endpointMap[guestEndpointRef] = context;

		// If the application requested info using TEndpointInfo, fill the basic metadata with Big Endian
		if (guestInfoPtr != 0)
		{
			struct MacTEndpointInfo {
				Common::UInt32 type;
				Common::UInt32 flags;
			};
			auto* info = globals->allocator.ToPointer<MacTEndpointInfo>(guestInfoPtr);
			info->type.Set(socketType == SOCK_STREAM ? 1 : 2); // 1 = TCP (Stream), 2 = UDP (Datagram)
			info->flags.Set(0);
		}

		// If everything done, set kOTNoError (0)
		if (guestErrPtr != 0)
		{
			auto* outErr = globals->allocator.ToPointer<Common::SInt32>(guestErrPtr);
			outErr->Set(0); // kOTNoError
		}

		// Return  the EndpointRef to r3 register
		state->r3 = guestEndpointRef;	 
    }
        
	void OpenTransportLib_OTAsyncOpenEndpoint(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 uint32_t guestConfigPtr = state->r3; // Setup(ex: "tcp" ou "udp")
		uint32_t guestFlags = state->r4;     // Flags
		uint32_t guestInfoPtr = state->r5;    // TEndpointInfo
		uint32_t guestProcPtr = state->r6;    // Notifier (PPCVM callback)
		uint32_t guestContext = state->r7;   // Context pointer to r7 register (PPC convention)

		// 1. Identify the same mode like sync mode.
		int socketType = SOCK_STREAM; 
		if (guestConfigPtr != 0)
		{
			const char* configStr = globals->allocator.ToPointer<const char>(guestConfigPtr);
			if (configStr != nullptr && (std::strstr(configStr, "udp") != nullptr || std::strstr(configStr, "UDP") != nullptr))
			{
				socketType = SOCK_DGRAM;
			}
		}

		// 2. Create the real socket on Darling/Linux
		int hostSocketFd = socket(AF_INET, socketType, 0);
		if (hostSocketFd < 0)
		{
			state->r3 = -3211; // kOTSysError (If nauive socket fails to create)
			return;
		}

		// 3. Register the async  Notifier received
		uint32_t guestEndpointRef = globals->nextEndpointId++;
		
		EndpointContext context;
		context.fd = hostSocketFd;
		context.isAsynchronous = true;    // APi async, set non-block mode
		context.notifierUPP = guestProcPtr; // Store VM callback 
		context.contextPtr = guestContext;   // Store context

		// Set the socket in non-locking mode if requested
		int flags = fcntl(hostSocketFd, F_GETFL, 0);
		fcntl(hostSocketFd, F_SETFL, flags | O_NONBLOCK);

		globals->endpointMap[guestEndpointRef] = context;

		// 4. Fill the information with Big Endian if needed
		if (guestInfoPtr != 0)
		{
			struct MacTEndpointInfo {
				Common::UInt32 type;
				Common::UInt32 flags;
			};
			auto* info = globals->allocator.ToPointer<MacTEndpointInfo>(guestInfoPtr);
			info->type.Set(socketType == SOCK_STREAM ? 1 : 2);
			info->flags.Set(0);
		}

		// 5. Assynchronous secret
		if (guestProcPtr != 0)
		{
			// Set the registers to prepare the Mac OS 9 calllback 
			state->r3 = guestContext;          // User context
			state->r4 = 0x1000;                // kOTOpenCompleteEvent (0x1000)
			state->r5 = 0;                     // kOTNoError (0)
			state->r6 = guestEndpointRef;      // EndpointRef cookie
			
		}

		state->r3 = 0; 

    }
        
	void OpenTransportLib_OTCloseProvider(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 uint32_t guestEndpointRef = state->r3; // O ID do endpoint é passado pela VM em r3
		
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it != globals->endpointMap.end())
		{
			// 1. Close the real file descriptor on Darling/Linux
			close(it->second.fd); 
			
			// 2. Remove the endpoint from emulator global table 
			globals->endpointMap.erase(it);
			
			// 3. Return kOTNoError (0) signals absolute success on Mac OS 9
			state->r3 = 0; 
		}
		else
		{
			// If the socket do not exist on our table
			state->r3 = -3201; // kOTBadProviderErr (-3201)
		}
    }
        
	void OpenTransportLib_OTBind(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	  // OTBind(EndpointRef ref, TBind* req, TBind* ret)
	    uint32_t guestEndpointRef = state->r3;
		uint32_t reqPtr = state->r4; // MacTBind Pointer (request)
		// state->r5 would be the 'ret' to retrieve the final atributed adress (optional)

		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = -3201; // kOTBadProviderErr
			return;
		}

		// If reqPtr is 0 (NULL), then OpenTransport will select a port an autimatic IP (INADDR_ANY)
		if (reqPtr == 0)
		{
			sockaddr_in hostAddr;
			std::memset(&hostAddr, 0, sizeof(hostAddr));
			hostAddr.sin_family = AF_INET;
			hostAddr.sin_port = htons(0);
			hostAddr.sin_addr.s_addr = htonl(INADDR_ANY);

			if (bind(it->second.fd, (struct sockaddr*)&hostAddr, sizeof(hostAddr)) < 0)
			{
				state->r3 = -3211; // kOTSysError
				return;
			}
			state->r3 = 0; // kOTNoError
			return;
		}

		// Tranlate the TBind structure ponter using the ClassiX allocator
		// Note: The exact method of ClassiX normally request acess to allocator using Globals
		const auto* guestBind = globals->allocator.ToPointer<MacTBind>(reqPtr);
		
		if (guestBind->addr.len.Get() == 0 || guestBind->addr.buf.Get() == 0)
		{
			// Without specified afressm it will assume automatic attribution.  
			sockaddr_in hostAddr;
			std::memset(&hostAddr, 0, sizeof(hostAddr));
			hostAddr.sin_family = AF_INET;
			if (bind(it->second.fd, (struct sockaddr*)&hostAddr, sizeof(hostAddr)) < 0) {
				state->r3 = -3211;
				return;
			}
			state->r3 = 0;
			return;
		}

		// Get the real InetAddress adress insude the VM virtual buffer
		const auto* macInetAddr = globals->allocator.ToPointer<MacInetAddressLayout>(guestBind->addr.buf.Get());

		// convert the Big Endian data from VM to the host native format
		sockaddr_in hostAddr;
		std::memset(&hostAddr, 0, sizeof(hostAddr));
		hostAddr.sin_family = AF_INET;
		hostAddr.sin_port = htons(macInetAddr->port.Get());
		hostAddr.sin_addr.s_addr = htonl(macInetAddr->host.Get());

		// Execute the native bind from BSD Sockets on host
		if (bind(it->second.fd, (struct sockaddr*)&hostAddr, sizeof(hostAddr)) < 0)
		{
			if (errno == EADDRINUSE) {
				state->r3 = -3213; // kOTAddressBusyErr
			} else {
				state->r3 = -3211; // kOTSysError
			}
			return;
		}

		state->r3 = 0; // kOTNoError
    }
        
	void OpenTransportLib_OTUnbind(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 state->r3 = 0;
    }
        
	void OpenTransportLib_OTConnect(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
     // OTConnect(EndpointRef ref, TCall* sndCall, TCall* rcvCall)
	    uint32_t guestEndpointRef = state->r3;
		uint32_t sndCallPtr = state->r4; // MacTCall pointer (remote data destination )
		// state->r5 would be the rcvCall to obtain answer details (very rare for basic TCP)

		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = -3201; // kOTBadProviderErr
			return;
		}

		if (sndCallPtr == 0)
		{
			state->r3 = -3206; // kOTBadArgsErr (Cannot connect to nowhere)
			return;
		}

		// Translate the TCall structure sent by VM
		const auto* guestCall = globals->allocator.ToPointer<MacTCall>(sndCallPtr);

		if (guestCall->addr.len.Get() == 0 || guestCall->addr.buf.Get() == 0)
		{
			state->r3 = -3206; // kOTBadArgsErr
			return;
		}

		// Extract the IP e and the destination Port from the remote from PowerPC VM
		const auto* macInetAddr = globals->allocator.ToPointer<MacInetAddressLayout>(guestCall->addr.buf.Get());

		sockaddr_in hostAddr;
		std::memset(&hostAddr, 0, sizeof(hostAddr));
		hostAddr.sin_family = AF_INET;
		hostAddr.sin_port = htons(macInetAddr->port.Get());
		hostAddr.sin_addr.s_addr = htonl(macInetAddr->host.Get());

		// Execute the remote conection try
		int result = connect(it->second.fd, (struct sockaddr*)&hostAddr, sizeof(hostAddr));
		
		if (result < 0)
		{
			// If the socket is set to assynchronous mode (not locked), the connect retorns EINPROGRESS
			if (errno == EINPROGRESS)
			{
				state->r3 = -3152; // kOTNoDataErr / InProgress from OpenTransport
			}
			else if (errno == ECONNREFUSED)
			{
				state->r3 = -3212; // kOTConnectionRefusedErr
			}
			else
			{
				state->r3 = -3211; // kOTSysError
			}
			return;
		}

		state->r3 = 0; //Synchronous connection done!
    }
        
	void OpenTransportLib_OTRcvConnect(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 state->r3 = 0;
    }
        
	void OpenTransportLib_OTAccept(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 uint32_t serverEp = state->r3; // Server socket waiting
		uint32_t resEp = state->r4;    // Socket that will receive the connection

		auto itServer = globals->endpointMap.find(serverEp);
		if (itServer == globals->endpointMap.end())
		{
			state->r3 = -3201; // kOTBadProviderErr
			return;
		}

		// Execute the BSD accept command on host
		sockaddr_in clientAddr;
		socklen_t clientLen = sizeof(clientAddr);
		int clientFd = accept(itServer->second.fd, (struct sockaddr*)&clientAddr, &clientLen);

		if (clientFd < 0)
		{
			state->r3 = -3211; // kOTSysError
			return;
		}

		// If the application reuse the endpopint
		EndpointContext clientContext;
		clientContext.fd = clientFd;
		clientContext.isAsynchronous = itServer->second.isAsynchronous;
		clientContext.notifierUPP = itServer->second.notifierUPP;
		clientContext.contextPtr = itServer->second.contextPtr;

		// Associate the new file descriptor to the new ID
		globals->endpointMap[resEp] = clientContext;

		state->r3 = 0; // kOTNoError
    }
        
	void OpenTransportLib_OTSnd(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
        uint32_t guestEndpointRef = state->r3;
		uint32_t guestBufPtr = state->r4;      // virtual pointer from VM for the data to send
		size_t nbytes = state->r5;             // Needed quantity of bytes
		uint32_t otFlags = state->r6;          // OpenTransport flags(ex: OTEOR, OTMG)

		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = -3201; // kOTBadProviderErr (-3201)
			return;
		}

		if (guestBufPtr == 0 && nbytes > 0)
		{
			state->r3 = -3206; // kOTBadArgsErr (-3206)
			return;
		}

		// Translate a virtual memory pointer from  VM a legit real host pointer
		const void* hostBuffer = globals->allocator.ToPointer<const void>(guestBufPtr);

		// Flags mapping: OOpenTransport had additional flags lie kOTMoreData (0x0001).
		// For normal host TCP BSD it should be set flag to 0,
		// unless you need to support Urgent/Out-of-Band (MSG_OOB).
		int hostFlags = 0;

		// Run the native BSD Sockets from libSystem/Darling sent function
		ssize_t bytesSent = send(it->second.fd, hostBuffer, nbytes, hostFlags);

		if (bytesSent < 0)
		{
			// If the socket is on assynchronous mode (not lock) and system buffer is full
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				state->r3 = -3162; // kOTFlowErr (-3162) -> Tell the application to try again later
			}
			else if (errno == EPIPE || errno == ECONNRESET)
			{
				state->r3 = -3215; // kOTConnectionClosedErr (-3215)
			}
			else
			{
				state->r3 = -3211; // kOTSysError (-3211)
			}
			return;
		}

		// Sucess: Return the number of bytes sent (stored in r3, by PPC standards)
		state->r3 = static_cast<uint32_t>(bytesSent);	 
    }
        
	void OpenTransportLib_OTRcv(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 	uint32_t guestEndpointRef = state->r3;
		uint32_t guestBufPtr = state->r4;      // Virtual pointer from  VM to store received data 
		size_t nbytes = state->r5;             // Guest maximum buffer size
		uint32_t guestFlagsPtr = state->r6;    // Virtual pointer to the VM store the exit flags

		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = -3201; // kOTBadProviderErr (-3201)
			return;
		}

		if (guestBufPtr == 0 && nbytes > 0)
		{
			state->r3 = -3206; // kOTBadArgsErr (-3206)
			return;
		}

		// Translate the writing buffer pointer to the host
		void* hostBuffer = globals->allocator.ToPointer<void>(guestBufPtr);

		// Execute the native BSD receive function from host data
		ssize_t bytesReceived = recv(it->second.fd, hostBuffer, nbytes, 0);

		if (bytesReceived < 0)
		{
			// If the socket is not locked and without data queue form host
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				state->r3 = -3162; // kOTNoDataErr (-3162)
			}
			else if (errno == ECONNRESET)
			{
				state->r3 = -3215; // kOTConnectionClosedErr (-3215)
			}
			else
			{
				state->r3 = -3211; // kOTSysError (-3211)
			}
			return;
		}
		else if (bytesReceived == 0)
		{
			// on BSD Sockets, recv returning 0 means the remote close the connection correctly
			state->r3 = -3215; // kOTConnectionClosedErr (-3215)
			return;
		}

		// If an application received a flag when receive data, it should be reset (0)
		if (guestFlagsPtr != 0)
		{
			auto* outFlags = globals->allocator.ToPointer<Common::UInt32>(guestFlagsPtr);
			*outFlags = 0; // No additional flags (like kOTMoreData) on this basic read
		}

		// Sucess: Return the number of bytes really read and stored on VM memory
		state->r3 = static_cast<uint32_t>(bytesReceived);
    }
        
	void OpenTransportLib_OTRcvUData(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
        uint32_t guestEndpointRef = state->r3;
		uint32_t guestUnitDataPtr = state->r4; // MacTUnitData structure given by VM to fill up 
		uint32_t guestFlagsPtr = state->r5;    // Optional flag for the output

		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = -3201; // kOTBadProviderErr
			return;
		}

		if (guestUnitDataPtr == 0)
		{
			state->r3 = -3206; // kOTBadArgsErr
			return;
		}

		// Translate the TUnitData structure where we write the answers.
		auto* guestUnitData = globals->allocator.ToPointer<MacTUnitData>(guestUnitDataPtr);

		// 1. Set buffers to capture the package origin (IP/Port from who was sent)
		sockaddr_in srcHostAddr;
		socklen_t addrLen = sizeof(srcHostAddr);
		std::memset(&srcHostAddr, 0, sizeof(srcHostAddr));

		// 2. Validate the space that the VM give us to store the short data (Payload)
		uint32_t maxPayloadLen = guestUnitData->udata.maxlen.Get();
		uint32_t payloadBufPtr = guestUnitData->udata.buf.Get();

		if (maxPayloadLen == 0 || payloadBufPtr == 0)
		{
			state->r3 = -3206; // kOTBadArgsErr
			return;
		}
		void* hostPayloadBuffer = globals->allocator.ToPointer<void>(payloadBufPtr);

		// 3. Run the native UDP receive function
		ssize_t bytesReceived = recvfrom(it->second.fd, hostPayloadBuffer, maxPayloadLen, 0,
		                                 (struct sockaddr*)&srcHostAddr, &addrLen);

		if (bytesReceived < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				state->r3 = -3162; // kOTNoDataErr
			} else {
				state->r3 = -3211; // kOTSysError
			}
			return;
		}

		// 4. Fill the Host Adress back to VM memory(if it was allocated space)
		uint32_t addrBufPtr = guestUnitData->addr.buf.Get();
		uint32_t maxAddrLen = guestUnitData->addr.maxlen.Get();
		
		if (addrBufPtr != 0 && maxAddrLen >= sizeof(MacInetAddressLayout))
		{
			auto* macInetAddr = globals->allocator.ToPointer<MacInetAddressLayout>(addrBufPtr);
			
			// Write the data with the automatic inversion garanteded for Big Endian via .Set()!
			macInetAddr->addressType.Set(2); // AF_INET no Classic Mac OS
			macInetAddr->port.Set(ntohs(srcHostAddr.sin_port));
			macInetAddr->host.Set(ntohl(srcHostAddr.sin_addr.s_addr));
			
			guestUnitData->addr.len.Set(sizeof(MacInetAddressLayout));
		}

		// 5. Update the TUnitData data structures from VM
		guestUnitData->udata.len.Set(static_cast<uint32_t>(bytesReceived));
		guestUnitData->opt.len.Set(0); // No adiitonal options mapped

		// Clean the flags if the pointer exists 
		if (guestFlagsPtr != 0)
		{
			auto* outFlags = globals->allocator.ToPointer<Common::UInt32>(guestFlagsPtr);
			*outFlags = 0;
		}

		state->r3 = 0; // kOTNoError (UDP success also return 0)
	 
    }
        
	void OpenTransportLib_OTSndUData(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	    uint32_t guestEndpointRef = state->r3;
		uint32_t guestUnitDataPtr = state->r4; // VM virtual pointer to the MacTUnitData structure

		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = -3201; // kOTBadProviderErr
			return;
		}

		if (guestUnitDataPtr == 0)
		{
			state->r3 = -3206; // kOTBadArgsErr
			return;
		}

		// Translate the TUnitData structe from VM virtual memory
		const auto* guestUnitData = globals->allocator.ToPointer<MacTUnitData>(guestUnitDataPtr);

		// 1. Extract and convert the Host Adress (InetAddress)
		if (guestUnitData->addr.len.Get() == 0 || guestUnitData->addr.buf.Get() == 0)
		{
			state->r3 = -3206; // kOTBadArgsErr
			return;
		}
		const auto* macInetAddr = globals->allocator.ToPointer<MacInetAddressLayout>(guestUnitData->addr.buf.Get());

		sockaddr_in destHostAddr;
		std::memset(&destHostAddr, 0, sizeof(destHostAddr));
		destHostAddr.sin_family = AF_INET;
		destHostAddr.sin_port = htons(macInetAddr->port.Get());
		destHostAddr.sin_addr.s_addr = htonl(macInetAddr->host.Get());

		// 2. Extract the  Payload (the UDP short data)
		uint32_t payloadLen = guestUnitData->udata.len.Get();
		uint32_t payloadBufPtr = guestUnitData->udata.buf.Get();

		if (payloadLen == 0 || payloadBufPtr == 0)
		{
			state->r3 = 0; // Sent an empty UDP is valid on OT
			return;
		}
		const void* hostPayloadBuffer = globals->allocator.ToPointer<const void>(payloadBufPtr);

		// 3. Run the native host UDP command
		ssize_t bytesSent = sendto(it->second.fd, hostPayloadBuffer, payloadLen, 0,
		                           (struct sockaddr*)&destHostAddr, sizeof(destHostAddr));

		if (bytesSent < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				state->r3 = -3162; // kOTFlowErr
			} else {
				state->r3 = -3211; // kOTSysError
			}
			return;
		}

		state->r3 = 0; // kOTNoError (Note: OTSndUData return 0 in case of sucess, not the number of bytes!)

    }
        
	void OpenTransportLib_OTInstallNotifier(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 uint32_t guestEndpointRef = state->r3;
		uint32_t notifierUPP = state->r4; // callback adress pointer
		uint32_t contextPtr = state->r5;  // callback argument

		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it != globals->endpointMap.end())
		{
			// Store the VM contect maps 
			it->second.notifierUPP = notifierUPP;
			it->second.contextPtr = contextPtr;
			state->r3 = 0; // kOTNoError
		}
		else
		{
			state->r3 = -3201; // kOTBadProviderErr
		}
    }
        
	void OpenTransportLib_OTRemoveNotifier(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	  uint32_t guestEndpointRef = state->r3;

		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it != globals->endpointMap.end())
		{
			// Clean the safely callback pointers
			it->second.notifierUPP = 0;
			it->second.contextPtr = 0;
			state->r3 = 0; // kOTNoError
		}
		else
		{
			state->r3 = -3201; // kOTBadProviderErr
		}
     }
        
	void OpenTransportLib_OTSetAsynchronous(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 uint32_t guestEndpointRef = state->r3;

		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it != globals->endpointMap.end())
		{
			it->second.isAsynchronous = true;
			
			//  Warning: Activaye the mode non-locking on native socket on the host Linux/Darling
			int flags = fcntl(it->second.fd, F_GETFL, 0);
			fcntl(it->second.fd, F_SETFL, flags | O_NONBLOCK);
			
			state->r3 = 0; // kOTNoError
		}
		else
		{
			state->r3 = -3201;
		}
    }
        
	void OpenTransportLib_OTSetSynchronous(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	  uint32_t guestEndpointRef = state->r3;

		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it != globals->endpointMap.end())
		{
			it->second.isAsynchronous = false;
			
			// Remove the non-locking status, returning the socket to the sync default
			int flags = fcntl(it->second.fd, F_GETFL, 0);
			fcntl(it->second.fd, F_SETFL, flags & ~O_NONBLOCK);
			
			state->r3 = 0; // kOTNoError
		}
		else
		{
			state->r3 = -3201;
		}
    }

	 void OpenTransportLib_OTLook(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3; // O identificador passado pela VM

		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = -3201; // kOTBadProviderErr
			return;
		}

		int hostFd = it->second.fd;

		// Configure the Linux poll to read the current descriptor state wthout blocking (timeout 0)
		pollfd pfd;
		pfd.fd = hostFd;
		pfd.events = POLLIN | POLLOUT | POLLHUP | POLLERR;
		pfd.revents = 0;

		int pollResult = poll(&pfd, 1, 0);

		if (pollResult < 0)
		{
			state->r3 = 0; // If some error happens on host poll, it is assumed no pending events (T_NOTHING)
			return;
		}

		// Tf the descriptor is ready but without changes of queue
		if (pollResult == 0)
		{
			state->r3 = 0; // 0 is T_NOTHING on OpenTransport (No active event)
			return;
		}
		
		// 1. Critical errors and sudden remote disconnecttion
		if (pfd.revents & (POLLHUP | POLLERR))
		{
			state->r3 = 0x0008; // 0x0008 is T_DISCONNECT on Mac OS 9
			return;
		}

		// 2. Pending reading data on network stack
		if (pfd.revents & POLLIN)
		{
			// Technical note:  On TCP sockets in server mode, POLLIN also indicates a pending connection 
			// on queue ready to be accepted
			// OpenTransport define: T_LISTEN to servers and T_DATA to clients.
			
			// As simple heurist, the socket is always waiting to obtain data
			// or assume kOTDataEvent / T_DATA (0x0001).
			state->r3 = 0x0001; // 0x0001 is T_DATA (Normal data arrived)
			return;
		}

		// 3. Writing buffer was released libertado (Ready to sent more data)
		if (pfd.revents & POLLOUT)
		{
			// If the application excedded the buffer size and try again with kOTFlowErr (EAGAIN),
			// Then OTLook should return T_GODATA to warn that can transmit again 
			state->r3 = 0x0004; // 0x0004 is T_GODATA on OpenTransport
			return;
		}

		state->r3 = 0; // Fallback for T_NOTHING
	}

    // OTInetStringToAddress(InetInterfaceRef ref, char* str, InetAddress* addr)
	void OpenTransportLib_OTInetStringToAddress(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestInterfaceRef = state->r3; // Nirmally ignored (kDefaultInetInterface)
		uint32_t guestStrPtr = state->r4;       // Virtual pointer to the string (ex: "google.com" ou "google.com:80")
		uint32_t guestAddrPtr = state->r5;      // Virtual pointer to the MacInetAddressLayout output structure

		if (guestStrPtr == 0 || guestAddrPtr == 0)
		{
			state->r3 = -3206; // kOTBadArgsErr
			return;
		}

		// 1.Translate and isolate the string sent by VM
		const char* rawHostStr = globals->allocator.ToPointer<const char>(guestStrPtr);
		std::string hostStr(rawHostStr);

		// OpenTransport suports "host:port" (ex: "127.0.0.1:8080"). Then the port can be extracted
		std::string nodeStr = hostStr;
		std::string portStr = "0"; // Default port if not specified
		
		size_t colonPos = hostStr.find_last_of(':');
		if (colonPos != std::string::npos)
		{
			nodeStr = hostStr.substr(0, colonPos);
			portStr = hostStr.substr(colonPos + 1);
		}

		// 2. Configure and set the hist native DNS resolution (getaddrinfo)
		struct addrinfo hints;
		struct addrinfo* res = nullptr;
		std::memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;       // Force IPv4 ( Mac OS 9 do not support native IPv6 nativo non classic OT)
		hints.ai_socktype = SOCK_STREAM; // Generic type only for resolutuion

		int status = getaddrinfo(nodeStr.c_str(), portStr.c_str(), &hints, &res);
		if (status != 0)
		{
			// If getaddrinfo fails (ex: fail to resolve a domain or without internet), return DNS error
			state->r3 = -3216; // kOTBadAddressErr (Or a equivalent DNS errorfrom OpenTransport)
			return;
		}

		// 3. Extract the results from the host operating system
		struct sockaddr_in* ipv4 = (struct sockaddr_in*)res->ai_addr;
		uint32_t resolvedIp = ntohl(ipv4->sin_addr.s_addr);
		uint16_t resolvedPort = ntohs(ipv4->sin_port);

		// release thememory from native getaddrinfo
		freeaddrinfo(res);

		// 4. Store the result back to VM virtual memory (Big Endian)
		auto* macInetAddr = globals->allocator.ToPointer<MacInetAddressLayout>(guestAddrPtr);
		
		macInetAddr->addressType.Set(2); // AF_INET on Classic Mac OS always is 2
		macInetAddr->port.Set(resolvedPort);
		macInetAddr->host.Set(resolvedIp);

		state->r3 = 0; // kOTNoError (Sucess!)
	}

    // OTInetAddressToName(InetInterfaceRef ref, InetAddress* addr, char* nameBuf)
	void OpenTransportLib_OTInetAddressToName(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestInterfaceRef = state->r3; // Normally ignored (kDefaultInetInterface)
		uint32_t guestAddrPtr = state->r4;      // Virtual Pointer for MacInetAddressLayout inpuit structure
		uint32_t guestNameBufPtr = state->r5;   // Virtual Pointer to the buffer to store the domain

		if (guestAddrPtr == 0 || guestNameBufPtr == 0)
		{
			state->r3 = -3206; // kOTBadArgsErr
			return;
		}

		// 1. Translate and extract the input IP from the VM (using Big Endian)
		const auto* macInetAddr = globals->allocator.ToPointer<MacInetAddressLayout>(guestAddrPtr);
		uint32_t guestIp = macInetAddr->host.Get();

		// Mount the standard sockaddr_in that the host waits
		sockaddr_in hostAddr;
		std::memset(&hostAddr, 0, sizeof(hostAddr));
		hostAddr.sin_family = AF_INET;
		hostAddr.sin_addr.s_addr = htonl(guestIp);

		// 2. Make ready a local buffer from  host to receive the domain string
		char hostNameBuffer[NI_MAXHOST];
		std::memset(hostNameBuffer, 0, sizeof(hostNameBuffer));

		// Execute the reverse resolution from  C++ of host
		int status = getnameinfo((struct sockaddr*)&hostAddr, sizeof(hostAddr),
		                         hostNameBuffer, sizeof(hostNameBuffer),
		                         nullptr, 0, NI_NAMEREQD);

		if (status != 0)
		{
			// If reverse DNS fails or the IP do not had a proper domain.
			state->r3 = -3216; // kOTBadAddressErr
			return;
		}

		// 3. Translate the writing pointer from VM and copy the string to thewre
		char* guestNameBuf = globals->allocator.ToPointer<char>(guestNameBufPtr);
		
		// Safety: OpenTransport stipulate thst the output buffer should have typically 256 bytes
		std::strncpy(guestNameBuf, hostNameBuffer, 255);
		guestNameBuf[255] = '\0'; // Garanty the null terminador

		state->r3 = 0; // kOTNoError (Sucess!)
	}
   

	// OTCreateConfiguration(const char* path) -> Retorna um ponteiro opaco
	void OpenTransportLib_OTCreateConfiguration(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		// Copy the current configuration
		state->r3 = state->r3; 
	}

	// OTRegisterAsClient(StringPtr name, OTNotifyProcPtr proc)
	void OpenTransportLib_OTRegisterAsClient(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0; // kOTNoError
	}

	// OTUnregisterAsClient()
	void OpenTransportLib_OTUnregisterAsClient(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0; // kOTNoError
	}

	// OTUnbind(EndpointRef ref)
	void OpenTransportLib_OTUnbind(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	   // It close the current connection
		state->r3 = 0; // kOTNoError
	}

	// OTSndDisconnect(EndpointRef ref, TCall* call)
	void OpenTransportLib_OTSndDisconnect(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it != globals->endpointMap.end())
		{
			// Execute a safe closing using the native native BSD socket
			shutdown(it->second.fd, SHUT_WR);
		}
		state->r3 = 0; // kOTNoError
	}

	// OTRcvDisconnect(EndpointRef ref, TDiscon* discon)
	void OpenTransportLib_OTRcvDisconnect(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0; // kOTNoError
	}

	// OTSndOrderlyDisconnect(EndpointRef ref)
	void OpenTransportLib_OTSndOrderlyDisconnect(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it != globals->endpointMap.end())
		{
			shutdown(it->second.fd, SHUT_WR);
		}
		state->r3 = 0; // kOTNoError
	}

	// OTRcvOrderlyDisconnect(EndpointRef ref)
	void OpenTransportLib_OTRcvOrderlyDisconnect(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0; // kOTNoError
	}

	// OTRemoveNotifier(ProviderRef ref)
	void OpenTransportLib_OTRemoveNotifier(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it != globals->endpointMap.end())
		{
			it->second.notifierUPP = 0;
			it->second.contextPtr = 0;
		}
		state->r3 = 0; // kOTNoError
	}

	// OTSetSynchronous(ProviderRef ref)
	void OpenTransportLib_OTSetSynchronous(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it != globals->endpointMap.end())
		{
			it->second.isAsynchronous = false;
			// Remove the non-lockinf flag from the native host socket
			int flags = fcntl(it->second.fd, F_GETFL, 0);
			fcntl(it->second.fd, F_SETFL, flags & ~O_NONBLOCK);
		}
		state->r3 = 0; // kOTNoError
	}

	// OTAckSends(ProviderRef ref)
	void OpenTransportLib_OTAckSends(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0; // kOTNoError
	}

	// OTDontAckSends(ProviderRef ref)
	void OpenTransportLib_OTDontAckSends(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0; // kOTNoError
	}

	// OTIoctl(ProviderRef ref, uint32_t cmd, void* arg)
	void OpenTransportLib_OTIoctl(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0; // kOTNoError (Ignore low level STREAMS UNIX)
	}

	// OTGetEndpointState(EndpointRef ref)
	void OpenTransportLib_OTGetEndpointState(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 4; //Safe T_DATAXFER 4 flag to make connect and and to download
	}

	// OTSync(ProviderRef ref)
	void OpenTransportLib_OTSync(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0; // kOTNoError
	}

	// OTUnknown - A fake function for unknown functions that can be settled as null.
	void OpenTransportLib_OTUnknown(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0; 
	}
        
}
