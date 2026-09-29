//
// Palettes.cpp
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

#include <CoreGraphics/CoreGraphics.h>
#include "Prototypes.h"
#include "NotImplementedException.h"
#include "CarbonLib.h"
#include "Todo.h"

void CarbonLib_ActivatePalette(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_AnimateEntry(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CopyPalette(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CTab2Palette(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DisposePalette(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_Entry2Index(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetEntryColor(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetEntryUsage(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetGray(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetNewPalette(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetPalette(CarbonLib::Globals* globals, MachineState* state)
{
	uint32_t requestedPalette = state->r3;
	if (requestedPalette == 0xffffffff)
	{
		state->r3 = globals->allocator.ToIntPtr(&globals->grafPorts.GetDefaultPalette());
	}
	else
	{
		CarbonLib::UGrafPort& port = *globals->allocator.ToPointer<CarbonLib::UGrafPort>(requestedPalette);
		CarbonLib::Palette* palette = globals->grafPorts.PaletteOfGrafPort(port);
		state->r3 = globals->allocator.ToIntPtr(palette);
	}
}

void CarbonLib_GetPaletteUpdates(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_HasDepth(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_InitPalettes(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_NewPalette(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_NSetPalette(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_Palette2CTab(CarbonLib::Globals* globals, MachineState* state)
{
	const CarbonLib::Palette* palette = globals->allocator.ToPointer<CarbonLib::Palette>(state->r3);
	const Common::UInt32& cTabPtr = *globals->allocator.ToPointer<Common::UInt32>(state->r4);
	CarbonLib::ColorTable* cTab = globals->allocator.ToPointer<CarbonLib::ColorTable>(cTabPtr);
	cTab->count = palette->pmEntries;
	
	for (int16_t i = 0; i < palette->pmEntries; i++)
	{
		const CarbonLib::ColorInfo& input = palette->pmInfo[i];
		CarbonLib::ColorSpec& output = cTab->table[i];
		output.value = i;
		output.rgb = input.ciRGB;
	}
}

void CarbonLib_PmBackColor(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PmForeColor(CarbonLib::Globals* globals, MachineState* state)
{
	TODO("Add support for 8-bit pixel depth");
	CarbonLib::UGrafPort& port = globals->grafPorts.GetCurrentPort();
	if (CarbonLib::ColorTable* table = globals->grafPorts.ColorTableOfGrafPort(port))
	{
		int16_t colorIndex = static_cast<int16_t>(state->r3);
		if (colorIndex >= table->count)
		{
			std::cerr << "*** invalid color index " << colorIndex << " for " << __func__ << std::endl;
			return;
		}
		
		port.color.rgbFgColor = table->table[colorIndex].rgb;
	
		CGFloat max = std::numeric_limits<uint16_t>::max();
		CGFloat r = port.color.rgbFgColor.red / max;
		CGFloat g = port.color.rgbFgColor.green / max;
		CGFloat b = port.color.rgbFgColor.blue / max;
		
		CGContextRef ctx = globals->grafPorts.ContextOfGrafPort(port);
		CGContextSetRGBFillColor(ctx, r, g, b, 1);
		CGContextSetRGBStrokeColor(ctx, r, g, b, 1);
	}
	else
	{
		std::cerr << "*** Using " << __func__ << " on a non-color port" << std::endl;
		return;
	}
}

void CarbonLib_PMgrVersion(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_RestoreBack(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_RestoreDeviceClut(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_RestoreFore(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SaveBack(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SaveFore(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetDepth(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetEntryColor(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetEntryUsage(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetPalette(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetPaletteUpdates(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

