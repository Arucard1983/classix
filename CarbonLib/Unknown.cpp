//
// Unknown.cpp
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

#include <algorithm>
#include "Prototypes.h"
#include "CarbonLib.h"
#include "NotImplementedException.h"
#include "NotSupportedException.h"

using namespace OSEnvironment;
using namespace CarbonLib;

void CarbonLib_AbsoluteDeltaToDuration(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_AbsoluteDeltaToNanoseconds(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_AbsoluteToDuration(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_AbsoluteToNanoseconds(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_AddAbsoluteToAbsolute(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_AddAtomic(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_AddAtomic16(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_AddAtomic8(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_AddDurationToAbsolute(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_AddNanosecondsToAbsolute(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_addpt(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_addresource(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_AnimatePalette(CarbonLib::Globals* globals, MachineState* state)
{
	uint32_t portAddress = state->r3;
	uint32_t colorTablePointer = *globals->allocator.ToPointer<Common::UInt32>(state->r4);
	
	CarbonLib::ColorTable& newTable = *globals->allocator.ToPointer<ColorTable>(colorTablePointer);
	CarbonLib::UGrafPort& port = *globals->allocator.ToPointer<UGrafPort>(portAddress);
	int16_t sourceIndex = static_cast<int16_t>(state->r5);
	int16_t destinationIndex = static_cast<int16_t>(state->r6);
	int16_t length = static_cast<int16_t>(state->r7);
	
	if (CarbonLib::ColorTable* table = globals->grafPorts.ColorTableOfGrafPort(port))
	if (CarbonLib::Palette* palette = globals->grafPorts.PaletteOfGrafPort(port))
	{
		const int16_t limit = std::min<int16_t>({
			length,
			newTable.count - sourceIndex,
			table->count - destinationIndex,
			palette->pmEntries - destinationIndex});
		
		for (int16_t i = 0; i < limit; i++)
		{
			const RGBColor& color = newTable.table[sourceIndex + i].rgb;
			table->table[destinationIndex + i].rgb = color;
			palette->pmInfo[destinationIndex + i].ciRGB = color;
		}
	}
	
	globals->ipc().PerformAction<void>(IPCMessage::RequestUpdate, portAddress);
}

void CarbonLib_appendmenu(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_AppendMenu(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_BitAndAtomic(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_BitAndAtomic16(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_BitAndAtomic8(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_BitOrAtomic(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_BitOrAtomic16(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_BitOrAtomic8(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_BitXorAtomic(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_BitXorAtomic16(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_BitXorAtomic8(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_BlockMoveDataUncached(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_BlockMoveUncached(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_BlockZero(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_BlockZeroUncached(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_c2pstr(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CallOSTrapUniversalProc(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "68k OS Trap emulation is not supported on Darling!");
}

void CarbonLib_CallUniversalProc(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotSupportedException(__func__, "68k code emulation/UniversalProc is not supported on Darling!");
}

void CarbonLib_CloseDriver(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CloseWindow(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CompareAndSwap(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CopyRgn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_create(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_createresfile(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CrsrDevButtonDown(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CrsrDevButtonOp(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CrsrDevButtons(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CrsrDevButtonUp(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CrsrDevDisposeDevice(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CrsrDevDoubleTime(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CrsrDevFlush(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CrsrDevMove(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CrsrDevNewDevice(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CrsrDevSetAcceleration(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CrsrDevSetButtons(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CrsrDevUnitsPerInch(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_Debugger68k(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_debugstr(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DebugStr68k(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DecrementAtomic(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DecrementAtomic16(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DecrementAtomic8(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DeleteMenu(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_deltapoint(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_dibadmount(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DisposeRoutineDescriptor(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_dizero(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_dragcontrol(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_draggrayrgn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_dragwindow(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DrawMenuBar(CarbonLib::Globals* globals, MachineState* state)
{
	globals->ipc().PerformAction<void>(IPCMessage::ClearMenus);
	for (const Resources::MENU* menu : globals->menus)
	{
		uint16_t menuId = menu->menuId;
		globals->ipc().PerformAction<void>(IPCMessage::InsertMenu, menuId, menu->GetTitle());
		size_t i = 1;
		for (const auto* menuItem = menu->GetFirstItem(); menuItem != nullptr; menuItem = menuItem->GetNextItem())
		{
			std::string title = menuItem->GetTitle();
			char keyEquivalent = menuItem->GetKeyEquivalent();
			bool enabled = (menu->enableFlags & (1 << i)) != 0;
			globals->ipc().PerformAction<void>(IPCMessage::InsertMenuItem, menuId, title, keyEquivalent, enabled);
			i++;
		}
	}
}

void CarbonLib_drawstring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DrawText(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DurationToAbsolute(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DurationToNanoseconds(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_eject(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_equalpt(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_EqualRect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_EqualRgn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_equalstring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_FillRect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_FillRgn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_findcontrol(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_finddialogitem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_findwindow(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_FindWindow(CarbonLib::Globals* globals, MachineState* state)
{
	typedef std::tuple<WindowPartCode, uint32_t> FindWindowType;
	
	Common::UInt32 bigEndianPoint = Common::UInt32(state->r3);
	const CarbonLib::Point& point = *reinterpret_cast<CarbonLib::Point*>(&bigEndianPoint);
	
	WindowPartCode windowPart;
	Common::UInt32* windowPointer = globals->allocator.ToPointer<Common::UInt32>(state->r4);
	
	std::tie(windowPart, *windowPointer) =
		globals->ipc().PerformComplexAction<FindWindowType>(IPCMessage::FindWindowByCoordinates, point);
	
	state->r3 = static_cast<uint32_t>(windowPart);
}

void CarbonLib_flushvol(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_FrameRect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_FrameRgn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_fsdelete(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_fsopen(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_fsrename(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_get1namedresource(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_getcontroltitle(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetCurrentProcess(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetCursor(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_getdialogitemtext(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_getfinfo(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_getfnum(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_getfontname(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_getindstring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetMenu(CarbonLib::Globals* globals, MachineState* state)
{
	uint16_t key = static_cast<uint16_t>(state->r3);
	ResourceEntry* entry = globals->resources().GetRawResource("MENU", key);
	state->r3 = globals->allocator.ToIntPtr(&entry->handle());
}

void CarbonLib_getmenuitemtext(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_getnamedresource(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetPixel(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_getresinfo(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetScript(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetTimeBaseInfo(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_getvinfo(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_getvol(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_getwtitle(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_growwindow(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_IncrementAtomic(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_IncrementAtomic16(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_IncrementAtomic8(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_InsertMenu(CarbonLib::Globals* globals, MachineState* state)
{
	uint32_t pointerAddress = *globals->allocator.ToPointer<Common::UInt32>(state->r3);
	const Resources::MENU* pointer = globals->allocator.ToPointer<Resources::MENU>(pointerAddress);
	auto insertAt = std::find(globals->menus.begin(), globals->menus.end(), pointer);
	globals->menus.insert(insertAt, pointer);
}

void CarbonLib_insertmenuitem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_InsertMenuItem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_InsetRect(CarbonLib::Globals* globals, MachineState* state)
{
	CarbonLib::Rect& rect = *globals->allocator.ToPointer<CarbonLib::Rect>(state->r3);
	rect.left = static_cast<int16_t>(rect.left + state->r4);
	rect.right = static_cast<int16_t>(rect.right - state->r4);
	rect.bottom = static_cast<int16_t>(rect.bottom - state->r5);
	rect.top = static_cast<int16_t>(rect.top + state->r5);
	
	if (rect.right - rect.left < 1 || rect.bottom - rect.top < 1)
	{
		memset(&rect, 0, sizeof rect);
	}
}

void CarbonLib_InvertRect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_InvertRgn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_iucomppstring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_iucompstring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_iudatepstring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_iudatestring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_iuequalpstring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_iuequalstring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_iuldatestring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_iultimestring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_iustringorder(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_iutimepstring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_iutimestring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_laddtocell(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_lcellsize(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_lclick(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_lclrcell(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ldraw(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_lgetcell(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_lgetcelldatalocation(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_LineTo(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_LMGetTheCursor(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_LMSetTheCursor(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_lnew(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_LoadResource(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_lrect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_lsetcell(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_lsetselect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_menuselect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_MoveWindow(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_NanosecondsToAbsolute(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_NanosecondsToDuration(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_newcolordialog(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_newcontrol(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_newcwindow(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_newdialog(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_newmenu(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_NewRoutineDescriptor(CarbonLib::Globals* globals, MachineState* state)
{
	RoutineDescriptor* descriptor = globals->allocator.AllocateVariableSize<RoutineDescriptor, RoutineRecord>("Routine Descriptor", 1);
	descriptor->goMixedModeTrap = RoutineDescriptor::kMixedModeMagic;
	descriptor->version = 7; // latest version per MixedMode.h
	descriptor->routineDescriptorFlags = 0;
	descriptor->reserved1 = 0;
	descriptor->reserved2 = 0;
	descriptor->selectorInfo = 0;
	descriptor->routineCount = 0; // it's actually count-1
	descriptor->routineRecords[0].procInfo = state->r4;
	descriptor->routineRecords[0].reserved1 = 0;
	descriptor->routineRecords[0].ISA = static_cast<uint8_t>(state->r5);
	descriptor->routineRecords[0].routineFlags = 4; // kUseNativeISA
	descriptor->routineRecords[0].procDescriptor = state->r3;
	descriptor->routineRecords[0].reserved2 = 0;
	descriptor->routineRecords[0].selector = 0;
	
	state->r3 = globals->allocator.ToIntPtr(descriptor);
}

void CarbonLib_newstring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_newwindow(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_NQDMisc(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_numtostring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_OffsetRect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_OffsetRgn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_opendeskacc(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_opendriver(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_OpenDriver(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_openresfile(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_openrf(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_openrfperm(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_p2cstr(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PaintRgn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_paramtext(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_pinrect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_pt2rect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ptinrect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PtInRect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ptinrgn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_pttoangle(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_relstring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ReplaceText(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ResizePalette(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_rstflock(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_setcontroltitle(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetCursor(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_setdialogitemtext(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_setfinfo(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_setflock(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_setmenuitemtext(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetPort(CarbonLib::Globals* globals, MachineState* state)
{
	UGrafPort& port = *globals->allocator.ToPointer<UGrafPort>(state->r3);
	globals->grafPorts.SetCurrentPort(port);
}

void CarbonLib_SetRect(CarbonLib::Globals* globals, MachineState* state)
{
	CarbonLib::Rect& rect = *globals->allocator.ToPointer<CarbonLib::Rect>(state->r3);
	rect.left = static_cast<int16_t>(state->r4);
	rect.top = static_cast<int16_t>(state->r5);
	rect.right = static_cast<int16_t>(state->r6);
	rect.bottom = static_cast<int16_t>(state->r7);
}

void CarbonLib_SetRectRgn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_setresinfo(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetScript(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_setstring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_setvol(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_setwtitle(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_sfgetfile(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_sfpgetfile(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_sfpputfile(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_sfputfile(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_shieldcursor(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ShowWindow(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_stdline(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_stdtext(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_stringtonum(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_stringwidth(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_stuffhex(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SubAbsoluteFromAbsolute(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SubDurationFromAbsolute(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SubNanosecondsFromAbsolute(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_subpt(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_teclick(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_TestAndClear(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_TestAndSet(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_testcontrol(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_trackbox(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_trackcontrol(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_trackgoaway(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_UnionRect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_UnionRgn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_unmountvol(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_upperstring(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_UpTime(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_WideAdd(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_WideBitShift(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_WideCompare(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_WideDivide(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_WideMultiply(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_WideNegate(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_WideShift(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_WideSquareRoot(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_WideSubtract(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_WideWideDivide(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_XorRgn(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

