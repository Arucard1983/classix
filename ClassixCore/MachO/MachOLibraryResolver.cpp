//
// MachOLibraryResolver.cpp
// Classix
//

#include "MachOLibraryResolver.h"
#include <iostream>
#include <dlfcn.h>

namespace MachO
{
    // Armazena a ponte entre o endereço do stub PPC e o ponteiro nativo Intel
    // Isto será consultado mais tarde pelo interpretador PPCVM quando bater no NativeTag
    std::unordered_map<uint32_t, void*> NativeBridgeMap;

    MachOSymbolResolver::MachOSymbolResolver(Common::Allocator& allocator)
        : allocator(allocator) {}

    uint32_t MachOSymbolResolver::Resolve(const std::string& symbolName, CFM::SymbolClasses symbolClass)
    {
        std::cout << "[MachO SymbolResolver] Looking up native symbol: " << symbolName << std::endl;
        
        // 1. Tentar resolver o símbolo nativo na biblioteca atual do Host Intel
        // Nota: Para testes, procuramos globalmente (RTLD_DEFAULT). 
        // Idealmente, farias dlopen da dylib correspondente.
        void* nativeFunctionPtr = dlsym(RTLD_DEFAULT, symbolName.c_str());
        
        // O OS X costuma prefixar os símbolos com um underscore (ex: _printf). Tenta sem ele se falhar.
        if (!nativeFunctionPtr && symbolName[0] == '_') {
            nativeFunctionPtr = dlsym(RTLD_DEFAULT, symbolName.substr(1).c_str());
        }

        if (!nativeFunctionPtr) {
            std::cerr << "[MachO SymbolResolver] Error: Could not resolve nativly: " << symbolName << std::endl;
            return 0; // Símbolo não encontrado
        }

        // 2. Alocar um pequeno bloco na memória virtual do PPCVM para o Trampolim G3
        // Precisamos de 8 bytes: 4 para o NativeTag e 4 para armazenar o ID/Ponteiro
        uint32_t stubVirtualAddr = allocator.AllocateVirtual("MachOStub", 8);
        uint32_t* stubHostPtr = static_cast<uint32_t*>(allocator.ToPointer(stubVirtualAddr));

        // 3. Escrever o Opcode Mágico (NativeTag) que o PPCVM vai intercetar
        // O ClassiX usa frequentemente 0x4E544956 ('NTIV' em ASCII)
        const uint32_t NativeTagOpcode = 0x4E544956; 
        stubHostPtr[0] = __builtin_bswap32(NativeTagOpcode); // Garante Big Endian para o emulador PPC
        
        // Na segunda dword do stub, colocamos um ID único ou o próprio endereço virtual
        stubHostPtr[1] = __builtin_bswap32(stubVirtualAddr);

        // 4. Registar a associação no mapa global para o interpretador
        NativeBridgeMap[stubVirtualAddr] = nativeFunctionPtr;

        std::cout << "[MachO SymbolResolver] Created PPC Stub for " << symbolName 
                  << " at VM Address: 0x" << std::hex << stubVirtualAddr << std::dec << std::endl;

        return stubVirtualAddr; 
    }

    // --- Implementação do MachOLibraryResolver ---

    MachOLibraryResolver::MachOLibraryResolver(Common::Allocator& allocator, OSEnvironment::Managers& managers)
        : allocator(allocator), managers(managers)
    {
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
        if (allowedLibraries.find(libraryName) != allowedLibraries.end())
        {
            std::cout << "[MachO Resolver] Hijacking Mach-O library: " << libraryName << std::endl;
            
            // Instanciar o resolver correto passando o alocador
            if (!resolverInstance) {
                resolverInstance = std::make_unique<MachOSymbolResolver>(allocator);
            }
            return resolverInstance.get();
        }
        return nullptr;
    }
}
