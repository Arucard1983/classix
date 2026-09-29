//
// MacErrors.cpp
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

#include <iostream>
#include <iomanip>
#include <cstdlib>
#include "Prototypes.h"

void CarbonLib_SysError(InterfaceLib::Globals* globals, MachineState* state)
{
	// Fatal error that on classic Mac OS simply frozen the system!
	int16_t errorID = static_cast<int16_t>(state->r3);

	std::cerr << "\n==================================================" << std::endl;
	std::cerr << "   FATAL: MAC OS SYSTEM PANIC (SysError / Bomb)   " << std::endl;
	std::cerr << "==================================================" << std::endl;
	std::cerr << "Program thrown SysError with ID: " << errorID << std::endl;
	
	// Tradução rápida dos códigos de bomba mais famosos do Mac OS
	switch (errorID) {
		case 1:  std::cerr << "Description: Bus Error (Invalid Memory Access)" << std::endl; break;
		case 2:  std::cerr << "Description: Address Error (Invalid Memory Adress)" << std::endl; break;
		case 3:  std::cerr << "Description: Illegal Instruction (Invalid PPC Instruction)" << std::endl; break;
		case 4:  std::cerr << "Description: Zero Divide (Numerical Exception)" << std::endl; break;
		case 9:  std::cerr << "Description: Line 1010 Trap (Application tryed to use A-Trap, which is not supported)" << std::endl; break;
		case 25: std::cerr << "Description: Memory Manager Error (Corrupted Memory Heap)" << std::endl; break;
		case 28: std::cerr << "Description: Stack Overflow (Stack Overflow Heap)" << std::endl; break;
		default: std::cerr << "Description: Unknown System Error." << std::endl; break;
	}

	// Machine State Dump for Darling debug
	std::cerr << "\n--- PPC REGISTERS AT FAILURE MOMENT ---" << std::endl;
	std::cerr << "PC:  0x" << std::hex << std::setw(8) << std::setfill('0') << state->pc << std::endl;
	std::cerr << "R3:  0x" << std::hex << std::setw(8) << std::setfill('0') << state->r3 << "  |  R4:  0x" << state->r4 << std::endl;
	std::cerr << "R1 (SP): 0x" << std::hex << std::setw(8) << std::setfill('0') << state->r1 << std::endl;
	
	std::cerr << "==================================================\n" << std::endl;

        // Controlled exiting
	globals->ipc().PerformAction<void>(IPCMessage::TerminateApplication);
	std::exit(EXIT_FAILURE);
}

