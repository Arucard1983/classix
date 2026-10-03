//
// ToolUtils.cpp
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
#include "CarbonLib.h"

// Helper interno para localizar o byte e a máscara de bits correta
// O Mac OS clássico conta os bits de 0 (MSB, 0x80) a 7 (LSB, 0x01)
static inline void GetBitPtrAndMask(CarbonLib::Globals* globals, uint32_t baseAddr, int32_t bitNum, uint8_t*& bytePtr, uint8_t& mask)
{
    // O índice do bit pode ser superior a 7 (pode apontar para bytes seguintes ou anteriores)
    int32_t byteOffset = bitNum >> 3;      // bitNum / 8
    int32_t bitInByte = bitNum & 7;        // bitNum % 8
    
    // Converte o endereço base da VM PPC adicionando o offset de bytes
    bytePtr = globals->allocator.ToPointer<uint8_t>(baseAddr + byteOffset);
    
    // Máscara Big-Endian: Bit 0 é 0x80, Bit 7 é 0x01
    mask = static_cast<uint8_t>(0x80 >> bitInByte);
}

void CarbonLib_BitAnd(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = state->r3 & state->r4;
}

void CarbonLib_BitClr(CarbonLib::Globals* globals, MachineState* state)
{
	uint8_t* bytePtr;
    uint8_t mask;
    GetBitPtrAndMask(globals, state->r3, static_cast<int32_t>(state->r4), bytePtr, mask);
    *bytePtr &= ~mask; // Desativa o bit
}

void CarbonLib_BitNot(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = ~state->r3;
}

void CarbonLib_BitOr(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = state->r3 | state->r4;
}

void CarbonLib_BitSet(CarbonLib::Globals* globals, MachineState* state)
{
	uint8_t* bytePtr;
    uint8_t mask;
    GetBitPtrAndMask(globals, state->r3, static_cast<int32_t>(state->r4), bytePtr, mask);
    *bytePtr |= mask; // Ativa o bit
}

void CarbonLib_BitShift(CarbonLib::Globals* globals, MachineState* state)
{
    int32_t count = static_cast<int32_t>(state->r4);
    // Se count for positivo, roda para a esquerda. Se for negativo, para a direita.
    if (count > 0) {
        state->r3 = state->r3 << count;
    } else if (count < 0) {
        state->r3 = state->r3 >> (-count);
    }
}

void CarbonLib_BitTst(CarbonLib::Globals* globals, MachineState* state)
{
	uint8_t* bytePtr;
    uint8_t mask;
    GetBitPtrAndMask(globals, state->r3, static_cast<int32_t>(state->r4), bytePtr, mask);
    
    // Devolve Boolean (1 se ativo, 0 se inativo) no registo de retorno r3
    state->r3 = ((*bytePtr & mask) != 0) ? 1 : 0;
}

void CarbonLib_BitXor(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = state->r3 ^ state->r4;
}

