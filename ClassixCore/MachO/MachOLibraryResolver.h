//
// MachOLibraryResolver.h
// Classix
//

#ifndef __Classix__MachOLibraryResolver__
#define __Classix__MachOLibraryResolver__

#include "LibraryResolver.h"
#include "Managers.h"
#include "NativeAllocator.h"
#include <unordered_set>
#include <string>

namespace MachO
{
    class MachOLibraryResolver : public CFM::LibraryResolver
    {
    private:
        Common::Allocator& allocator;
        OSEnvironment::Managers& managers;
        std::unordered_set<std::string> allowedLibraries;

    public:
        MachOLibraryResolver(Common::Allocator& allocator, OSEnvironment::Managers& managers);
        virtual ~MachOLibraryResolver() = default;

        // Mandatory method from interface CFM::LibraryResolver
        virtual CFM::SymbolResolver* Resolve(const std::string& libraryName) override;

        // Enables to setup which dylibs ot frameworks are accepted
        void AllowLibrary(const std::string& libraryName);
    };
}

#endif /* defined(__Classix__MachOLibraryResolver__) */

