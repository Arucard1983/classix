//
// Notification.cpp
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
#include <CoreFoundation/CoreFoundation.h>

// Auxiliary function to handle Notification pointer from PowerPC memory
struct GuestNMRec {
    uint32_t qLink; int16_t qType; int16_t nmFlags; int32_t nmPrivate;
    int16_t nmReserved; int16_t nmMark; uint32_t nmIcon; uint32_t nmSound;
    uint32_t nmStr; uint32_t nmResp; int32_t nmRefCon;
};

void CarbonLib_NMInstall(CarbonLib::Globals* globals, MachineState* state)
{
     uint32_t guestNMRecAddr = state->r3;
	if (guestNMRecAddr == 0) {
        state->r3 = -50; //paramErr
        return;
    }
	GuestNMRec* src = reinterpret_cast<GuestNMRec*>(globals->MemoryMap(guestNMRecAddr));
	std::stringstream uiMessage;
    
    // Extract the Pascal string
    if (src->nmStr) {
        uint8_t* pascalStr = reinterpret_cast<uint8_t*>(globals->MemoryMap(src->nmStr));
        uint8_t length = pascalStr[0]; 
        if (length > 0) {
            CFStringRef cfAlertStr = CFStringCreateWithBytes(
                kCFAllocatorDefault,
                &pascalStr[1],
                length,
                kCFStringEncodingMacRoman,
                false
            );

            if (cfAlertStr) {
                try {
                    std::string alertText = CFStringToStdString(cfAlertStr, kCFStringEncodingMacRoman);
                    uiMessage << alertText;
                } catch (const std::logic_error& e) {
                    uiMessage << "[Incorrect Information Text]";
                }
                CFRelease(cfAlertStr);
            }
        }
    }

    // If the Pascal string is null, then place  default one
    if (uiMessage.str().empty() && src->nmMark != 0) {
        uiMessage << "The Classic Application was requested your Attention.";
    }

	if (!uiMessage.str().empty()) {
        globals->uiChannel->PerformAction<void>(
            CarbonLib::IPCMessage::DisplayInformation, 
            uiMessage.str()
        );
    }
	state->r3 = 0;
}

void CarbonLib_NMRemove(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0; //Always null
}

