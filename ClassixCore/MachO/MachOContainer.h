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
    // Mach-O format constants
    const uint32_t LC_SEGMENT     = 0x1;
    const uint32_t LC_UNIXTHREAD  = 0x5;
    const uint32_t LC_LOAD_DYLIB  = 0xC;

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

    struct dylib_command {
        uint32_t cmd;
        uint32_t cmdsize;
        uint32_t name_offset;
        uint32_t timestamp;
        uint32_t current_version;
        uint32_t compatibility_version;
    };

    // mapping memory struct
    struct segment_command {
        uint32_t cmd;
        uint32_t cmdsize;
        char     segname[16];
        uint32_t vmaddr;
        uint32_t vmsize;
        uint32_t fileoff;
        uint32_t filesize;
        int32_t  maxprot;
        int32_t  initprot;
        uint32_t nsects;
        uint32_t flags;
    };
    
    struct section {
    char     sectname[16];
    char     segname[16];
    uint32_t addr;
    uint32_t size;
    uint32_t offset;
    uint32_t align;
    uint32_t reloff;
    uint32_t nreloc;
    uint32_t flags;
    uint32_t reserved1;
    uint32_t reserved2;
};

struct ParsedSegment {
    std::string name;
    uint32_t vmAddr;
    uint32_t vmSize;
    uint32_t fileOffset;
    uint32_t fileSize;
    uint32_t initProt;
    const uint8_t* dataPtr;
};

    class MachOContainer
    {
    private:
        Common::Allocator& allocator;
        const uint8_t* dataBegin;
        const uint8_t* dataEnd;
        bool swapBytes;
        
        std::vector<std::string> importedLibraries;
        uint32_t entryPoint; // Store the found entry point
        
        std::vector<ParsedSegment> segments;

        // Safetly load override for signed and unsigned integers
        uint32_t Read32(uint32_t value) const {
            return swapBytes ? __builtin_bswap32(value) : value;
        }
        int32_t Read32(int32_t value) const {
            return swapBytes ? static_cast<int32_t>(__builtin_bswap32(static_cast<uint32_t>(value))) : value;
        }

    public:
        MachOContainer(Common::Allocator& allocator, const uint8_t* begin, const uint8_t* end);
        
        bool ValidateHeader();
        void ParseLoadCommands();
        
        size_t size() const { return dataEnd - dataBegin; }
        
        const std::vector<std::string>& GetImportedLibraries() const { return importedLibraries; }
        uint32_t GetEntryPoint() const { return entryPoint; }
        
        const std::vector<ParsedSegment>& GetSegments() const { return segments; }
    };
}

#endif // __Classix__MachOContainer__
