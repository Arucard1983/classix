//
// MacWindows.cpp
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
// Copyrigh (C) 2026 Arucard1983
// Window Management using Carbon

#include <Carbon/Carbon.h>
#include <CoreGraphics/CoreGraphics.h>
#include <sstream>
#include "Prototypes.h"
#include "CarbonLib.h"
#include "ResourceTypes.h"
#include "NotImplementedException.h"
#include "Todo.h"

using namespace OSEnvironment;
using namespace CarbonLib;
using namespace CarbonLib::Resources;

void CarbonLib_BeginUpdate(CarbonLib::Globals* globals, MachineState* state)
{
	UGrafPort& port = *globals->allocator.ToPointer<UGrafPort>(state->r3);
	globals->grafPorts.BeginUpdate(port);
}

void CarbonLib_BringToFront(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CalcVis(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CalcVisBehind(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CheckUpdate(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ClipAbove(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DisposeWindow(CarbonLib::Globals* globals, MachineState* state)
{
	uint32_t windowKey = state->r3;
	 if (windowKey == 0) return; //avoid null pointers
	globals->ipc().PerformAction<void>(IPCMessage::CloseWindow, windowKey);
	UGrafPort* port = globals->allocator.ToPointer<UGrafPort>(windowKey);
	globals->grafPorts.DestroyGrafPort(*port);
	globals->allocator.Deallocate(port);
}

void CarbonLib_DragGrayRgn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DragTheRgn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DragWindow(CarbonLib::Globals* globals, MachineState* state)
{
	uint32_t windowKey = state->r3;
	uint32_t boundsAddress = state->r5;
	
	// TODO: Create a default size if the app calls NULL (0)
	CarbonLib::Rect dragBounds{}; 
	
	if (boundsAddress != 0)
	{
		dragBounds = *globals->allocator.ToPointer<CarbonLib::Rect>(boundsAddress);
	}
	else
	{
		// Optional: Set a big rectangle if the size is 0
		dragBounds.top = 0; dragBounds.left = 0;
		dragBounds.bottom = 2000; dragBounds.right = 2000;
	}
	
	globals->ipc().PerformAction<void>(IPCMessage::DragWindow, windowKey, dragBounds);
}

void CarbonLib_DrawGrowIcon(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DrawNew(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_EndUpdate(CarbonLib::Globals* globals, MachineState* state)
{
	uint32_t key = state->r3;
	 if (key == 0) return; // Safety net agianst null pointers
	UGrafPort* port = globals->allocator.ToPointer<UGrafPort>(key);
	 if (!port) return;
	CGRect dirtyRect = globals->grafPorts.EndUpdate(*port);
	globals->ipc().PerformAction<void>(IPCMessage::SetDirtyRect, key, dirtyRect);
}

void CarbonLib_FrontWindow(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = globals->ipc().PerformAction<uint32_t>(IPCMessage::FindFrontWindow);
}

void CarbonLib_GetAuxWin(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetCWMgrPort(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetGrayRgn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetNewCWindow(CarbonLib::Globals* globals, MachineState* state)
{
	uint16_t resourceId = static_cast<uint16_t>(state->r3);
	WIND* window = globals->resources().GetResource<WIND>(resourceId);
	
	if (window == nullptr)
	{
		state->r3 = 0;
		return; //Abort if null pointer
	}
	
	uint32_t portAddress = state->r4;
	UGrafPort* port;
	std::string title = window->title;
	const CarbonLib::Rect& rect = window->windowRect;
	
	Palette* palette = nullptr;
	if (ResourceEntry* entry = globals->resources().GetRawResource("pltt", resourceId))
	{
		palette = reinterpret_cast<Palette*>(entry->begin());
	}
	
	if (portAddress == 0)
	{
		std::stringstream ss;
		ss << "Window: \"" << title << "\"";
		port = &globals->grafPorts.AllocateColorGrafPort(rect, palette, title);
		portAddress = globals->allocator.ToIntPtr(port);
	}
	else
	{
		port = globals->allocator.ToPointer<UGrafPort>(portAddress);
		assert(port->IsColor() && "Not a Color QuickDraw port");
	}
	
	bool visible = window->visible;
	int16_t procID  = window->procID; 
	uint32_t createBehind = state->r5;
	bool goAwayFlag  = window->goAwayFlag;
	uint32_t refCon = window->refCon;
	uint32_t surfaceId = globals->grafPorts.SurfaceOfGrafPort(*port);
	
	globals->ipc().PerformAction<void>(IPCMessage::CreateWindow, portAddress, surfaceId, rect, visible, title, createBehind, procID, goAwayFlag, refCon);
	state->r3 = portAddress;
}

void CarbonLib_GetNewWindow(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetWindowPic(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetWMgrPort(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetWRefCon(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetWTitle(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetWVariant(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GrowWindow(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_HideWindow(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_HiliteWindow(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_InitWindows(CarbonLib::Globals* globals, MachineState* state)
{
	// nothing to do here
}

void CarbonLib_InvalRect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_InvalRgn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_NewCWindow(CarbonLib::Globals* globals, MachineState* state)
{
	
	// We use storage pointers as window indentifiers in the UI head, since they're guaranteed to be unique
	// for any given window.
	uint32_t portAddress = state->r3;
	UGrafPort* port;
	
	// We try to get the title first, since we can use it for the allocation name.
	const char* pascalTitle = globals->allocator.ToPointer<const char>(state->r5);
	std::string cppTitle = PascalStringToCPPString(pascalTitle);
	
	const CarbonLib::Rect& rect = *globals->allocator.ToPointer<const CarbonLib::Rect>(state->r4);
	
	if (portAddress == 0)
	{
		std::stringstream ss;
		ss << "Window: \"" << cppTitle << "\"";
		port = &globals->grafPorts.AllocateColorGrafPort(rect, nullptr, cppTitle);
		portAddress = globals->allocator.ToIntPtr(port);
	}
	else
	{
		port = globals->allocator.ToPointer<UGrafPort>(portAddress);
		assert(port->IsColor() && "Not a Color QuickDraw port");
	}
	
	bool visible = state->r6 != 0;
	int16_t procID = static_cast<int16_t>(state->r7);  // r7: Style or Border (ex: documentProc)
	uint32_t createBehind = state->r8; // Z order of Window
	bool goAwayFlag  = state->r9 != 0;                   // r9: if display the close button
	uint32_t refCon      = state->r10;                       // r10: Application data reference
	uint32_t surfaceId = globals->grafPorts.SurfaceOfGrafPort(*port);
	
	globals->ipc().PerformAction<void>(IPCMessage::CreateWindow, portAddress, surfaceId, rect, visible, cppTitle, createBehind, procID, goAwayFlag, refCon);
	state->r3 = portAddress;
}

void CarbonLib_NewWindow(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PaintBehind(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PaintOne(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PinRect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SaveOld(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SelectWindow(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SendBehind(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetDeskCPat(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetWinColor(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetWindowPic(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetWRefCon(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetWTitle(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ShowHide(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SizeWindow(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_TrackBox(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_TrackGoAway(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ValidRect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ValidRgn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ZoomWindow(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

