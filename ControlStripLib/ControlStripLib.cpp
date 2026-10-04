//
// ControlStripLib.cpp
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

#include <dlfcn.h>

#include "ControlStripLib.h"
#include "Managers.h"
#include "MachineState.h"
#include "NotImplementedException.h"

namespace ControlStripLib
{
	struct Globals
	{
		Common::Allocator& allocator;
		
		Globals(Common::Allocator& allocator)
		: allocator(allocator)
		{ }
	};
}

extern "C"
{
	ControlStripLib::Globals* LibraryLoad(Common::Allocator* allocator, OSEnvironment::Managers* managers)
	{
		managers->Gestalt().SetValue("sdev", 0);
		return allocator->Allocate<ControlStripLib::Globals>("ControlStripLib Globals", *allocator);
	}
	
	SymbolType LibraryLookup(ControlStripLib::Globals* globals, const char* name, void** result)
	{
		char functionName[42] = "ControlStripLib_";
		char* end = stpncpy(functionName + 16, name, 25);
		if (*end == 0)
		{
			if (void* symbol = dlsym(RTLD_SELF, functionName))
			{
				*result = symbol;
				return CodeSymbol;
			}
		}
		
		*result = nullptr;
		return SymbolNotFound;
	}
	
	void LibraryUnload(ControlStripLib::Globals* globals)
	{
		globals->allocator.Deallocate(globals);
	}
	
#pragma mark -
	
	void ControlStripLib_SBGetBarGraphWidth(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// short SBGetBarGraphWidth(short barCount);
		state->r3 = 0;
	}
	
	void ControlStripLib_SBTrackPopupMenu(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// short SBTrackPopupMenu(const Rect* moduleRect, MenuRef menu);
		state->r3 = 0;
	}
	
	void ControlStripLib_SBIsShowHideHotKeyEnabled(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// OSErr SBIsShowHideHotKeyEnabled(Boolean* enabled);
		uint32_t enabledAddress = state->r3;
		if (globals->allocator.IsValidAddress(enabledAddress, sizeof(uint8_t)))
		{
			uint8_t* enabled = globals->allocator.ToPointer<uint8_t>(enabledAddress);
			*enabled = 0; // false
		}
		state->r3 = 0;
	}
	
	void ControlStripLib_SBSavePreferences(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// OSErr SBSavePreferences(ConstStr255Param prefResourceName, Handle preferences);
		state->r3 = 0;
	}
	
	void ControlStripLib_SBIsControlStripVisible(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		state->r3 = 0;
	}
	
	void ControlStripLib_SBEnableShowHideHotKey(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// OSErr SBEnableShowHideHotKey(Boolean enabled);
		throw PPCVM::NotImplementedException(__func__);
	}
	
	void ControlStripLib_SBShowHelpString(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// OSErr SBShowHelpString(const Rect* moduleRect, StringPtr helpString);
		state->r3 = 0;
	}
	
	void ControlStripLib_SBModalDialogInContext(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// void SBModalDialogInContext(ModalFilterUPP filterProc, short* itemHit);
		state->r3 = 0;
	}
	
	void ControlStripLib_SBGetDetachedIndString(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// void SBGetDetachedIndString(StringPtr theString, Handle stringList, short whichString);
		state->r3 = 0;
	}
	
	void ControlStripLib_SBSetControlStripFontID(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// OSErr SBSetControlStripFontID(short fontID);
		state->r3 = 0;
	}
	
	void ControlStripLib_SBGetShowHideHotKey(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// OSErr SBGetShowHideHotKey(short* modifiers, unsigned char* keyCodes);
		state->r3 = 0;
	}
	
	void ControlStripLib_SBShowHideControlStrip(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// void SBShowHideControlStrip(Boolean showIt);
		state->r3 = 0;
	}
	
	void ControlStripLib_SBTrackSlider(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// short SBTrackSlider(const Rect* moduleRect, short ticksOnSlider, short initialValue);
		state->r3 = state->r5;
	}
	
	void ControlStripLib_SBSetShowHideHotKey(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// OSErr SBSetShowHideHotKey(short modifiers, unsigned char keyCode);
		state->r3 = 0;
	}
	
	void ControlStripLib_SBGetControlStripFontSize(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// OSErr SBGetControlStripFontSize(short* fontSize);
		state->r3 = 0;
	}
	
	void ControlStripLib_SBOpenModuleResourceFile(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// short SBOpenModuleResourceFile(OSType fileCreator);
		state->r3 = -1;
	}
	
	void ControlStripLib_SBGetDetachIconSuite(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// OSErr SBGetDetachIconSuite(Handle* theIconSuite, short theResID, unsigned long selector);
		state->r3 = 0;
	}
	
	void ControlStripLib_SBHitTrackSlider(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// short SBHitTrackSlider(const Rect* moduleRect, short ticksOnSlider, short initialValue, Boolean* hit);
		uint32_t hitAddr = state->r6;
		if (globals->allocator.IsValidAddress(hitAddr, sizeof(uint8_t)))
		{
			uint8_t* hit = globals->allocator.ToPointer<uint8_t>(hitAddr);
			*hit = 0; // Not clicked
		}
		state->r3 = state->r5;
	}
	
	void ControlStripLib_SBDrawBarGraph(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// void SBDrawBarGraph(short level, short barCount, short direction, Point barGraphTopLeft);
		state->r3 = 0;
	}
	
	void ControlStripLib_SBGetControlStripFontID(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// OSErr SBGetControlStripFontID(short* fontId);
		uint32_t fontIdAddr = state->r3;
		if (globals->allocator.IsValidAddress(fontIdAddr, sizeof(int16_t)))
		{
			int16_t* fontId = globals->allocator.ToPointer<int16_t>(fontIdAddr);
			*fontId = 0; // Chicago / System Font default
		}
		state->r3 = 0;
	}
	
	void ControlStripLib_SBSafeToAccessStartupDisk(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// Boolean SBSafeToAccessStartupDisk();
		state->r3 = 1;
	}
	
	void ControlStripLib_SBLoadPreferences(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// OSErr SBLoadPreferences(ConstStr255Param perfResourceName, Handle* preferences);
		state->r3 = 0;
	}
	
	void ControlStripLib_SBSetControlStripFontSize(ControlStripLib::Globals* globals, PPCVM::MachineState* state)
	{
		// OSErr SBSetControlStripFontSize(short fontSize);
		state->r3 = 0;
	}
}
