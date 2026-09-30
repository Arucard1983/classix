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
#include <ffi.h>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <memory>

namespace MachO
{
    // Tipos de dados suportados pela nossa ponte dinâmica com a libffi
    enum class FFIType {
        Integer,
        Pointer,
        Double
    };

    // Metadados completos de cada símbolo intercetado no binário Mach-O
    struct NativeBridgeTarget {
        void* functionPtr;
        std::string symbolName;
        std::vector<FFIType> argTypes;
        FFIType returnType;
    };

    // O mapa global que serve de ponte entre os Stubs virtuais PPC e as funções nativas do Host
    extern std::unordered_map<uint32_t, NativeBridgeTarget> NativeBridgeMap;

    // Resolver responsável por criar stubs individuais para cada função (ex: printf, sqrt)
    class MachOSymbolResolver : public CFM::SymbolResolver
    {
    private:
        Common::Allocator& allocator;
    public:
        MachOSymbolResolver(Common::Allocator& allocator);
        virtual ~MachOSymbolResolver() override = default;

        virtual uint32_t Resolve(const std::string& symbolName, CFM::SymbolClasses symbolClass) override;
    };

    // Resolver de alto nível que interceta o carregamento de dylibs como a libSystem
    class MachOLibraryResolver : public CFM::LibraryResolver
    {
    private:
        Common::Allocator& allocator;
        OSEnvironment::Managers& managers;
        std::unordered_set<std::string> allowedLibraries;
        std::unique_ptr<MachOSymbolResolver> resolverInstance;

    public:
        MachOLibraryResolver(Common::Allocator& allocator, OSEnvironment::Managers& managers);
        virtual ~MachOLibraryResolver() override = default;

        virtual CFM::SymbolResolver* Resolve(const std::string& libraryName) override;
        void AllowLibrary(const std::string& libraryName);
    };
}

#endif /* defined(__Classix__MachOLibraryResolver__) */
