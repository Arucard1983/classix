//
// Menus.cpp
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
#include "Todo.h"

using namespace CarbonLib;

void CarbonLib_AppendResMenu(CarbonLib::Globals* globals, MachineState* state)
{
	TODO("AppendResMenu does nothing");
}

void CarbonLib_CalcMenuSize(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CheckItem(CarbonLib::Globals* globals, MachineState* state)
{
	uint32_t pointerAddress = *globals->allocator.ToPointer<Common::UInt32>(state->r3);
	const Resources::MENU* menu = globals->allocator.ToPointer<Resources::MENU>(pointerAddress);
	
	uint16_t menuIndex = menu->menuId;
	uint16_t itemIndex = static_cast<uint16_t>(state->r4 - 1); // Classic menus are 1-based
	bool check = state->r5;
	globals->ipc().PerformAction<void>(IPCMessage::CheckItem, menuIndex, itemIndex, check);
}

void CarbonLib_ClearMenuBar(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CountMItems(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DeleteMCEntries(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DeleteMenuItem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DisableItem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DisposeMCInfo(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DisposeMenu(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_EnableItem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_FlashMenuBar(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetItemCmd(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetItemIcon(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetItemMark(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetItemStyle(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetMBarHeight(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 20; //Standard menu height
}

void CarbonLib_GetMCEntry(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetMCInfo(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetMenuBar(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetMenuHandle(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetMenuItemText(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetNewMBar(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_HiliteMenu(CarbonLib::Globals* globals, MachineState* state)
{
	// pass on this one
}

void CarbonLib_InitMenus(CarbonLib::Globals* globals, MachineState* state)
{
	// nothing to do here
}

void CarbonLib_InitProcMenu(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_InsertFontResMenu(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_InsertIntlResMenu(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_InsertResMenu(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_InvalMenuBar(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_LMGetTheMenu(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_MenuChoice(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_MenuKey(CarbonLib::Globals* globals, MachineState* state)
{
	typedef std::tuple<uint16_t, uint16_t> MenuSelectResult;
	char character = static_cast<char>(state->r3);
	
	uint16_t menu;
	uint16_t item;
	std::tie(menu, item) = globals->ipc().PerformComplexAction<MenuSelectResult>(IPCMessage::MenuKey, character);
	state->r3 = (menu << 16) | (item + 1); // menus are 1-based in Classic
}

void CarbonLib_MenuSelect(CarbonLib::Globals* globals, MachineState* state)
{
	typedef std::tuple<uint16_t, uint16_t> MenuSelectResult;
	Point pt = Point::FromWord(state->r3);
	
	uint16_t menu;
	uint16_t item;
	std::tie(menu, item) = globals->ipc().PerformComplexAction<MenuSelectResult>(IPCMessage::MenuSelect, pt);
	state->r3 = (menu << 16) | (item + 1); // menus are 1-based in Classic
}

void CarbonLib_NewMenu(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_PopUpMenuSelect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetItemCmd(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetItemIcon(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetItemMark(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetItemStyle(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetMCEntries(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetMCInfo(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetMenuBar(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetMenuFlash(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetMenuItemText(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SystemEdit(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SystemMenu(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

