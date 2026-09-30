//
// MachOLibraryResolver.h
// Classix
//

#ifndef __Classix__MachOLibraryResolver__
#define __Classix__MachOLibraryResolver__

#include <ffi.h> // Infraestrutura da libffi
#include <map>

namespace MachO
{
    enum class SymbolType { Integer, Float };

    struct NativeBridgeTarget {
        void* functionPtr;
        SymbolType type;
    };

    extern std::unordered_map<uint32_t, NativeBridgeTarget> NativeBridgeMap;
     // Mapa global externo que o teu interpretador PPCVM vai ler ao intercetar a Trap
    //extern std::unordered_map<uint32_t, void*> NativeBridgeMap;

    class MachOSymbolResolver : public CFM::SymbolResolver
    {
    private:
        Common::Allocator& allocator;
    public:
        MachOSymbolResolver(Common::Allocator& allocator);
        virtual ~MachOSymbolResolver() override = default;

        virtual uint32_t Resolve(const std::string& symbolName, CFM::SymbolClasses symbolClass) override;
    };

    class MachOLibraryResolver : public CFM::LibraryResolver
    {
    private:
        Common::Allocator& allocator;
        OSEnvironment::Managers& managers;
        std::unordered_set<std::string> allowedLibraries;
        std::unique_ptr<MachOSymbolResolver> resolverInstance; // Gestão limpa da instância

    public:
        MachOLibraryResolver(Common::Allocator& allocator, OSEnvironment::Managers& managers);
        virtual ~MachOLibraryResolver() override = default;

        virtual CFM::SymbolResolver* Resolve(const std::string& libraryName) override;
        void AllowLibrary(const std::string& libraryName);
    };
}

#endif /* defined(__Classix__MachOLibraryResolver__) */
