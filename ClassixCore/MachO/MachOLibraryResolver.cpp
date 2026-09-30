//
// MachOLibraryResolver.cpp
// Classix
//

#include "MachOLibraryResolver.h"
#include "MachOThreadHelper.h" // Necessário para aceder às estruturas de threads
#include <iostream>
#include <dlfcn.h>
#include <pthread.h>

namespace MachO
{
    // Definição da tabela global de pontes nativas
    std::unordered_map<uint32_t, NativeBridgeTarget> NativeBridgeMap;

    MachOSymbolResolver::MachOSymbolResolver(Common::Allocator& allocator)
        : allocator(allocator) {}

    uint32_t MachOSymbolResolver::Resolve(const std::string& symbolName, CFM::SymbolClasses symbolClass)
    {
#ifdef DEBUG_DISASSEMBLE
        std::cout << "[MachO SymbolResolver] A procurar símbolo nativo: " << symbolName << std::endl;
#endif
        
        // 1. Limpa o nome do símbolo removendo o prefixo clássico do formato Mach-O, se presente
        std::string cleanName = (!symbolName.empty() && symbolName[0] == '_') ? symbolName.substr(1) : symbolName;

        // 2. CASO ESPECIAL: Interceção preemptiva de criação de threads (pthread_create)
        if (cleanName == "pthread_create")
        {
            // Lambda estática que funciona como a nossa C-function nativa injetada via FFI
            auto pthreadCreateOverride = [](pthread_t* thread, const pthread_attr_t* attr, 
                                            void* (*start_routine)(void*), void* arg) -> int 
            {
                // NOTA: Como o interpretador universal corre no contexto global do thread atual,
                // recuperamos os ponteiros do ambiente utilizando referências válidas ou estáticas se necessário.
                // No PowerPC emulado, start_routine e arg representam endereços virtuais de 32 bits (PPC PC e PPC Arg)
                uint32_t ppcEntryPoint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(start_routine));
                uint32_t ppcArg = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(arg));

                // Acedemos ao gestor central de memória e ambiente de execução para instanciar a thread
                // Dica: Ajusta os seletores abaixo para apontar para as instâncias globais do teu VirtualMachine.cpp
                extern Common::Allocator* g_macho_allocator;
                extern OSEnvironment::Managers* g_macho_managers;
                extern PPCVM::MachineState* g_macho_main_state;

                // Captura um instantâneo (snapshot) completo dos registadores atuais da thread pai
                MachO::MachOThreadArgs* tArgs = new MachO::MachOThreadArgs{
                    *g_macho_allocator, g_macho_managers, ppcEntryPoint, ppcArg, *g_macho_main_state
                };

                // Cria o fio de execução nativo redirecionado para o nosso trampolim universal
                return ::pthread_create(thread, attr, MachO::MachOThreadTrampoline, tArgs);
            };

            NativeBridgeTarget target;
            target.functionPtr = (void*)+pthreadCreateOverride;
            target.symbolName = symbolName;
            target.returnType = FFIType::Integer;

            uint32_t stubVirtualAddr = allocator.AllocateVirtual("MachO_PthreadStub", 8);
            uint32_t* stubHostPtr = static_cast<uint32_t*>(allocator.ToPointer(stubVirtualAddr));

            const uint32_t NativeTagOpcode = 0x4E544956; // 'NTIV'
            stubHostPtr[0] = __builtin_bswap32(NativeTagOpcode);
            stubHostPtr[1] = __builtin_bswap32(stubVirtualAddr);

            NativeBridgeMap[stubVirtualAddr] = target;
            return stubVirtualAddr;
        }

        // 3. CASO GERAL: Tenta resolver qualquer outra função dinamicamente no Host (printf, sqrt, etc.)
        void* nativeFunctionPtr = dlsym(RTLD_DEFAULT, symbolName.c_str());
        
        // Se falhar com o nome original, tenta procurar no Host sem o underscore inicial
        if (!nativeFunctionPtr && !symbolName.empty() && symbolName[0] == '_') {
            nativeFunctionPtr = dlsym(RTLD_DEFAULT, cleanName.c_str());
        }

        if (!nativeFunctionPtr) {
            std::cerr << "[MachO SymbolResolver] Erro: Não foi possível mapear nativamente: " << symbolName << std::endl;
            return 0; 
        }

        // 4. Configura a estrutura FFI universal sem amarras a nomes específicos
        NativeBridgeTarget target;
        target.functionPtr = nativeFunctionPtr;
        target.symbolName = symbolName;
        target.returnType = FFIType::Integer; // Fallback padrão de retorno numérico

        // 5. Aloca o bloco do Trampolim G3 na memória virtual do PPCVM (8 bytes)
        uint32_t stubVirtualAddr = allocator.AllocateVirtual("MachOStub", 8);
        uint32_t* stubHostPtr = static_cast<uint32_t*>(allocator.ToPointer(stubVirtualAddr));

        // 6. Escreve o Opcode Mágico (NativeTag) intercetável pelo interpretador
        const uint32_t NativeTagOpcode = 0x4E544956; // 'NTIV'
        stubHostPtr[0] = __builtin_bswap32(NativeTagOpcode); // Força Big Endian para o PPC
        stubHostPtr[1] = __builtin_bswap32(stubVirtualAddr); // ID único/endereço do Stub

        // 7. Mapeia globalmente para o interpretador universal ler em Runtime
        NativeBridgeMap[stubVirtualAddr] = target;

#ifdef DEBUG_DISASSEMBLE
        std::cout << "[MachO SymbolResolver] Stub PPC criado para [" << symbolName 
                  << "] no endereço virtual: 0x" << std::hex << stubVirtualAddr << std::dec << std::endl;
#endif

        return stubVirtualAddr; 
    }

    // --- Implementação do MachOLibraryResolver ---

    MachOLibraryResolver::MachOLibraryResolver(Common::Allocator& allocator, OSEnvironment::Managers& managers)
        : allocator(allocator), managers(managers)
    {
        // Bibliotecas padrão do ecossistema macOS/Darwin a intercetar
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
            if (!resolverInstance) {
                resolverInstance = std::make_unique<MachOSymbolResolver>(allocator);
            }
            return resolverInstance.get();
        }
        return nullptr;
    }
}
