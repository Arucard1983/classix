//
// FixMath.cpp
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

#include <cmath>
#include <cstdint>
#include "Prototypes.h"
#include "NotImplementedException.h"

//MacOS constants
#define kFixedOne      0x00010000
#define kFractOne      0x40000000
#define kFixedMax      0x7FFFFFFF
#define kFixedMin      0x80000000

void CarbonLib_Fix2Frac(CarbonLib::Globals* globals, MachineState* state)
{
	// Converte Fixed (16.16) para Fract (2.30) -> shift de 14 bits à esquerda
	int32_t x = static_cast<int32_t>(state->r3);
	state->r3 = static_cast<uint32_t>(x << 14);
}

void CarbonLib_Fix2Long(CarbonLib::Globals* globals, MachineState* state)
{
	// Converte Fixed (16.16) para Long (inteiro corrido). 
	// Adiciona 0.5 (0x8000) para arredondamento correto antes do shift.
	int32_t x = static_cast<int32_t>(state->r3);
	if (x >= 0) {
		state->r3 = (x + 0x8000) >> 16;
	} else {
		state->r3 = -(( -x + 0x8000) >> 16);
	}
}

void CarbonLib_Fix2X(CarbonLib::Globals* globals, MachineState* state)
{
	// No Carbon, o 'X' (Extended) passa a ser processado nos registadores de FPU ou via ponteiro em r4.
	// Assumindo passagem padrão Carbon por ponteiro para double em r4:
	double* destExtended = globals->allocator.ToPointer<double>(state->r4);
	if (destExtended) {
		*destExtended = static_cast<int32_t>(state->r3) / 65536.0;
	}
}

void CarbonLib_FixATan2(CarbonLib::Globals* globals, MachineState* state)
{
	// Arcotangente de r3/r4 (num/den em Fixed). Devolve o ângulo em Fixed (radianos).
	double y = static_cast<int32_t>(state->r3) / 65536.0;
	double x = static_cast<int32_t>(state->r4) / 65536.0;
	double atan2Val = std::atan2(y, x);
	state->r3 = static_cast<uint32_t>(atan2Val * 65536.0);
}

void CarbonLib_FixDiv(CarbonLib::Globals* globals, MachineState* state)
{
	// Divisão Fixed / Fixed. r3 = numerados, r4 = denominador
	int64_t num = static_cast<int32_t>(state->r3);
	int64_t den = static_cast<int32_t>(state->r4);
	
	if (den == 0) {
		state->r3 = (num >= 0) ? kFixedMax : kFixedMin; // Proteção contra divisão por zero
		return;
	}
	
	int64_t result = (num << 16) / den;
	state->r3 = static_cast<uint32_t>(result);
}

void CarbonLib_FixMul(CarbonLib::Globals* globals, MachineState* state)
{
	// Multiplicação Fixed * Fixed. r3 = a, r4 = b
	int64_t a = static_cast<int32_t>(state->r3);
	int64_t b = static_cast<int32_t>(state->r4);
	
	int64_t result = (a * b + 0x8000) >> 16;
	state->r3 = static_cast<uint32_t>(result);
}

void CarbonLib_FixRatio(CarbonLib::Globals* globals, MachineState* state)
{
	// Multiplica dois inteiros de 16-bits (r3 e r4) e devolve um resultado Fixed (16.16)
	int16_t count = static_cast<int16_t>(state->r3);
	int16_t shortInm = static_cast<int16_t>(state->r4);
	
	int32_t result = static_cast<int32_t>(count) << 16;
	state->r3 = static_cast<uint32_t>(result / shortInm);
}

void CarbonLib_FixRound(CarbonLib::Globals* globals, MachineState* state)
{
	// Arredonda um Fixed para o inteiro mais próximo, mantendo o formato Fixed (.0000)
	int32_t x = static_cast<int32_t>(state->r3);
	if (x >= 0) {
		state->r3 = static_cast<uint32_t>(((x + 0x8000) >> 16) << 16);
	} else {
		state->r3 = static_cast<uint32_t>(-((( -x + 0x8000) >> 16) << 16));
	}
}

void CarbonLib_Frac2Fix(CarbonLib::Globals* globals, MachineState* state)
{
	// Converte Fract (2.30) para Fixed (16.16) -> shift de 14 bits à direita com sinal
	int32_t x = static_cast<int32_t>(state->r3);
	if (x >= 0) {
		state->r3 = (x + 0x2000) >> 14; // Arredondamento
	} else {
		state->r3 = -(( -x + 0x2000) >> 14);
	}
}

void CarbonLib_Frac2X(CarbonLib::Globals* globals, MachineState* state)
{
	double* destExtended = globals->allocator.ToPointer<double>(state->r4);
	if (destExtended) {
		*destExtended = static_cast<int32_t>(state->r3) / 1073741824.0;
	}
}

void CarbonLib_FracCos(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 contém um Fixed em radianos. Devolve um Fract.
	double angle = static_cast<int32_t>(state->r3) / 65536.0;
	double cosVal = std::cos(angle);
	state->r3 = static_cast<uint32_t>(cosVal * 1073741824.0);
}

void CarbonLib_FracDiv(CarbonLib::Globals* globals, MachineState* state)
{
	// Divisão Fract / Fract
	int64_t num = static_cast<int32_t>(state->r3);
	int64_t den = static_cast<int32_t>(state->r4);
	
	if (den == 0) {
		state->r3 = (num >= 0) ? kFixedMax : kFixedMin;
		return;
	}
	
	int64_t result = (num << 30) / den;
	state->r3 = static_cast<uint32_t>(result);
}

void CarbonLib_FracMul(CarbonLib::Globals* globals, MachineState* state)
{
	// Multiplicação Fract * Fract. Escala de 30 bits
	int64_t a = static_cast<int32_t>(state->r3);
	int64_t b = static_cast<int32_t>(state->r4);
	
	int64_t result = (a * b + 0x20000000) >> 30;
	state->r3 = static_cast<uint32_t>(result);
}

void CarbonLib_FracSin(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 contém um Fixed que representa o ângulo em radianos. Devolve um Fract.
	double angle = static_cast<int32_t>(state->r3) / 65536.0;
	double sinVal = std::sin(angle);
	state->r3 = static_cast<uint32_t>(sinVal * 1073741824.0); // 2^30
}

void CarbonLib_FracSqrt(CarbonLib::Globals* globals, MachineState* state)
{
	/ Raiz quadrada de um Fract (r3). Devolve um Fract.
	double x = static_cast<int32_t>(state->r3) / 1073741824.0;
	if (x < 0.0) x = 0.0;
	double sqrtVal = std::sqrt(x);
	state->r3 = static_cast<uint32_t>(sqrtVal * 1073741824.0);
}

void CarbonLib_Long2Fix(CarbonLib::Globals* globals, MachineState* state)
{
	// Converte Long para Fixed (16.16)
	int32_t x = static_cast<int32_t>(state->r3);
	state->r3 = static_cast<uint32_t>(x << 16);
}

void CarbonLib_X2Fix(CarbonLib::Globals* globals, MachineState* state)
{
	// Lê o double do ponteiro r3 e converte para Fixed em r3.
	double* srcExtended = globals->allocator.ToPointer<double>(state->r3);
	if (srcExtended) {
		state->r3 = static_cast<uint32_t>(*srcExtended * 65536.0);
	} else {
		state->r3 = 0;
	}
}

void CarbonLib_X2Frac(CarbonLib::Globals* globals, MachineState* state)
{
	double* srcExtended = globals->allocator.ToPointer<double>(state->r3);
	if (srcExtended) {
		state->r3 = static_cast<uint32_t>(*srcExtended * 1073741824.0);
	} else {
		state->r3 = 0;
	}
}

