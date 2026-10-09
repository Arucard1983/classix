//
// MachineState.cpp
// Classix
//
// Copyright (C) 2012 Félix Cloutier
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

#include "MachineState.h"
#include <cstring>
#include <chrono>

namespace
{
	uint64_t GetElapsedNanos()
	{
		// Garante a extração estável da contagem do relógio monolítico de alta resolução do Host
		return static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	}
	
	const uint64_t kNanosInSeconds = 1000000000ULL;
}

namespace PPCVM
{
	MachineState::MachineState()
	{
		// Substituição do memset bruto por inicialização segura campo a campo.
		// Isto protege o alinhamento estrito de 16 bytes exigido pelos registadores AltiVec (vpr).
		std::memset(gpr, 0, sizeof(gpr));
		std::memset(fpr, 0, sizeof(fpr));
		std::memset(cr, 0, sizeof(cr));
		std::memset(vpr, 0, sizeof(vpr));
		
		xer = 0;
		lr = 0;
		ctr = 0;
		pc = 0;
		vscr = 0;
		fpscr.hex = 0;
	}

	void MachineState::SetCR(uint32_t value)
	{
		// Desempacota o registador Condition Register (CR) de 32-bits da VM nos 8 nibbles (CR0-CR7)
		for (int i = 0; i < 8; i++)
		{
			cr[i] = static_cast<uint8_t>((value >> (28 - i * 4)) & 0xF);
		}
	}

	uint32_t MachineState::GetCR() const
	{
		// CORREÇÃO CRÍTICA: Inicializa a variável estritamente a zero para limpar lixo residual.
		// Removida a atribuição redundante pré-loop que causava sobreposição no CR0.
		uint32_t crValue = 0;
		for (int i = 0; i < 8; i++)
		{
			crValue |= (static_cast<uint32_t>(cr[i] & 0xF) << (28 - i * 4));
		}
		
		return crValue;
	}
	
	uint32_t MachineState::GetRTCU() const
	{
		// Retorna a parte superior do Time Base (Segundos decorridos)
		return static_cast<uint32_t>(GetElapsedNanos() / kNanosInSeconds);
	}
	
	uint32_t MachineState::GetRTCL() const
	{
		// Retorna a parte inferior do Time Base (Nanossegundos residuais dentro do segundo atual)
		// Forçado o cast estável de 64-bits para garantir que a aritmética do módulo não trunca dados
		return static_cast<uint32_t>(GetElapsedNanos() % kNanosInSeconds);
	}
}
