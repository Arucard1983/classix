//
// MachOLibraryResolver.cpp
// Classix
//

#include "MachOLibraryResolver.h"
#include <iostream>

namespace MachO
{
    // A simpler SymbolResolver for Mach-O that uses ClassiX conventions
    class MachOSymbolResolver : public CFM::SymbolResolver
    {
    public:
        MachOSymbolResolver() = default;
        virtual ~MachOSymbolResolver() = default;

        // If the target binary ask for a symbol (ex: _printf),
        // from a library, then return the virtual adress.
        // If it is a syscall that uses NativeCall, place NativeTag here.
        virtual uint32_t Resolve(const std::string& symbolName, CFM::SymbolClasses symbolClass) override
        {
            std::cout << "[MachO SymbolResolver] Looking up symbol: " << symbolName << std::endl;
             
            // TODO: SymTab
            return 0; 
        }
    };


    MachOLibraryResolver::MachOLibraryResolver(Common::Allocator& allocator, OSEnvironment::Managers& managers)
        : allocator(allocator), managers(managers)
    {
        // Base Mach-O Libraries accepted by Super-Rosetta/Darling environment
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
        // Check if the native host library exists
        if (allowedLibraries.find(libraryName) != allowedLibraries.end())
        {
            std::cout << "[MachO Resolver] Acknowledged Mach-O library: " << libraryName << std::endl;
            
            // Select the library to the target Mach-O.
            // This will avoid calling DummyLibraryResolver from original ClassiX
            static auto instance = std::make_unique<MachOSymbolResolver>();
            return instance.get();
        }

        // Unkown symbol that can be handled by DummyResolver
        return nullptr;
    }
}
