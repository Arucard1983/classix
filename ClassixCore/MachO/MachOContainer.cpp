//
// MachOContainer.cpp
// Classix
//

#include "MachOContainer.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>

namespace MachO
{
    MachOContainer::MachOContainer(Common::Allocator& allocator, const uint8_t* begin, const uint8_t* end)
        : allocator(allocator), dataBegin(begin), dataEnd(end), swapBytes(false), entryPoint(0)
    {
        if (!ValidateHeader()) {
            throw std::runtime_error("Invalid file or not a Power PC Mach-O.");
        }
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

        int32_t cpuType = Read32(header->cputype);
        if (cpuType != 18) { // 18 = CPU_TYPE_POWERPC
            std::cerr << "[MachO] Error: Unsupported Arch (Expected PowerPC 32-bit)" << std::endl;
            return false;
        }

        return true;
    }

    void MachOContainer::ParseLoadCommands()
    {
        const mach_header* header = reinterpret_cast<const mach_header*>(dataBegin);
        uint32_t numCommands = Read32(header->ncmds);
        
        const uint8_t* currentCmdPtr = dataBegin + sizeof(mach_header);
        
        for (uint32_t i = 0; i < numCommands; ++i) {
            // Bounds protection from base command
            if (currentCmdPtr + sizeof(load_command) > dataEnd) break;

            const load_command* cmd = reinterpret_cast<const load_command*>(currentCmdPtr);
            uint32_t cmdType = Read32(cmd->cmd);
            uint32_t cmdSize = Read32(cmd->cmdsize);
            
            // Advanced protection: avoid arithmetic overflows that breaks the cycle
            if (cmdSize < sizeof(load_command) || (currentCmdPtr + cmdSize) > dataEnd) {
                std::cerr << "[MachO Parser] Corrupted command size detected." << std::endl;
                break;
            }

            if (cmdType == LC_LOAD_DYLIB) {
                if (cmdSize >= sizeof(dylib_command)) {
                    const dylib_command* dylibCmd = reinterpret_cast<const dylib_command*>(currentCmdPtr);
                    uint32_t offset = Read32(dylibCmd->name_offset);
                    
                    // Wirting protection for dylib String
                    const uint8_t* stringPtr = currentCmdPtr + offset;
                    if (stringPtr >= dataBegin && stringPtr < dataEnd) {
                        // Calculate the maximum string size for our command
                        size_t maxLen = (currentCmdPtr + cmdSize) - stringPtr;
                        std::string libraryPath(reinterpret_cast<const char*>(stringPtr), std::min(strlen(reinterpret_cast<const char*>(stringPtr)), maxLen));
                        
                        importedLibraries.push_back(libraryPath);
                        std::cout << "[MachO Parser] Dependency found: " << libraryPath << std::endl;
                    }
                }
            }
            else if (cmdType == LC_SEGMENT) {
    if (cmdSize >= sizeof(segment_command)) {
        const segment_command* segCmd = reinterpret_cast<const segment_command*>(currentCmdPtr);
        
        // Read and normalize segment field with bswap if needed
        uint32_t vmaddr   = Read32(segCmd->vmaddr);
        uint32_t vmsize   = Read32(segCmd->vmsize);
        uint32_t fileoff  = Read32(segCmd->fileoff);
        uint32_t filesize = Read32(segCmd->filesize);
        int32_t  initprot = Read32(segCmd->initprot);

        // Settle segment name safely (to garantee the termination \0)
        char segNameBuf[17] = {0};
        std::memcpy(segNameBuf, &segCmd->segname, 16);
        std::string segmentName(segNameBuf);

        // Protection agianst files with corrupted offsets
        const uint8_t* segmentData = nullptr;
        if (filesize > 0) {
            if (dataBegin + fileoff + filesize <= dataEnd) {
                segmentData = dataBegin + fileoff;
            } else {
                std::cerr << "[MachO Parser] Warning: Segment " << segmentName << " out of file bounds." << std::endl;
            }
        }

        ParsedSegment segment = {
            .name = segmentName,
            .vmAddr = vmaddr,
            .vmSize = vmsize,
            .fileOffset = fileoff,
            .fileSize = filesize,
            .initProt = initprot,
            .dataPtr = segmentData
        };

                  segments.push_back(segment);

                  std::cout << "[MachO Parser] Segment Mapped: " << segmentName 
                  << " -> VM: 0x" << std::hex << vmaddr 
                  << " (Size: 0x" << vmsize << ")" << std::dec << std::endl;
              }
            }
            else if (cmdType == LC_UNIXTHREAD) {
    std::cout << "[MachO Parser] UnixThread Command detected." << std::endl;
    
    // Proteção de limites mínima para ler o cabeçalho do flavor
    if (cmdSize >= sizeof(load_command) + 8) {
        const uint32_t* threadStatePtr = reinterpret_cast<const uint32_t*>(currentCmdPtr + sizeof(load_command));
        
        uint32_t flavor = Read32(threadStatePtr[0]); // 1 = PPC_THREAD_STATE
        uint32_t count  = Read32(threadStatePtr[1]);
        
        if (flavor == 1 && count >= 40) { // PPC_THREAD_STATE tem tipicamente 40 dwords
            // O registador SRR0 (Instruction Pointer / Entry Point) é o primeiro elemento do estado
            uint32_t srr0 = Read32(threadStatePtr[2]);
            
            entryPoint = srr0;
            std::cout << "[MachO Parser] Entry Point (SRR0) found: 0x" 
                      << std::hex << entryPoint << std::dec << std::endl;
        } else {
            std::cerr << "[MachO Parser] Unsupported thread flavor (" << flavor << ") or invalid count." << std::endl;
        }
     }
   }
            
            currentCmdPtr += cmdSize;
        }
    }
}
