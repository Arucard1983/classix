//
// MachOLibraryResolver.cpp
// Classix
//

#include "MachOLibraryResolver.h"
#include <iostream>

namespace MachO
{
    MachOLibraryResolver::MachOLibraryResolver(Common::Allocator& allocator, OSEnvironment::Managers& managers)
        : allocator(allocator), managers(managers)
    {
        // Por default, the libSystem is loaded to Darling handle it
        AllowLibrary("/usr/lib/libSystem.B.dylib");
        AllowLibrary("libSystem.B.dylib");
        AllowLibrary("libSystem");
    }

    void MachOLibraryResolver::AllowLibrary(const std::string& libraryName)
    {
        allowedLibraries.insert(libraryName);
    }

    CFM::SymbolResolver* MachOLibraryResolver::Resolve(const std::string& libraryName)
    {
        // If the library is found from the list
        if (allowedLibraries.find(libraryName) != allowedLibraries.end())
        {
            std::cout << "[MachO Resolver] Mapping library: " << libraryName << " to native libSystem." << std::endl;
            
            // TODO: Native library loading
            
            return nullptr; 
        }

        // If it is an unkown library, it should br dispatched by a dummy resolver.(ex: DummyResolver)
        return nullptr;
    }
}
