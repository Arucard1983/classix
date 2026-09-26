//
// MachOLibraryResolver.h
// Classix
//

#ifndef __Classix__MachOLibraryResolver__
#define __Classix__MachOLibraryResolver__

#include "LibraryResolver.h"
#include "SymbolResolver.h"
#include "Managers.h"
#include "NativeAllocator.h"
#include <unordered_set>
#include <string>

namespace MachO
{
    // SymbolResolver for Mach-O 
    // return 0 or the opcode NativeTag (0x4E544956) for intercepted Syscalls
    class MachOSymbolResolver : public CFM::SymbolResolver
    {
    public:
        MachOSymbolResolver() = default;
        virtual ~MachOSymbolResolver() override = default;

        // Mandatory implementation of CFM::SymbolResolver interface
        virtual uint32_t Resolve(const std::string& symbolName, CFM::SymbolClasses symbolClass) override;
    };

    // LibraryResolver  intercept dylibs loading like libSystem.B.dylib
    class MachOLibraryResolver : public CFM::LibraryResolver
    {
    private:
        Common::Allocator& allocator;
        OSEnvironment::Managers& managers;
        std::unordered_set<std::string> allowedLibraries;

    public:
        MachOLibraryResolver(Common::Allocator& allocator, OSEnvironment::Managers& managers);
        virtual ~MachOLibraryResolver() override = default;

        // Mandatory Implementation by CFM::LibraryResolver interface
        virtual CFM::SymbolResolver* Resolve(const std::string& libraryName) override;

        // Register which dylibs and frameworks accept by Mach-O environment
        void AllowLibrary(const std::string& libraryName);
    };
}

#endif /* defined(__Classix__MachOLibraryResolver__) */
