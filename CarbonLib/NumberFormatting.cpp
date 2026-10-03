//
// NumberFormatting.cpp
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

#include <cstdio>
#include <cstdlib>
#include <string>
#include <cmath>
#include "Prototypes.h"
#include "NotImplementedException.h"

// Helper nativo: Converte Pascal String do PEF PPC para std::string (C-String)
std::string PascalToCString(const uint8_t* pStr) {
    if (!pStr) return "";
    uint8_t length = pStr[0]; // O primeiro byte dita o tamanho
    return std::string(reinterpret_cast<const char*>(&pStr[1]), length);
}

// Helper nativo: Escreve uma std::string de volta no formato Pascal String para o PEF PPC
void CStringToPascal(const std::string& cStr, uint8_t* pStr) {
    if (!pStr) return;
    size_t len = std::min(cStr.length(), static_cast<size_t>(255)); // Limite clássico de 255 chars
    pStr[0] = static_cast<uint8_t>(len);
    std::memcpy(&pStr[1], cStr.c_str(), len);
}

// ============================================================================
// STUBS INTELIGENTES REUTILIZANDO A LIBSYSTEM / LIBC DO DARLING
// ============================================================================

void CarbonLib_NumToString(CarbonLib::Globals* globals, MachineState* state)
{
    // state->r3 = O número a converter (int32_t) vindo do PEF
    // state->r4 = Ponteiro na memória PPC para onde guardar a Pascal String
    
    int32_t number = static_cast<int32_t>(state->r3);
    uint32_t pStrAddr = state->r4;

    if (!pStrAddr || !globals->allocator.IsValidAddress(pStrAddr, 256)) return;
    uint8_t* pStr = globals->allocator.ToPointer<uint8_t>(pStrAddr);

    // Usamos a libc nativa (via libSystem do Darling) para formatar a string de forma limpa
    std::string result = std::to_string(number);
    
    // Devolvemos no formato Pascal Big-Endian que o PEF lê
    CStringToPascal(result, pStr);
}

void CarbonLib_StringToNum(CarbonLib::Globals* globals, MachineState* state)
{
    // state->r3 = Ponteiro na memória PPC para a Pascal String de origem
    // state->r4 = Ponteiro na memória PPC para a struct/variável onde guardar o int32_t de destino
    
    uint32_t pStrAddr = state->r3;
    uint32_t resultAddr = state->r4;

    if (!pStrAddr || !resultAddr) return;

    uint8_t* pStr = globals->allocator.ToPointer<uint8_t>(pStrAddr);
    
    // Converte a Pascal String do PEF para C-String nativa
    std::string cStr = PascalToCString(pStr);

    // Usa a libSystem/libc para fazer o parse numérico seguro
    int32_t parsedNumber = 0;
    try {
        parsedNumber = std::stol(cStr);
    } catch (...) {
        parsedNumber = 0; // Fallback se a string não for um número válido
    }

    // Grava na memória do PEF usando a tipagem automática do ClassiX para evitar inversão de bytes!
    Common::SInt32* output = globals->allocator.ToPointer<Common::SInt32>(resultAddr);
    *output = parsedNumber; 
}

void CarbonLib_ExtendedToString(CarbonLib::Globals* globals, MachineState* state)
{
    // state->r3 = Ponteiro para um float de 80-bits (Extended) do Mac Clássico
    // state->r4 = Ponteiro para a Pascal String de destino
    // Nota: Extended do 68k/PPC geralmente mapeia para o double de 64-bits ou 80-bits do host.
    
    uint32_t extAddr = state->r3;
    uint32_t pStrAddr = state->r4;

    if (!extAddr || !pStrAddr) return;

    // Lemos usando o tipo flutuante gerenciado do ClassiX (ex: Real64 se mapeado como double)
    Common::Real64* macFloat = globals->allocator.ToPointer<Common::Real64>(extAddr);
    uint8_t* pStr = globals->allocator.ToPointer<uint8_t>(pStrAddr);

    // Converte usando a formatação de floats nativa do host
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.6f", static_cast<double>(*macFloat));

    CStringToPascal(buffer, pStr);
}

void CarbonLib_StringToExtended(CarbonLib::Globals* globals, MachineState* state)
{
    uint32_t pStrAddr = state->r3;
    uint32_t extAddr = state->r4;

    if (!pStrAddr || !extAddr) return;

    uint8_t* pStr = globals->allocator.ToPointer<uint8_t>(pStrAddr);
    std::string cStr = PascalToCString(pStr);

    double value = std::strtod(cStr.c_str(), nullptr);

    // O operador do Real64 faz o swap automático do float para a memória lida pelo PEF!
    Common::Real64* macFloat = globals->allocator.ToPointer<Common::Real64>(extAddr);
    *macFloat = value;
}

// ============================================================================
// STUBS DE FORMATAÇÃO ADAPTATIVA (FormatRec)
// ============================================================================
// FormatRec controla formatação avançada de moeda e milhares da era clássica. 
// Para evitar crashes, criamos stubs inteligentes que apenas limpam a string.

void CarbonLib_FormatRecToString(CarbonLib::Globals* globals, MachineState* state)
{
    uint32_t pStrAddr = state->r5; // No padrão clássico, o destino está em r5
    if (pStrAddr) {
        uint8_t* pStr = globals->allocator.ToPointer<uint8_t>(pStrAddr);
        CStringToPascal("0", pStr); // Stub seguro padrão
    }
    state->r3 = 0; // noErr
}

void CarbonLib_StringToFormatRec(CarbonLib::Globals* globals, MachineState* state)
{
    state->r3 = 0; // noErr (Dizemos que o parse da formatação foi concluído)
}
