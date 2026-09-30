//
// MachOThreadHelper.cpp
// Classix
//


#include "MachOThreadHelper.h"
#include "Interpreter.h"
#include <iostream>
#include <exception>

namespace MachO
{
    extern "C" void* MachOThreadTrampoline(void* arg)
    {
        // Garante a libertação automática da estrutura de argumentos ao sair da thread
        std::unique_ptr<MachOThreadArgs> tArgs(static_cast<MachOThreadArgs*>(arg));
        
        {
            // 1. Ativa o marcador de execução RAII do ClassixCore para registar esta thread nativa
            auto marker = tArgs->managers->ThreadManager().CreateExecutionMarker();
            
            // 2. Aloca um MachineState totalmente ISOLADO na stack desta thread do Host
            PPCVM::MachineState localState;
            
            // Copia o estado dos registadores gerais e de controlo da thread pai para herdar o contexto
            localState = tArgs->parentStateCopy;
            
            // 3. Aplica a convenção de chamadas do PowerPC para Mach-O / POSIX:
            // O registador r1 (Stack Pointer) deve manter o endereço configurado na thread pai
            // O argumento da rotina (start_routine parameter) entra obrigatoriamente em r3
            localState.r3 = tArgs->argParameter;
            
            // Ajusta o alinhamento de segurança da stack da máquina virtual (16 bytes)
            localState.r1 = (localState.r1 - 16) & ~15;

            try
            {
                // 4. Instancia um interpretador dedicado para rodar nesta thread do Host
                PPCVM::Execution::Interpreter interpreter(tArgs->allocator, localState);
                
                // Define o Link Register (LR) para o endereço de terminação automática do interpretador
                localState.lr = tArgs->allocator.ToIntPtr(interpreter.GetEndAddress());
                
                // Converte o Entry Point virtual do Guest PPC num ponteiro legível pelo emulador
                const Common::UInt32* startPC = tArgs->allocator.ToPointer<Common::UInt32>(tArgs->entryPointPC);
                
                // 5. Arranca a execução da máquina virtual em paralelo com o Host!
                interpreter.Execute(startPC);
            }
            catch (const std::exception& ex)
            {
                std::cerr << "*** [MachO Thread] Exceção na VM do Classix: " << ex.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "*** [MachO Thread] Bloqueio/Crash crítico na VM do Classix!" << std::endl;
            }
        }

        return nullptr;
    }
}
