//
// CarbonLib.h
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
// Initial CarbonLib list using original InterfaceLib prototypes, before the purge begins...
//Copyright (C) 2026 Arucard1983

#include <array>
#include <type_traits>
#include <unordered_map>

#include "CommonDefinitions.h"
#include "Allocator.h"
#include "MachineState.h"
#include "SymbolType.h"
#include "BigEndian.h"
#include "ResourceManager.h"
#include "GrafPortManager.h"
#include "UIChannel.h"
#include "ResourceTypes.h"
#include "Managers.h"

namespace CarbonLib
{
	struct Globals
	{
		// the padding helps make it less harmful if a program was to overwrite stuff from the globals
		uint8_t padding[0x1000];
		
		Common::Allocator& allocator;
		OSEnvironment::Managers& managers;
		GrafPortManager grafPorts;
		std::vector<const Resources::MENU*> menus;
		UIChannel* uiChannel;
		
		std::array<std::string, 4> dialogParams;
		
		uint32_t systemFatalErrorHandler;
		
		Globals(Common::Allocator& allocator, OSEnvironment::Managers& managers);
		
		inline UIChannel& ipc() { return *uiChannel; }
		inline OSEnvironment::ResourceManager& resources() { return managers.ResourceManager(); }
		inline OSEnvironment::Gestalt& gestalt() { return managers.Gestalt(); }
		
		~Globals();
	};
	
	const std::string& IPCMessageName(IPCMessage message);
}

extern "C"
{
	CarbonLib::Globals* LibraryLoad(Common::Allocator* allocator, OSEnvironment::Managers* managers);
	SymbolType LibraryLookup(CarbonLib::Globals* globals, const char* symbolName, void** symbol);
	void LibraryUnload(CarbonLib::Globals* context);
	
	extern const char* LibraryCodeSymbolNames[];
	extern const char* LibraryDataSymbolNames[];
	
	void CarbonLib___LibraryInit(CarbonLib::Globals* globals, PPCVM::MachineState* state);
}

