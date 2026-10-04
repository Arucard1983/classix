//
// StringCompare.cpp
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

//
// StringCompare.cpp
// Classix
//

#include "Prototypes.h"
#include "CarbonLib.h"
#include <string_view>
#include <algorithm>
#include <cstring>

// Funções auxiliares locais para extrair e comparar strings da memória emulada
namespace {
	// Compara duas Pascal Strings
	int ComparePascalStrings(CarbonLib::Globals* globals, uint32_t str1Addr, uint32_t str2Addr, bool caseInsensitive, bool ignoreDiacritics)
	{
		if (!globals->allocator.IsValidAddress(str1Addr, 1) || !globals->allocator.IsValidAddress(str2Addr, 1))
			return 0;

		uint8_t len1 = *globals->allocator.ToPointer<uint8_t>(str1Addr);
		uint8_t len2 = *globals->allocator.ToPointer<uint8_t>(str2Addr);

		if (!globals->allocator.IsValidAddress(str1Addr + 1, len1) || !globals->allocator.IsValidAddress(str2Addr + 1, len2))
			return 0;

		const char* p1 = globals->allocator.ToPointer<const char>(str1Addr + 1);
		const char* p2 = globals->allocator.ToPointer<const char>(str2Addr + 1);

		size_t minLen = std::min(len1, len2);
		for (size_t i = 0; i < minLen; ++i)
		{
			char c1 = p1[i];
			char c2 = p2[i];

			if (caseInsensitive)
			{
				c1 = std::tolower(static_cast<unsigned char>(c1));
				c2 = std::tolower(static_cast<unsigned char>(c2));
			}

			if (c1 != c2)
				return (c1 < c2) ? -1 : 1;
		}

		if (len1 != len2)
			return (len1 < len2) ? -1 : 1;

		return 0;
	}

	// Compara buffers de texto com tamanho explícito (Text / String clássico)
	int CompareTextBuffers(CarbonLib::Globals* globals, uint32_t str1Addr, uint32_t len1, uint32_t str2Addr, uint32_t len2, bool caseInsensitive)
	{
		if (!globals->allocator.IsValidAddress(str1Addr, len1) || !globals->allocator.IsValidAddress(str2Addr, len2))
			return 0;

		const char* p1 = globals->allocator.ToPointer<const char>(str1Addr);
		const char* p2 = globals->allocator.ToPointer<const char>(str2Addr);

		size_t minLen = std::min(len1, len2);
		for (size_t i = 0; i < minLen; ++i)
		{
			char c1 = p1[i];
			char c2 = p2[i];

			if (caseInsensitive)
			{
				c1 = std::tolower(static_cast<unsigned char>(c1));
				c2 = std::tolower(static_cast<unsigned char>(c2));
			}

			if (c1 != c2)
				return (c1 < c2) ? -1 : 1;
		}

		if (len1 != len2)
			return (len1 < len2) ? -1 : 1;

		return 0;
	}
}

extern "C"
{
	// EqualString e RelString usam convenções específicas de registos (antigas traps do Utilities Manager)
	void CarbonLib_EqualString(CarbonLib::Globals* globals, MachineState* state)
	{
		// r3 = str1 (Pascal), r4 = str2 (Pascal)
		// r5 = flags (Bit 31: caseInsensitive, Bit 30: diacritics)
		bool caseInsensitive = (state->r5 & 0x80000000) != 0;
		bool ignoreDiacritics = (state->r5 & 0x40000000) != 0;

		int res = ComparePascalStrings(globals, state->r3, state->r4, caseInsensitive, ignoreDiacritics);
		
		// Curiosidade histórica do Mac OS: EqualString retorna FALSE (0) se forem iguais!
		state->r3 = (res == 0) ? 0 : 1;
	}

	void CarbonLib_RelString(CarbonLib::Globals* globals, MachineState* state)
	{
		bool caseInsensitive = (state->r5 & 0x80000000) != 0;
		bool ignoreDiacritics = (state->r5 & 0x40000000) != 0;

		// Retorna -1, 0, ou 1
		state->r3 = ComparePascalStrings(globals, state->r3, state->r4, caseInsensitive, ignoreDiacritics);
	}

#pragma mark - International Utilities Manager (IU)

	void CarbonLib_IUEqualPString(CarbonLib::Globals* globals, MachineState* state)
	{
		// Devolve 0 se forem iguais, correto para a convenção IU
		state->r3 = ComparePascalStrings(globals, state->r3, state->r4, false, false);
	}

	void CarbonLib_IUEqualString(CarbonLib::Globals* globals, MachineState* state)
	{
		// r3 = str1, r4 = str2, r5 = len1, r6 = len2
		state->r3 = CompareTextBuffers(globals, state->r3, state->r5, state->r4, state->r6, false);
	}

	void CarbonLib_IUCompPString(CarbonLib::Globals* globals, MachineState* state)
	{
		state->r3 = ComparePascalStrings(globals, state->r3, state->r4, false, false);
	}

	void CarbonLib_IUCompString(CarbonLib::Globals* globals, MachineState* state)
	{
		state->r3 = CompareTextBuffers(globals, state->r3, state->r5, state->r4, state->r6, false);
	}

	void CarbonLib_IUMagPString(CarbonLib::Globals* globals, MachineState* state)
	{
		state->r3 = ComparePascalStrings(globals, state->r3, state->r4, false, false);
	}

	void CarbonLib_IUMagString(CarbonLib::Globals* globals, MachineState* state)
	{
		state->r3 = CompareTextBuffers(globals, state->r3, state->r5, state->r4, state->r6, false);
	}

	void CarbonLib_IUMagIDPString(CarbonLib::Globals* globals, MachineState* state)
	{
		// ID = Ignore Diacritics & Case
		state->r3 = ComparePascalStrings(globals, state->r3, state->r4, true, true);
	}

	void CarbonLib_IUMagIDString(CarbonLib::Globals* globals, MachineState* state)
	{
		state->r3 = CompareTextBuffers(globals, state->r3, state->r5, state->r4, state->r6, true);
	}

	void CarbonLib_IUStringOrder(CarbonLib::Globals* globals, MachineState* state)
	{
		state->r3 = CompareTextBuffers(globals, state->r3, state->r5, state->r4, state->r6, false);
	}

	void CarbonLib_IUTextOrder(CarbonLib::Globals* globals, MachineState* state)
	{
		// r3 = str1, r4 = str2, r5 = len1, r6 = len2
		state->r3 = CompareTextBuffers(globals, state->r3, state->r5, state->r4, state->r6, false);
	}

#pragma mark - Stubs de Ordenação por Script/Idioma (Casos Triviais)

	void CarbonLib_IULangOrder(CarbonLib::Globals* globals, MachineState* state)
	{
		// Determina a precedência de idiomas. Devolvemos 0 (iguais/neutro)
		state->r3 = 0;
	}

	void CarbonLib_IUScriptOrder(CarbonLib::Globals* globals, MachineState* state)
	{
		state->r3 = 0;
	}

	void CarbonLib_ScriptOrder(CarbonLib::Globals* globals, MachineState* state)
	{
		state->r3 = 0;
	}
}

