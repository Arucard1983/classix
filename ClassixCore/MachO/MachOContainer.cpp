//
// MachOContainer.cpp
// Classix
//

#include "MachOContainer.h"
#include <stdexcept>
#include <iostream>

namespace MachO
{
    MachOContainer::MachOContainer(Common::Allocator& allocator, const uint8_t* begin, const uint8_t* end)
        : allocator(allocator), dataBegin(begin), dataEnd(end), swapBytes(false)
    {
        if (!ValidateHeader()) {
            throw std::runtime_error("Invalid file or not a Power PC Mach-O.");
        }
        // Execute the symbols parsing
        ParseLoadCommands();
    }

    bool MachOContainer::ValidateHeader()
    {
        if (size() < sizeof(mach_header)) return false;

        const mach_header* header = reinterpret_cast<const mach_header*>(dataBegin);
        
        if (header->magic == 0xFEEDFACE) {
            swapBytes = false;
        } else if (header->magic == 0xCEFAEDFE) { 
            swapBytes = true;
        } else {
            return false;
        }

        uint32_t cpuType = Read32(header->cputype);
        if (cpuType != 18) { // 18 = CPU_TYPE_POWERPC
            std::cerr << "[MachO] Error: Unsupported Arch" << std::endl;
            return false;
        }

        return true;
    }

    void MachOContainer::ParseLoadCommands()
    {
        const mach_header* header = reinterpret_cast<const mach_header*>(dataBegin);
        uint32_t numCommands = Read32(header->ncmds);
        
        // The first command begins just after the main header
        const uint8_t* currentCmdPtr = dataBegin + sizeof(mach_header);
        
        for (uint32_t i = 0; i < numCommands; ++i) {
			// Simple out of bounds protection
			if (currentCmdPtr + sizeof(load_command) > dataEnd) break;

            const load_command* cmd = reinterpret_cast<const load_command*>(currentCmdPtr);
            uint32_t cmdType = Read32(cmd->cmd);
            uint32_t cmdSize = Read32(cmd->cmdsize);
            
            if (cmdType == LC_LOAD_DYLIB) {
                const dylib_command* dylibCmd = reinterpret_cast<const dylib_command*>(currentCmdPtr);
                uint32_t offset = Read32(dylibCmd->name_offset);
                
                // String pointer is the beginning of the command and offset
                const char* libraryPath = reinterpret_cast<const char*>(currentCmdPtr + offset);
                
                // Store the string (ex: extract "/usr/lib/libSystem.B.dylib")
                importedLibraries.push_back(std::string(libraryPath));
                
                std::cout << "[MachO Parser] Dependecy found: " << libraryPath << std::endl;
            }
            
            // next command
            currentCmdPtr += cmdSize;
        }
    }
}
