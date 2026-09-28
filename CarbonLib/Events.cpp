//
// Events.cpp
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
#include "NotImplementedException.h"

using namespace CarbonLib;

void CarbonLib_Button(CarbonLib::Globals* globals, MachineState* state)
{
	globals->ipc().PerformAction<void>(IPCMessage::RefreshWindows);
	state->r3 = globals->ipc().PerformAction<bool>(IPCMessage::IsMouseDown);
}

void CarbonLib_EventAvail(CarbonLib::Globals* globals, MachineState* state)
{
	// same as GetNextEvent, but without discarding the event if it's matched, and without a timeout
	globals->ipc().PerformAction<void>(IPCMessage::RefreshWindows);
	
	EventMask mask = static_cast<EventMask>(state->r3);
	MacRegion empty;
	empty.rgnSize = 10;
	empty.rgnBBox.top = 0;
	empty.rgnBBox.left = 0;
	empty.rgnBBox.right = 0;
	empty.rgnBBox.bottom = 0;
	
	EventRecord nextEvent = globals->ipc().PerformAction<EventRecord>(IPCMessage::PeekNextEvent, mask, 0, empty);
	
	*globals->allocator.ToPointer<EventRecord>(state->r4) = nextEvent;
	state->r3 = nextEvent.what != 0;
}

void CarbonLib_FlushEvents(CarbonLib::Globals* globals, MachineState* state)
{
	EventMask discardMask = static_cast<EventMask>(state->r3);
	EventMask stopMask = static_cast<EventMask>(state->r4);
	globals->ipc().PerformAction<void>(IPCMessage::DiscardEventsUntil, discardMask, stopMask);
}

void CarbonLib_GetCaretTime(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetDblTime(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetEvQHdr(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetKeys(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetMouse(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetNextEvent(CarbonLib::Globals* globals, MachineState* state)
{
	globals->ipc().PerformAction<void>(IPCMessage::RefreshWindows);
	
	EventMask mask = static_cast<EventMask>(state->r3);
	uint32_t timeout = 0xffffffff;
	
	MacRegion empty;
	empty.rgnSize = 10;
	empty.rgnBBox.top = 0;
	empty.rgnBBox.left = 0;
	empty.rgnBBox.right = 0;
	empty.rgnBBox.bottom = 0;
	
	EventRecord nextEvent = globals->ipc().PerformAction<EventRecord>(IPCMessage::PeekNextEvent, mask, timeout, empty);
	globals->ipc().PerformAction<void>(IPCMessage::DequeueNextEvent, mask);
	
	*globals->allocator.ToPointer<EventRecord>(state->r4) = nextEvent;
	state->r3 = nextEvent.what != 0;
}

void CarbonLib_GetOSEvent(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_IsCmdChar(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_KeyScript(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_KeyTranslate(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_LMGetKbdLast(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_LMGetKbdType(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_LMGetKeyRepThresh(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_LMGetKeyThresh(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_LMSetKbdLast(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_LMSetKbdType(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_LMSetKeyRepThresh(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_LMSetKeyThresh(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_OSEventAvail(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PostEvent(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PPostEvent(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetEventMask(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_StillDown(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SystemClick(CarbonLib::Globals* globals, MachineState* state)
{
	// purposefully does nothing on OS X
}

void CarbonLib_SystemEvent(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SystemTask(CarbonLib::Globals* globals, MachineState* state)
{
	globals->ipc().PerformAction<void>(IPCMessage::RefreshWindows);
}

void CarbonLib_WaitMouseUp(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_WaitNextEvent(CarbonLib::Globals* globals, MachineState* state)
{
	globals->ipc().PerformAction<void>(IPCMessage::RefreshWindows);
	
	MacRegion emptyRegion;
	emptyRegion.rgnSize = 10;
	
	EventMask mask = static_cast<EventMask>(state->r3);
	uint32_t ticksTimeout = state->r5; // tick = 1/60s
	MacRegion* region;
	if (state->r6 != 0)
	{
		Common::UInt32* regionHandle = globals->allocator.ToPointer<Common::UInt32>(state->r6);
		region = globals->allocator.ToPointer<MacRegion>(*regionHandle);
	}
	else
	{
		region = &emptyRegion;
	}
	
	EventRecord nextEvent = globals->ipc().PerformAction<EventRecord>(IPCMessage::PeekNextEvent, mask, ticksTimeout, *region);
	globals->ipc().PerformAction<void>(IPCMessage::DequeueNextEvent, mask);
	*globals->allocator.ToPointer<EventRecord>(state->r4) = nextEvent;
	
	state->r3 = nextEvent.what != 0;
}
