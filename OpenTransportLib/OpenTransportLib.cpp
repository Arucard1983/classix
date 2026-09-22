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
#include "OpenTransportLib.h"
#include "OpenTransportLibFunctions.h"
#include "NotImplementedException.h"
#include "NotSupportedException.h"
#include "MachineState.h"
#include "Todo.h"

namespace OpenTransportLib
{
	struct Globals
	{
		Common::Allocator& allocator;
  
		
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
		char* end = stpncpy(functionName + 17, name, 33);
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
		globals->allocator.Deallocate(globals);
	}
	
	
	
	void OpenTransportLib_InitOpenTransport(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }

	void OpenTransportLib_InitOpenTransportUtilities(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_CloseOpenTransport(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTOpenEndpoint(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTAsyncOpenEndpoint(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTCloseProvider(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTBind(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTUnbind(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTConnect(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTRcvConnect(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTAccept(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTSnd(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTRcv(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTSndUData(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTRcvUData(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTInstallNotifier(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTRemoveNotifier(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTSetAsynchronous(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTSetSynchronous(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTLook(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
	void OpenTransportLib_OTUnknown(OpenTransportLib::Globals* globals, PPCVM::MachineState* state)
	{
	 throw PPCVM::NotImplementedException(__func__);
        }
        
