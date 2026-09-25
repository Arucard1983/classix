//
// MachOContainer.h
// Classix
//

#ifndef __Classix__MachOContainer__
#define __Classix__MachOContainer__

#include <cstdint>
#include <vector>
#include <string>
#include "NativeAllocator.h"

namespace MachO
{
    //  Mach-O format constants
    const uint32_t LC_LOAD_DYLIB = 0xC;

    struct mach_header {
        uint32_t magic;
        int32_t  cputype;
        int32_t  cpusubtype;
        uint32_t filetype;
        uint32_t ncmds;
        uint32_t sizeofcmds;
        uint32_t flags;
    };

    struct load_command {
        uint32_t cmd;
        uint32_t cmdsize;
    };

    // Internal structure from a Mach-O dylib
    struct dylib_command {
        uint32_t cmd;          // LC_LOAD_DYLIB
        uint32_t cmdsize;       // Total command size including the string
        uint32_t name_offset;   // String offset from the command head
        uint32_t timestamp;
        uint32_t current_version;
        uint32_t compatibility_version;
    };

    class MachOContainer
    {
    private:
        Common::Allocator& allocator;
        const uint8_t* dataBegin;
        const uint8_t* dataEnd;
        bool swapBytes;
        
        // Lista of all imported libraries (ex: "/usr/lib/libSystem.B.dylib")
        std::vector<std::string> importedLibraries;

        uint32_t Read32(uint32_t value) const {
            if (!swapBytes) return value;
            return __builtin_bswap32(value);
        }

    public:
        MachOContainer(Common::Allocator& allocator, const uint8_t* begin, const uint8_t* end);
        
        bool ValidateHeader();
        void ParseLoadCommands();
        
        size_t size() const { return dataEnd - dataBegin; }
        
        // Expose the found libraries to the Resolver
        const std::vector<std::string>& GetImportedLibraries() const { return importedLibraries; }
    };
}
