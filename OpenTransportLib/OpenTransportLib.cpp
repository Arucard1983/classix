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
#include <mutex>
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
		int fd;                      // File Descriptor of BSD socket from host
		bool isAsynchronous = false; // Lock status defined by OTSetAsynchronous
		uint32_t notifierUPP = 0;    // Callback address of PowerPC VM 
		uint32_t contextPtr = 0;     // Guest context pointer from Mac OS 9
	};

	struct Globals
	{
		Common::Allocator& allocator;

		// Set the fictitious ID from Guest 
		std::map<uint32_t, EndpointContext> endpointMap;
		std::atomic<uint32_t> nextEndpointId;
		std::mutex endpointMutex; // Garantee mutual exclusion on endpointMap
		
		Globals(Common::Allocator& allocator)
		: allocator(allocator), nextEndpointId(1)
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
		
		*result = (void*)OpenTransportLib_OTUnknown; //Trap to catch safelly unsupported functions.
                 return CodeSymbol;
	}
	
	void LibraryUnload(Globals* globals)
	{
		{
			std::lock_guard<std::mutex> lock(globals->endpointMutex);
			for (auto const& [id, context] : globals->endpointMap)
			{
				close(context.fd);
			}
			globals->endpointMap.clear();
		}
		globals->allocator.Deallocate(globals);
	}
	
	// Auxiliary structs for IP address
	struct MacTNetbuf 
	{
		Common::UInt32 maxlen; 
		Common::UInt32 len;    
		Common::UInt32 buf;    
	};

	struct MacTBind 
	{
		MacTNetbuf addr;     
		Common::UInt32 qlen; 
	};

	struct MacTCall 
	{
		MacTNetbuf addr;  
		MacTNetbuf opt;   
		MacTNetbuf udata; 
		Common::UInt32 sequence;
	};

	struct MacInetAddressLayout 
	{
		Common::UInt16 addressType; 
		Common::UInt16 port;        
		Common::UInt32 host;        
	};

	struct MacTUnitData
	{
		MacTNetbuf addr;  
		MacTNetbuf opt;   
		MacTNetbuf udata; 
	};
	
	void OpenTransportLib_InitOpenTransport(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0; // kOTNoError
	}

	void OpenTransportLib_InitOpenTransportUtilities(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0;
	}
		
	void OpenTransportLib_CloseOpenTransport(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		for (auto const& [id, context] : globals->endpointMap)
		{
			close(context.fd);
		}
		globals->endpointMap.clear();
		state->r3 = 0;
	}
		
	void OpenTransportLib_OTOpenEndpoint(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestConfigPtr = state->r3; 
		uint32_t guestFlags = state->r4;     
		uint32_t guestInfoPtr = state->r5;    
		uint32_t guestErrPtr = state->r6;     

		int socketType = SOCK_STREAM; 

		if (guestConfigPtr != 0)
		{
			const char* configStr = globals->allocator.ToPointer<const char>(guestConfigPtr);
			if (configStr != nullptr)
			{
				if (std::strstr(configStr, "udp") != nullptr || std::strstr(configStr, "UDP") != nullptr)
				{
					socketType = SOCK_DGRAM; 
				}
			}
		}

		int hostSocketFd = socket(AF_INET, socketType, 0);
		if (hostSocketFd < 0)
		{
			if (guestErrPtr != 0)
			{
				auto* outErr = globals->allocator.ToPointer<Common::SInt32>(guestErrPtr);
				if (outErr) outErr->Set(-3211); // kOTSysError
			}
			state->r3 = 0; 
			return;
		}

		uint32_t guestEndpointRef = globals->nextEndpointId++;
		
		EndpointContext context;
		context.fd = hostSocketFd;
		context.isAsynchronous = false; 
		context.notifierUPP = 0;
		context.contextPtr = 0;

		{
			std::lock_guard<std::mutex> lock(globals->endpointMutex);
			globals->endpointMap[guestEndpointRef] = context;
		}

		if (guestInfoPtr != 0)
		{
			struct MacTEndpointInfo {
				Common::UInt32 type;
				Common::UInt32 flags;
			};
			auto* info = globals->allocator.ToPointer<MacTEndpointInfo>(guestInfoPtr);
			if (info)
			{
				info->type.Set(socketType == SOCK_STREAM ? 1 : 2); 
				info->flags.Set(0);
			}
		}

		if (guestErrPtr != 0)
		{
			auto* outErr = globals->allocator.ToPointer<Common::SInt32>(guestErrPtr);
			if (outErr) outErr->Set(0); 
		}

		state->r3 = guestEndpointRef;	 
	}
		
	void OpenTransportLib_OTAsyncOpenEndpoint(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestConfigPtr = state->r3; 
		uint32_t guestFlags = state->r4;     
		uint32_t guestInfoPtr = state->r5;    
		uint32_t guestProcPtr = state->r6;    
		uint32_t guestContext = state->r7;   

		int socketType = SOCK_STREAM; 
		if (guestConfigPtr != 0)
		{
			const char* configStr = globals->allocator.ToPointer<const char>(guestConfigPtr);
			if (configStr != nullptr && (std::strstr(configStr, "udp") != nullptr || std::strstr(configStr, "UDP") != nullptr))
			{
				socketType = SOCK_DGRAM;
			}
		}

		int hostSocketFd = socket(AF_INET, socketType, 0);
		if (hostSocketFd < 0)
		{
			state->r3 = -3211; 
			return;
		}

		uint32_t guestEndpointRef = globals->nextEndpointId++;
		
		EndpointContext context;
		context.fd = hostSocketFd;
		context.isAsynchronous = true;    
		context.notifierUPP = guestProcPtr; 
		context.contextPtr = guestContext;   

		int flags = fcntl(hostSocketFd, F_GETFL, 0);
		fcntl(hostSocketFd, F_SETFL, flags | O_NONBLOCK);

		{
			std::lock_guard<std::mutex> lock(globals->endpointMutex);
			globals->endpointMap[guestEndpointRef] = context;
		}

		if (guestInfoPtr != 0)
		{
			struct MacTEndpointInfo {
				Common::UInt32 type;
				Common::UInt32 flags;
			};
			auto* info = globals->allocator.ToPointer<MacTEndpointInfo>(guestInfoPtr);
			if (info)
			{
				info->type.Set(socketType == SOCK_STREAM ? 1 : 2);
				info->flags.Set(0);
			}
		}

		state->r3 = 0; 
	}
		
	void OpenTransportLib_OTCloseProvider(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		
		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it != globals->endpointMap.end())
		{
			close(it->second.fd); 
			globals->endpointMap.erase(it);
			state->r3 = 0; 
		}
		else
		{
			state->r3 = -3201; // kOTBadProviderErr
		}
	}
		
	void OpenTransportLib_OTBind(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		uint32_t reqPtr = state->r4; 

		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = -3201; 
			return;
		}

		if (reqPtr == 0)
		{
			sockaddr_in hostAddr;
			std::memset(&hostAddr, 0, sizeof(hostAddr));
			hostAddr.sin_family = AF_INET;
			hostAddr.sin_port = htons(0);
			hostAddr.sin_addr.s_addr = htonl(INADDR_ANY);

			if (bind(it->second.fd, (struct sockaddr*)&hostAddr, sizeof(hostAddr)) < 0)
			{
				state->r3 = -3211; 
				return;
			}
			state->r3 = 0; 
			return;
		}

		const auto* guestBind = globals->allocator.ToPointer<MacTBind>(reqPtr);
		if (!guestBind)
		{
			state->r3 = -3206; 
			return;
		}
		
		if (guestBind->addr.len.Get() == 0 || guestBind->addr.buf.Get() == 0)
		{
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

		const auto* macInetAddr = globals->allocator.ToPointer<MacInetAddressLayout>(guestBind->addr.buf.Get());
		if (!macInetAddr)
		{
			state->r3 = -3206;
			return;
		}

		sockaddr_in hostAddr;
		std::memset(&hostAddr, 0, sizeof(hostAddr));
		hostAddr.sin_family = AF_INET;
		hostAddr.sin_port = htons(macInetAddr->port.Get());
		hostAddr.sin_addr.s_addr = htonl(macInetAddr->host.Get());

		if (bind(it->second.fd, (struct sockaddr*)&hostAddr, sizeof(hostAddr)) < 0)
		{
			if (errno == EADDRINUSE) {
				state->r3 = -3213; // kOTAddressBusyErr
			} else {
				state->r3 = -3211; 
			}
			return;
		}

		state->r3 = 0; 
	}
		
	void OpenTransportLib_OTConnect(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		uint32_t sndCallPtr = state->r4; 

		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = -3201; 
			return;
		}

		if (sndCallPtr == 0)
		{
			state->r3 = -3206; 
			return;
		}

		const auto* guestCall = globals->allocator.ToPointer<MacTCall>(sndCallPtr);
		if (!guestCall || guestCall->addr.len.Get() == 0 || guestCall->addr.buf.Get() == 0)
		{
			state->r3 = -3206; 
			return;
		}

		const auto* macInetAddr = globals->allocator.ToPointer<MacInetAddressLayout>(guestCall->addr.buf.Get());
		if (!macInetAddr)
		{
			state->r3 = -3206;
			return;
		}

		sockaddr_in hostAddr;
		std::memset(&hostAddr, 0, sizeof(hostAddr));
		hostAddr.sin_family = AF_INET;
		hostAddr.sin_port = htons(macInetAddr->port.Get());
		hostAddr.sin_addr.s_addr = htonl(macInetAddr->host.Get());

		int result = connect(it->second.fd, (struct sockaddr*)&hostAddr, sizeof(hostAddr));
		
		if (result < 0)
		{
			if (errno == EINPROGRESS)
			{
				state->r3 = -3152; 
			}
			else if (errno == ECONNREFUSED)
			{
				state->r3 = -3212; 
			}
			else
			{
				state->r3 = -3211; 
			}
			return;
		}

		state->r3 = 0; 
	}
		
	void OpenTransportLib_OTRcvConnect(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	      uint32_t guestEndpointRef = state->r3;
	      uint32_t callPtr = state->r4; //  Output MacTCall structure given by PPCVM

	      std::lock_guard<std::mutex> lock(globals->endpointMutex);
	      auto it = globals->endpointMap.find(guestEndpointRef);
	      if (it == globals->endpointMap.end())
	       {
		state->r3 = -3201; // kOTBadProviderErr
		return;
	       }

	       // Tf the PPCVM do not give any structure to fill, just answer OK
	       if (callPtr == 0)
	        {
		 state->r3 = 0; // kOTNoError
		 return;
	        }

	       auto* guestCall = globals->allocator.ToPointer<MacTCall>(callPtr);
	       if (!guestCall)
	       {
		 state->r3 = -3206; // kOTBadArgsErr
		 return;
	       }

	        uint32_t addrBufPtr = guestCall->addr.buf.Get();
	        uint32_t maxAddrLen = guestCall->addr.maxlen.Get();

	      // If the application give buffer size to receive the host adress 
	         if (addrBufPtr != 0 && maxAddrLen >= sizeof(MacInetAddressLayout))
	         {
		   sockaddr_in peerAddr;
		   socklen_t peerLen = sizeof(peerAddr);
		   std::memset(&peerAddr, 0, sizeof(peerAddr));

		  // Request the host operating system about the connect identity 
		  if (getpeername(it->second.fd, (struct sockaddr*)&peerAddr, &peerLen) == 0)
		   {
			auto* macInetAddr = globals->allocator.ToPointer<MacInetAddressLayout>(addrBufPtr);
			if (macInetAddr)
			{
				macInetAddr->addressType.Set(2); // AF_INET no Classic Mac OS
				macInetAddr->port.Set(ntohs(peerAddr.sin_port));
				macInetAddr->host.Set(ntohl(peerAddr.sin_addr.s_addr));
				
				guestCall->addr.len.Set(sizeof(MacInetAddressLayout));
			}
		   } 
	   }

	 // Clean the remaining data  buffers or macTCall strucrtures, since the BSD standard do not use them.
	 guestCall->opt.len.Set(0);
	 guestCall->udata.len.Set(0);

	 state->r3 = 0; // kOTNoError
	}
		
	void OpenTransportLib_OTAccept(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t serverEp = state->r3; 
		uint32_t resEp = state->r4;    

		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto itServer = globals->endpointMap.find(serverEp);
		if (itServer == globals->endpointMap.end())
		{
			state->r3 = -3201; 
			return;
		}

		sockaddr_in clientAddr;
		socklen_t clientLen = sizeof(clientAddr);
		int clientFd = accept(itServer->second.fd, (struct sockaddr*)&clientAddr, &clientLen);

		if (clientFd < 0)
		{
			state->r3 = -3211; 
			return;
		}

		EndpointContext clientContext;
		clientContext.fd = clientFd;
		clientContext.isAsynchronous = itServer->second.isAsynchronous;
		clientContext.notifierUPP = itServer->second.notifierUPP;
		clientContext.contextPtr = itServer->second.contextPtr;

		globals->endpointMap[resEp] = clientContext;
		state->r3 = 0; 
	}
		
	void OpenTransportLib_OTSnd(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		uint32_t guestBufPtr = state->r4;      
		size_t nbytes = state->r5;             
		uint32_t otFlags = state->r6;          

		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = -3201; 
			return;
		}

		if (guestBufPtr == 0 && nbytes > 0)
		{
			state->r3 = -3206; 
			return;
		}

		const void* hostBuffer = globals->allocator.ToPointer<const void>(guestBufPtr);
		int hostFlags = 0;

		ssize_t bytesSent = send(it->second.fd, hostBuffer, nbytes, hostFlags);

		if (bytesSent < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				state->r3 = -3162; // kOTFlowErr
			}
			else if (errno == EPIPE || errno == ECONNRESET)
			{
				state->r3 = -3215; // kOTConnectionClosedErr
			}
			else
			{
				state->r3 = -3211; 
			}
			return;
		}

		state->r3 = static_cast<uint32_t>(bytesSent);	 
	}
		
	void OpenTransportLib_OTRcv(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		uint32_t guestBufPtr = state->r4;      
		size_t nbytes = state->r5;             
		uint32_t guestFlagsPtr = state->r6;    

		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = -3201; 
			return;
		}

		if (guestBufPtr == 0 && nbytes > 0)
		{
			state->r3 = -3206; 
			return;
		}

		void* hostBuffer = globals->allocator.ToPointer<void>(guestBufPtr);
		ssize_t bytesReceived = recv(it->second.fd, hostBuffer, nbytes, 0);

		if (bytesReceived < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				state->r3 = -3162; 
			}
			else if (errno == ECONNRESET)
			{
				state->r3 = -3215; 
			}
			else
			{
				state->r3 = -3211; 
			}
			return;
		}
		else if (bytesReceived == 0)
		{
			state->r3 = -3215; 
			return;
		}

		if (guestFlagsPtr != 0)
		{
			auto* outFlags = globals->allocator.ToPointer<Common::UInt32>(guestFlagsPtr);
			if (outFlags) *outFlags = 0; 
		}

		state->r3 = static_cast<uint32_t>(bytesReceived);
	}
		
	void OpenTransportLib_OTRcvUData(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		uint32_t guestUnitDataPtr = state->r4; 
		uint32_t guestFlagsPtr = state->r5;    

		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = -3201; 
			return;
		}

		if (guestUnitDataPtr == 0)
		{
			state->r3 = -3206; 
			return;
		}

		auto* guestUnitData = globals->allocator.ToPointer<MacTUnitData>(guestUnitDataPtr);
		if (!guestUnitData)
		{
			state->r3 = -3206;
			return;
		}

		sockaddr_in srcHostAddr;
		socklen_t addrLen = sizeof(srcHostAddr);
		std::memset(&srcHostAddr, 0, sizeof(srcHostAddr));

		uint32_t maxPayloadLen = guestUnitData->udata.maxlen.Get();
		uint32_t payloadBufPtr = guestUnitData->udata.buf.Get();

		if (maxPayloadLen == 0 || payloadBufPtr == 0)
		{
			state->r3 = -3206; 
			return;
		}
		void* hostPayloadBuffer = globals->allocator.ToPointer<void>(payloadBufPtr);

		ssize_t bytesReceived = recvfrom(it->second.fd, hostPayloadBuffer, maxPayloadLen, 0,
		                                 (struct sockaddr*)&srcHostAddr, &addrLen);

		if (bytesReceived < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				state->r3 = -3162; 
			} else {
				state->r3 = -3211; 
			}
			return;
		}

		uint32_t addrBufPtr = guestUnitData->addr.buf.Get();
		uint32_t maxAddrLen = guestUnitData->addr.maxlen.Get();
		
		if (addrBufPtr != 0 && maxAddrLen >= sizeof(MacInetAddressLayout))
		{
			auto* macInetAddr = globals->allocator.ToPointer<MacInetAddressLayout>(addrBufPtr);
			if (macInetAddr)
			{
				macInetAddr->addressType.Set(2); 
				macInetAddr->port.Set(ntohs(srcHostAddr.sin_port));
				macInetAddr->host.Set(ntohl(srcHostAddr.sin_addr.s_addr));
				guestUnitData->addr.len.Set(sizeof(MacInetAddressLayout));
			}
		}

		guestUnitData->udata.len.Set(static_cast<uint32_t>(bytesReceived));
		guestUnitData->opt.len.Set(0); 

		if (guestFlagsPtr != 0)
		{
			auto* outFlags = globals->allocator.ToPointer<Common::UInt32>(guestFlagsPtr);
			if (outFlags) *outFlags = 0;
		}

		state->r3 = 0; 
	}
		
	void OpenTransportLib_OTSndUData(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		uint32_t guestUnitDataPtr = state->r4; 

		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = -3201; 
			return;
		}

		if (guestUnitDataPtr == 0)
		{
			state->r3 = -3206; 
			return;
		}

		const auto* guestUnitData = globals->allocator.ToPointer<MacTUnitData>(guestUnitDataPtr);
		if (!guestUnitData || guestUnitData->addr.len.Get() == 0 || guestUnitData->addr.buf.Get() == 0)
		{
			state->r3 = -3206; 
			return;
		}
		
		const auto* macInetAddr = globals->allocator.ToPointer<MacInetAddressLayout>(guestUnitData->addr.buf.Get());
		if (!macInetAddr)
		{
			state->r3 = -3206;
			return;
		}

		sockaddr_in destHostAddr;
		std::memset(&destHostAddr, 0, sizeof(destHostAddr));
		destHostAddr.sin_family = AF_INET;
		destHostAddr.sin_port = htons(macInetAddr->port.Get());
		destHostAddr.sin_addr.s_addr = htonl(macInetAddr->host.Get());

		uint32_t payloadLen = guestUnitData->udata.len.Get();
		uint32_t payloadBufPtr = guestUnitData->udata.buf.Get();

		if (payloadLen == 0 || payloadBufPtr == 0)
		{
			state->r3 = 0; 
			return;
		}
		const void* hostPayloadBuffer = globals->allocator.ToPointer<const void>(payloadBufPtr);

		ssize_t bytesSent = sendto(it->second.fd, hostPayloadBuffer, payloadLen, 0,
		                           (struct sockaddr*)&destHostAddr, sizeof(destHostAddr));

		if (bytesSent < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				state->r3 = -3162; 
			} else {
				state->r3 = -3211; 
			}
			return;
		}

		state->r3 = 0; 
	}
		
	void OpenTransportLib_OTInstallNotifier(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		uint32_t notifierUPP = state->r4; 
		uint32_t contextPtr = state->r5;  

		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it != globals->endpointMap.end())
		{
			it->second.notifierUPP = notifierUPP;
			it->second.contextPtr = contextPtr;
			state->r3 = 0; 
		}
		else
		{
			state->r3 = -3201; 
		}
	}

	void OpenTransportLib_OTSetAsynchronous(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;

		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it != globals->endpointMap.end())
		{
			it->second.isAsynchronous = true;
			int flags = fcntl(it->second.fd, F_GETFL, 0);
			fcntl(it->second.fd, F_SETFL, flags | O_NONBLOCK);
			state->r3 = 0; 
		}
		else
		{
			state->r3 = -3201;
		}
	}

	void OpenTransportLib_OTLook(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;

		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = -3201; 
			return;
		}

		int hostFd = it->second.fd;

		pollfd pfd;
		pfd.fd = hostFd;
		pfd.events = POLLIN | POLLOUT | POLLHUP | POLLERR;
		pfd.revents = 0;

		int pollResult = poll(&pfd, 1, 0);

		if (pollResult <= 0)
		{
			state->r3 = 0; 
			return;
		}
		
		if (pfd.revents & (POLLHUP | POLLERR))
		{
			state->r3 = 0x0008; // T_DISCONNECT
			return;
		}

		if (pfd.revents & POLLIN)
		{
			state->r3 = 0x0001; // T_DATA
			return;
		}

		if (pfd.revents & POLLOUT)
		{
			state->r3 = 0x0004; // T_GODATA
			return;
		}

		state->r3 = 0; 
	}

	void OpenTransportLib_OTInetStringToAddress(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestInterfaceRef = state->r3; 
		uint32_t guestStrPtr = state->r4;       
		uint32_t guestAddrPtr = state->r5;      

		if (guestStrPtr == 0 || guestAddrPtr == 0)
		{
			state->r3 = -3206; 
			return;
		}

		const char* rawHostStr = globals->allocator.ToPointer<const char>(guestStrPtr);
		if (!rawHostStr)
		{
			state->r3 = -3206;
			return;
		}
		std::string hostStr(rawHostStr);

		std::string nodeStr = hostStr;
		std::string portStr = "0"; 
		
		size_t colonPos = hostStr.find_last_of(':');
		if (colonPos != std::string::npos)
		{
			nodeStr = hostStr.substr(0, colonPos);
			portStr = hostStr.substr(colonPos + 1);
		}

		struct addrinfo hints;
		struct addrinfo* res = nullptr;
		std::memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;       
		hints.ai_socktype = SOCK_STREAM; 

		int status = getaddrinfo(nodeStr.c_str(), portStr.c_str(), &hints, &res);
		if (status != 0)
		{
			state->r3 = -3216; 
			return;
		}

		struct sockaddr_in* ipv4 = (struct sockaddr_in*)res->ai_addr;
		uint32_t resolvedIp = ntohl(ipv4->sin_addr.s_addr);
		uint16_t resolvedPort = ntohs(ipv4->sin_port);

		freeaddrinfo(res);

		auto* macInetAddr = globals->allocator.ToPointer<MacInetAddressLayout>(guestAddrPtr);
		if (macInetAddr)
		{
			macInetAddr->addressType.Set(2); 
			macInetAddr->port.Set(resolvedPort);
			macInetAddr->host.Set(resolvedIp);
		}

		state->r3 = 0; 
	}

	void OpenTransportLib_OTInetAddressToName(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestInterfaceRef = state->r3; 
		uint32_t guestAddrPtr = state->r4;      
		uint32_t guestNameBufPtr = state->r5;   

		if (guestAddrPtr == 0 || guestNameBufPtr == 0)
		{
			state->r3 = -3206; 
			return;
		}

		const auto* macInetAddr = globals->allocator.ToPointer<MacInetAddressLayout>(guestAddrPtr);
		if (!macInetAddr)
		{
			state->r3 = -3206;
			return;
		}
		uint32_t guestIp = macInetAddr->host.Get();

		sockaddr_in hostAddr;
		std::memset(&hostAddr, 0, sizeof(hostAddr));
		hostAddr.sin_family = AF_INET;
		hostAddr.sin_addr.s_addr = htonl(guestIp);

		char hostNameBuffer[NI_MAXHOST];
		std::memset(hostNameBuffer, 0, sizeof(hostNameBuffer));

		int status = getnameinfo((struct sockaddr*)&hostAddr, sizeof(hostAddr),
		                         hostNameBuffer, sizeof(hostNameBuffer),
		                         nullptr, 0, NI_NAMEREQD);

		if (status != 0)
		{
			state->r3 = -3216; 
			return;
		}

		char* guestNameBuf = globals->allocator.ToPointer<char>(guestNameBufPtr);
		if (!guestNameBuf)
		{
			state->r3 = -3206;
			return;
		}
		
		std::strncpy(guestNameBuf, hostNameBuffer, 255);
		guestNameBuf[255] = '\0'; 

		state->r3 = 0; 
	}
   
	void OpenTransportLib_OTCreateConfiguration(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = state->r3; 
	}

	void OpenTransportLib_OTRegisterAsClient(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0; 
	}

	void OpenTransportLib_OTUnregisterAsClient(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0; 
	}

	void OpenTransportLib_OTUnbind(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;

		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = -3201; 
			return;
		}

		int currentType = SOCK_STREAM;
		int typeLen = sizeof(currentType);
		getsockopt(it->second.fd, SOL_SOCKET, SO_TYPE, &currentType, (socklen_t*)&typeLen);

		close(it->second.fd);

		int newHostSocketFd = socket(AF_INET, currentType, 0);
		if (newHostSocketFd < 0)
		{
			state->r3 = -3211; 
			return;
		}

		if (it->second.isAsynchronous)
		{
			int flags = fcntl(newHostSocketFd, F_GETFL, 0);
			fcntl(newHostSocketFd, F_SETFL, flags | O_NONBLOCK);
		}

		it->second.fd = newHostSocketFd;
		state->r3 = 0; 
	}

	void OpenTransportLib_OTSndDisconnect(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it != globals->endpointMap.end())
		{
			shutdown(it->second.fd, SHUT_RDWR);
		}
		state->r3 = 0; 
	}

	void OpenTransportLib_OTRcvDisconnect(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = -3201;
			return;
		}

		char dummy;
		ssize_t res = recv(it->second.fd, &dummy, 1, MSG_PEEK | MSG_DONTWAIT);
		if (res == 0) {
			state->r3 = 0; 
		} else {
			state->r3 = 0; 
		}
	}

	void OpenTransportLib_OTSndOrderlyDisconnect(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it != globals->endpointMap.end())
		{
			shutdown(it->second.fd, SHUT_WR);
		}
		state->r3 = 0; 
	}

	void OpenTransportLib_OTRcvOrderlyDisconnect(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = -3201;
			return;
		}
		state->r3 = 0; 
	}

	void OpenTransportLib_OTRemoveNotifier(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it != globals->endpointMap.end())
		{
			it->second.notifierUPP = 0;
			it->second.contextPtr = 0;
		}
		state->r3 = 0; 
	}

	void OpenTransportLib_OTSetSynchronous(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it != globals->endpointMap.end())
		{
			it->second.isAsynchronous = false;
			int flags = fcntl(it->second.fd, F_GETFL, 0);
			fcntl(it->second.fd, F_SETFL, flags & ~O_NONBLOCK);
		}
		state->r3 = 0; 
	}

	void OpenTransportLib_OTAckSends(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0; 
	}

	void OpenTransportLib_OTDontAckSends(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0; 
	}

	void OpenTransportLib_OTIoctl(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0; 
	}

	void OpenTransportLib_OTGetEndpointState(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		uint32_t guestEndpointRef = state->r3;
		std::lock_guard<std::mutex> lock(globals->endpointMutex);
		auto it = globals->endpointMap.find(guestEndpointRef);
		if (it == globals->endpointMap.end())
		{
			state->r3 = 1; // T_UNBND
			return;
		}

		int error = 0;
		socklen_t len = sizeof(error);
		int retval = getsockopt(it->second.fd, SOL_SOCKET, SO_ERROR, &error, &len);

		if (retval != 0 || error != 0) {
			state->r3 = 1; 
			return;
		}

		sockaddr_in peer;
		socklen_t peerLen = sizeof(peer);
		if (getpeername(it->second.fd, (struct sockaddr*)&peer, &peerLen) == 0) {
			state->r3 = 4; // T_DATAXFER
		} else {
			int listening = 0;
			socklen_t listenLen = sizeof(listening);
			getsockopt(it->second.fd, SOL_SOCKET, SO_ACCEPTCONN, &listening, &listenLen);
			
			if (listening) {
				state->r3 = 3; // T_IDLE
			} else {
				state->r3 = 2; // T_BOUND
			}
		}
	}

	void OpenTransportLib_OTSync(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0; 
	}

	void OpenTransportLib_OTUnknown(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = -3204; // kOTNotSupportedErr
	}
}
