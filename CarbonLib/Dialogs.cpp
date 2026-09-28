//
// Dialogs.cpp
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

#include <sstream>
#include <algorithm>
#include "Prototypes.h"
#include "NotImplementedException.h"
#include "CarbonLib.h"
#include "ResourceTypes.h"
#include "Todo.h"

using namespace CarbonLib;
using namespace CarbonLib::Resources;

void CarbonLib_Alert(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_AppendDITL(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CautionAlert(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CloseDialog(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_CountDITL(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DialogCopy(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DialogCut(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DialogDelete(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DialogPaste(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DialogSelect(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DisposeDialog(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_DrawDialog(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ErrorSound(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_FindDialogItem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetAlertStage(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetDialogItem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetDialogItemText(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_GetNewDialog(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
	
	// tentative implementation: I don't feel like going through dialogs right now but I don't feel like removing
	// this, either
	/*
	uint16_t resourceId = static_cast<uint16_t>(state->r3);
	if (DLOG* dialog = globals->resources.GetResource<DLOG>(resourceId))
	{
		uint32_t portAddress = state->r4;
		
		std::string title = dialog->GetTitle();
		Rect rect = dialog->rect;
		
		UGrafPort* port;
		if (portAddress == 0)
		{
			std::stringstream ss;
			ss << "Dialog: \"" << title << "\"";
			port = &globals->grafPorts.AllocateColorGrafPort(rect, nullptr, title);
			portAddress = globals->allocator.ToIntPtr(port);
		}
		else
		{
			port = globals->allocator.ToPointer<UGrafPort>(portAddress);
		}
		
		bool visible = dialog->visibility == 1;
		globals->ipc().PerformAction<void>(IPCMessage::CreateDialog, portAddress, rect, visible, title);
	}
	 */
}

void CarbonLib_GetStdFilterProc(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_HideDialogItem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_InitDialogs(CarbonLib::Globals* globals, MachineState* state)
{
	globals->systemFatalErrorHandler = state->r3;
}

void CarbonLib_IsDialogEvent(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ModalDialog(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_NewColorDialog(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_NewDialog(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_NoteAlert(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ParamText(CarbonLib::Globals* globals, MachineState* state)
{
	for (size_t i = 0; i < globals->dialogParams.size(); i++)
	{
		const char* pascalString = globals->allocator.ToPointer<char>(state->gpr[3 + i]);
		uint8_t length = *pascalString;
		pascalString++;
		
		globals->dialogParams[i] = std::string(pascalString, pascalString + length);
	}
}

void CarbonLib_ResetAlertStage(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SelectDialogItemText(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetDialogCancelItem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetDialogDefaultItem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetDialogFont(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetDialogItem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetDialogItemText(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_SetDialogTracksCursor(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ShortenDITL(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_ShowDialogItem(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_StdFilterProc(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

void CarbonLib_StopAlert(CarbonLib::Globals* globals, MachineState* state)
{
	TODO("StopAlert really has a nasty and incomplete implementation");
	uint32_t key = 0;
	uint16_t templateKey = static_cast<uint16_t>(state->r3);
	
	Resources::ALRT* alert = globals->resources().GetResource<Resources::ALRT>(templateKey);
	globals->ipc().PerformAction<void>(IPCMessage::CreateDialog, key, alert->bounds, true, std::string("Stop!"));
	
	Resources::DITL* ditl = globals->resources().GetResource<Resources::DITL>(alert->ditl);
	auto enumerator = ditl->EnumerateControls();
	std::string paramString = "^0";
	while (enumerator.HasItem())
	{
		Control control = enumerator.GetControl();
		// replace string param placeholders
		for (size_t i = 0; i < globals->dialogParams.size(); i++)
		{
			paramString[1] = static_cast<decltype(paramString)::value_type>('0' + i);
			std::string::size_type paramPosition = control.label.find(paramString);
			while (paramPosition != std::string::npos)
			{
				control.label.replace(paramPosition, paramString.length(), globals->dialogParams[i]);
				paramPosition = control.label.find(paramString, paramPosition + globals->dialogParams[i].length());
			}
		}
		
		globals->ipc().PerformAction<void>(IPCMessage::CreateControl, key, control.type, control.enabled, control.bounds, control.label);
		enumerator.MoveNext();
	}
	
	// only return when the dialog is closed
	throw PPCVM::NotImplementedException("Alerts not really implemented");
}

void CarbonLib_UpdateDialog(CarbonLib::Globals* globals, MachineState* state)
{
	throw PPCVM::NotImplementedException(__func__);
}

