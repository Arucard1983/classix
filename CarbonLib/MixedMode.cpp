//
// MixedMode.cpp
// Classix
//
// Copyright (C) 2013 Félix Cloutier
//
// This file is part of Classix.
//
// Classix is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// Classix is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// Classix. If not, see http://www.gnu.org/licenses/.
//

#include "Prototypes.h"
#include "NotSupportedException.h"

// Estrutura clássica de um registo de rotina (Mixed Mode Manager)
struct RoutineRecord {
    Common::UInt32 procInfo;   // Informações de assinatura da função
    Common::UInt32 reserved1;
    Common::UInt8  ISA;        // 0 = 68k, 1 = PowerPC
    Common::UInt8  routineFlags;
    Common::UInt32 procDescriptor; // O endereço real (TVector PPC) <--- O que nos interessa!
};

// Estrutura do Descritor de Rotina que o Carbon espera receber de volta
struct RoutineDescriptor {
    Common::UInt16 goMixedModeTrap; // Trap clássica do 68k (0xAAFE)
    Common::UInt8  version;
    Common::UInt8  routineFlags;
    Common::UInt32 reserved1;
    Common::UInt32 reserved2;
    RoutineRecord  localRecord;     // O registo que descreve a função PPC
};

void CarbonLib_NewFatRoutineDescriptor(InterfaceLib::Globals* globals, MachineState* state)
{
    // state->r3 = Ponteiro para o RoutineRecord original (ou dados de inicialização)
    uint32_t recordPtr = state->r3;
    
    if (!recordPtr || !globals->allocator.IsValidAddress(recordPtr, sizeof(RoutineRecord))) {
        state->r3 = 0; // Retorna NULL se o ponteiro for inválido
        return;
    }

    RoutineRecord* srcRecord = globals->allocator.ToPointer<RoutineRecord>(recordPtr);

    // Alocamos um RoutineDescriptor persistente na memória para servir como UPP (Universal Procedure Pointers)
    RoutineDescriptor* uPP = globals->allocator.Allocate<RoutineDescriptor>("CarbonLib UPP (RoutineDescriptor)");
    
    // Configuramos o cabeçalho padrão esperado pelo interpretador/Carbon
    uPP->goMixedModeTrap = 0xAAFE; // Trap mágica histórica
    uPP->version         = 7;      // Versão estável do Mixed Mode Manager
    uPP->routineFlags    = 0;
    uPP->reserved1       = 0;
    uPP->reserved2       = 0;

    // Clonamos o registo de rotina aplicando a tipagem automática de Endianness do ClassiX
    uPP->localRecord.procInfo       = srcRecord->procInfo;
    uPP->localRecord.reserved1      = 0;
    uPP->localRecord.ISA            = 1; // Forçamos sempre 1 (kPowerPCISA), ignorando qualquer pedido de 68k!
    uPP->localRecord.routineFlags   = srcRecord->routineFlags;
    uPP->localRecord.procDescriptor = srcRecord->procDescriptor; // Endereço do código PPC (TVector)

    // Devolvemos o endereço do uPP gerado no registrador r3.
    // Sempre que a sua Toolbox nativa (ou libffi) precisar de disparar um callback para o PEF,
    // ela pode ler este UPP, verificar que o ISA é PPC e saltar diretamente para o procDescriptor!
    state->r3 = globals->allocator.ToIntPtr(uPP);
}
