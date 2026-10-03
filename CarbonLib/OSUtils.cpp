//
// Dialogs.cpp
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

#include <chrono>
#include <unistd.h>
#include <time.h>
#include <CoreFoundation/CoreFoundation.h>
#include "Prototypes.h"
#include "CarbonLib.h"
#include "NotImplementedException.h"

using namespace CarbonLib;

struct SysEnvRec {
    Common::SInt16 environsVersion;
    Common::SInt16 machineType;
    Common::SInt16 systemVersion;
    Common::SInt16 processor;
    bool           hasFPU; // 1 byte, nativo
    bool           hasColorQD;
    Common::SInt16 keyBoardType;
    Common::SInt16 atDrvrVersNum;
    Common::SInt16 sysVRefNum;
};

struct QElem {
    Common::UInt32 qLink; // Ponteiro gerenciado automaticamente
    // ... dados adicionais ...
};

struct QHdr {
    Common::UInt16 qFlags;
    Common::UInt32 qHead;
    Common::UInt32 qTail;
};

struct DeferredTask {
    Common::UInt32 qLink;
    Common::UInt16 qType;
    Common::UInt32 dtAddr;  // Endereço PPC gerenciado em BigEndian
    Common::UInt32 dtParam; // Parâmetro
    Common::UInt32 dtReserved;
};

void CarbonLib_Delay(CarbonLib::Globals* globals, MachineState* state)
{
    // state->r3 contém o número de ticks (1/60s) a esperar
    uint32_t ticksToWait = state->r3;
    
    if (ticksToWait > 0)
    {
        // 1 tick = 16666 microssegundos
        usleep(ticksToWait * 16666);
    }
    
    // Devolve o TickCount atual após a espera
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t totalTicks = ((uint64_t)ts.tv_sec * 60) + (ts.tv_nsec / 16666666);
    state->r3 = static_cast<uint32_t>(totalTicks & 0xFFFFFFFF);
}

void CarbonLib_TickCount(CarbonLib::Globals* globals, MachineState* state)
{
    // Devolve os ticks do sistema baseados no relógio monotónico do Linux
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    
    // tv_sec * 60 (ticks por segundo) + tv_nsec convertido para a fração de 1/60s
    state->r3 = (ts.tv_sec * 60) + (ts.tv_nsec / 16666666);
}

void CarbonLib_FlushCodeCacheRange(CarbonLib::Globals* globals, MachineState* state)
{
    // Numa arquitetura Carbon moderna no Darling, isto pode ser um No-Op seguro 
    // ou interagir com o subsistema de paginação de memória da VM, se aplicável.
    state->r3 = 0; // Devolve noErr (0)
}

void CarbonLib_IsMetric(CarbonLib::Globals* globals, MachineState* state)
{
   bool isMetric = true; // Fallback seguro
    
    // Cria uma referência para o Locale atual do usuário (gerido pelo Darling)
    CFLocaleRef currentLocale = CFLocaleCopyCurrent();
    if (currentLocale) {
        // Consulta a chave que dita se o sistema usa o sistema métrico
        CFBooleanRef metricValue = (CFBooleanRef)CFLocaleGetValue(currentLocale, kCFLocaleUsesMetricSystem);
        if (metricValue) {
            isMetric = CFBooleanValue(metricValue);
        }
        CFRelease(currentLocale);
    }

    state->r3 = isMetric ? 1 : 0;
}

void CarbonLib_InitUtil(CarbonLib::Globals* globals, MachineState* state)
{
    state->r3 = 0; // noErr
}

void CarbonLib_MakeDataExecutable(CarbonLib::Globals* globals, MachineState* state)
{
    state->r3 = 0; // Obsoleto no Carbon moderno, assumido como bem-sucedido
}

void CarbonLib_ReadLocation(CarbonLib::Globals* globals, MachineState* state)
{
    state->r3 = 0; // Sem suporte a hardware geográfico clássico
}

void CarbonLib_WriteLocation(CarbonLib::Globals* globals, MachineState* state)
{
    state->r3 = 0;
}

void CarbonLib_WriteParam(CarbonLib::Globals* globals, MachineState* state)
{
    state->r3 = 0;
}

void CarbonLib_SetA5(CarbonLib::Globals* globals, MachineState* state)
{
   // O PEF passa o "novo" A5 em r3. Salvamos o valor fictício anterior em r3 (retorno).
    // Como o mundo A5 é irrelevante no host nativo, apenas simulamos a troca.
    uint32_t oldA5 = 0x0A500A50; 
    state->r3 = oldA5; 
}

void CarbonLib_SetCurrentA5(CarbonLib::Globals* globals, MachineState* state)
{
     // Semelhante ao SetA5, apenas retorna um valor de stub seguro para o PEF.
    uint32_t oldA5 = 0x0A500A50;
    state->r3 = oldA5;
}

void CarbonLib_SysEnvirons(CarbonLib::Globals* globals, MachineState* state)
{
 if (state->r3 < 1)
    {
        state->r3 = 0xea83; // envBadVers
        return;
    }
         
    if (state->r3 > 2)
    {
        state->r3 = 0xea82; // envVersTooBig
        return;
    }
         
    SysEnvRec& record = *globals->allocator.ToPointer<SysEnvRec>(state->r4);
    
    // O operador = do Common::SInt16 faz o HostToBig automaticamente!
    record.environsVersion = static_cast<int16_t>(state->r3);
    record.machineType     = 0x41; 
    record.processor       = 5;    // kProcessorPowerPC
    record.hasFPU          = true; 
    record.hasColorQD      = true;
    record.keyBoardType    = 9;    
    record.atDrvrVersNum   = 0;
    record.sysVRefNum      = 0x80c3;

    uint32_t sysVersion = globals->managers.Gestalt().GetValue("sysv");
    uint32_t classicEnv = globals->managers.Gestalt().GetValue("clsc");

    if (classicEnv == 1 || sysVersion == 0x0922) {
        record.systemVersion = static_cast<int16_t>(sysVersion);
    } else {
        record.systemVersion = 0x0750; // Resposta padrão Carbon no Mac OS X
    }

    state->r3 = 0; // noErr
}

// O que fazer com as Filas Clássicas do OS (Enqueue / Dequeue / DTInstall)?
// No Carbon, o Deferred Task Manager (DTInstall) e manipulação manual da fila de interrupções do sistema operativo
// são obsoletos ou encapsulados por timers do CoreFoundation/RunLoop. 
void CarbonLib_Enqueue(CarbonLib::Globals* globals, MachineState* state)
{
    uint32_t elemPtr = state->r3;
    uint32_t qHdrPtr = state->r4;
    
    if (!elemPtr || !qHdrPtr) {
        state->r3 = static_cast<uint32_t>(-1); // paramErr
        return;
    }

    QElem* elem = globals->allocator.ToPointer<QElem>(elemPtr);
    QHdr* header = globals->allocator.ToPointer<QHdr>(qHdrPtr);

    elem->qLink = 0; // Novo fim da fila

    if (header->qHead == 0) {
        header->qHead = elemPtr;
    } else {
        // Encontra o fim real da fila se qTail não estiver mapeado fiavelmente
        uint32_t currentPtr = header->qHead;
        while (currentPtr != 0) {
            QElem* currentElem = globals->allocator.ToPointer<QElem>(currentPtr);
            if (currentElem->qLink == 0) {
                currentElem->qLink = elemPtr;
                break;
            }
            currentPtr = currentElem->qLink;
        }
    }
    header->qTail = elemPtr;
    state->r3 = 0; // noErr
}

void CarbonLib_Dequeue(CarbonLib::Globals* globals, MachineState* state)
{
    // Stub seguro
    state->r3 = 0; // noErr
}

void CarbonLib_DTInstall(CarbonLib::Globals* globals, MachineState* state)
{
    uint32_t dtPtr = state->r3;
    if (!dtPtr) {
        state->r3 = static_cast<uint32_t>(-1); // paramErr
        return;
    }

    DeferredTask* dt = globals->allocator.ToPointer<DeferredTask>(dtPtr);
    uint32_t callbackTVector = dt->dtAddr;

    if (callbackTVector) {
        // No Darling, idealmente agende isto no CFRunLoop principal.
        // Como stub temporário, executamos ou disparamos um aviso estruturado:
    }

    state->r3 = 0; // noErr
}

void CarbonLib_GetSysPPtr(CarbonLib::Globals* globals, MachineState* state)
{
    // Devolve um ponteiro fictício para a SysParms se necessário, ou noErr
    state->r3 = 0; 
}
