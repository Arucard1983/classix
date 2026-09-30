//
// MachOThreadHelper.h
// Classix
//

#ifndef __Classix__MachOThreadHelper__
#define __Classix__MachOThreadHelper__

#include "MachineState.h"
#include "Managers.h"
#include "NativeAllocator.h"

namespace MachO
{
    // Estrutura auxiliar para transportar os argumentos do Guest PPC para a nova thread nativa do Host
    struct MachOThreadArgs
    {
        Common::Allocator& allocator;
        OSEnvironment::Managers* managers;
        uint32_t entryPointPC;
        uint32_t argParameter;
        PPCVM::MachineState parentStateCopy; // Cópia do estado dos registadores da thread pai
    };

    // O Trampolim nativo C-linkage que será invocado pelo pthread_create do Host
    extern "C" void* MachOThreadTrampoline(void* arg);
}

#endif // defined(__Classix__MachOThreadHelper__)
