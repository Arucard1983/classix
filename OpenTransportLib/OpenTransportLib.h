//
// OpenTransportLib.h
// Classix
//
// Copyright (C) 2012 Félix Cloutier
//
// Copyright (C) 2026 Arucard 1983
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

#ifndef __Classix__OpenTransportLib__
#define __Classix__OpenTransportLib__

#include "Allocator.h"
#include "MachineState.h"
#include "SymbolType.h"

namespace OpenTransportLib
{
	struct Globals;
}
	
namespace OSEnvironment
{
	class Managers;
}

extern "C"
{
	OpenTransportLib::Globals* LibraryLoad(Common::Allocator* allocator, OSEnvironment::Managers* managers);
	SymbolType LibraryLookup(OpenTransportLib::Globals* globals, const char* symbolName, void** symbol);
	void LibraryUnload(OpenTransportLib::Globals* globals);
	
	extern const char* LibraryCodeSymbolNames[];
	extern const char* LibraryDataSymbolNames[];
}

#endif /* defined(__Classix__OpenTransportLib__) */
