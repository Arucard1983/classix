//
// MathLib.cpp
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

#include <cmath>
#include <dlfcn.h>
#include <fenv.h>
#include "MathLib.h"
#include "MathLibFunctions.h"
#include "MachineState.h"
#include "BigEndian.h"

#pragma pack(push, 2) // O Mac OS clássico alinha estas estruturas a 2 bytes (mac68k/PPC)
struct DecForm
{
    Common::SInt16 style; // 0 = Float (Exponencial), 1 = Fixed (Fixa)
    Common::SInt16 digits;
};

struct Decimal
{
    Common::SInt8 sgn;    // 0 para positivo, 1 para negativo
    Common::SInt8 unused; 
    Common::SInt16 exp;   // Expoente de base 10
    uint8_t sig[32];      // String Pascal contendo os dígitos (sig[0] é o tamanho)
};
#pragma pack(pop)

static inline int32_t MapFPClassify(double value)
{
    int hostClass = std::fpclassify(value);
    switch (hostClass)
    {
        case FP_NAN:
            // No Mac OS, distinguia-se Silent de Signaling NaN. 
            // Como aproximação segura, tratamos como Quiet NaN (2).
            return 2; 
        case FP_INFINITE:  return 3;
        case FP_NORMAL:    return 4;
        case FP_SUBNORMAL: return 5;
        case FP_ZERO:      return 6;
        default:           return 4; // Fallback para normal
    }
}

namespace MathLib
{
	struct Globals
	{
		Common::Allocator& allocator;
		uint32_t FE_DFL_ENV;
		Common::Real64 pi;
		
		Globals(Common::Allocator& allocator)
		: allocator(allocator)
		{
			pi = M_PI;
			FE_DFL_ENV = 0;
		}
	};
}

using PPCVM::MachineState;
using namespace MathLib;

const std::string piName = "pi";
const std::string envName = "_FE_DFL_ENV";

extern "C"
{
	Globals* LibraryLoad(Common::Allocator* allocator, OSEnvironment::Managers* managers)
	{
		return allocator->Allocate<Globals>("MathLib Globals", *allocator);
	}
	
	SymbolType LibraryLookup(Globals* globals, const char* name, void** result)
	{
		if (name == piName)
		{
			*result = &globals->pi;
			return DataSymbol;
		}
		
		if (name == envName)
		{
			*result = &globals->FE_DFL_ENV;
			return DataSymbol;
		}
		
		char functionName[22] = "MathLib_";
		char* end = stpncpy(functionName + 8, name, 13);
		if (*end != 0)
		{
			*result = nullptr;
			return SymbolNotFound;
		}
		
		if (void* symbol = dlsym(RTLD_SELF, functionName))
		{
			*result = symbol;
			return CodeSymbol;
		}
		
		*result = nullptr;
		return SymbolNotFound;
	}
	
	//80-bit register
	struct extended80 {
          uint8_t bytes[10];
         };
	
	void LibraryUnload(Globals* globals)
	{
		globals->allocator.Deallocate(globals);
	}
	
	void MathLib___fpclassify(Globals* globals, MachineState* state)
	{
		state->r3 = MapFPClassify(state->fpr[1]); 
	}
	
	void MathLib___fpclassifyd(Globals* globals, MachineState* state)
	{
		state->r3 = MapFPClassify(state->fpr[1]); 
	}
	
	void MathLib___fpclassifyf(Globals* globals, MachineState* state)
	{
		state->r3 = MapFPClassify(state->fpr[1]); 
	}
	
	void MathLib___inf(Globals* globals, MachineState* state)
	{
		state->r3 = std::isinf(state->fpr[1]) ? 1 : 0;
	}
	
	void MathLib___isfinite(Globals* globals, MachineState* state)
	{
		state->r3 = std::isfinite(state->fpr[1]) ? 1 : 0;
	}
	
	void MathLib___isfinited(Globals* globals, MachineState* state)
	{
		state->r3 = std::isfinite(state->fpr[1]) ? 1 : 0;
	}
	
	void MathLib___isfinitef(Globals* globals, MachineState* state)
	{
		state->r3 = std::isfinite(state->fpr[1]) ? 1 : 0;
	}
	
	void MathLib___isnan(Globals* globals, MachineState* state)
	{
		state->r3 = std::isnan(state->fpr[1]) ? 1 : 0;
	}
	
	void MathLib___isnand(Globals* globals, MachineState* state)
	{
		state->r3 = std::isnan(state->fpr[1]) ? 1 : 0;
	}
	
	void MathLib___isnanf(Globals* globals, MachineState* state)
	{
		state->r3 = std::isnan(state->fpr[1]) ? 1 : 0;
	}
	
	void MathLib___isnormal(Globals* globals, MachineState* state)
	{
	        state->r3 = std::isnormal(state->fpr[1]) ? 1 : 0;
	}
	
	void MathLib___isnormald(Globals* globals, MachineState* state)
	{
		state->r3 = std::isnormal(state->fpr[1]) ? 1 : 0;
	}
	
	void MathLib___isnormalf(Globals* globals, MachineState* state)
	{
		state->r3 = std::isnormal(state->fpr[1]) ? 1 : 0;
	}
	
	void MathLib___signbit(Globals* globals, MachineState* state)
	{
		state->r3 = std::signbit(state->fpr[1]) ? 1 : 0;
	}
	
	void MathLib___signbitd(Globals* globals, MachineState* state)
	{
		state->r3 = std::signbit(state->fpr[1]) ? 1 : 0;
	}
	
	void MathLib___signbitf(Globals* globals, MachineState* state)
	{
		state->r3 = std::signbit(state->fpr[1]) ? 1 : 0;
	}
	
	void MathLib_acos(Globals* globals, MachineState* state)
	{
		state->fpr[1] = acos(state->fpr[1]);
	}
	
	void MathLib_acosh(Globals* globals, MachineState* state)
	{
		state->fpr[1] = acosh(state->fpr[1]);
	}
	
	void MathLib_acoshl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = acosh(state->fpr[1]);
	}
	
	void MathLib_acosl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = acos(state->fpr[1]);
	}
	
	void MathLib_annuity(Globals* globals, MachineState* state)
	{
		double rate = state->fpr[1];
                double periods = state->fpr[2];

                if (rate == 0.0)
                {
                state->fpr[1] = periods;
                }
                else
                {
                 state->fpr[1] = (1.0 - std::pow(1.0 + rate, -periods)) / rate;
                }
	}
	
	void MathLib_asin(Globals* globals, MachineState* state)
	{
		state->fpr[1] = asin(state->fpr[1]);
	}
	
	void MathLib_asinh(Globals* globals, MachineState* state)
	{
		state->fpr[1] = asinh(state->fpr[1]);
	}
	
	void MathLib_asinhl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = asinh(state->fpr[1]);
	}
	
	void MathLib_asinl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = asin(state->fpr[1]);
	}
	
	void MathLib_atan(Globals* globals, MachineState* state)
	{
		state->fpr[1] = atan(state->fpr[1]);
	}
	
	void MathLib_atan2(Globals* globals, MachineState* state)
	{
		state->fpr[1] = atan2(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_atan2l(Globals* globals, MachineState* state)
	{
		state->fpr[1] = atan2(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_atanh(Globals* globals, MachineState* state)
	{
		state->fpr[1] = atanh(state->fpr[1]);
	}
	
	void MathLib_atanhl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = atanh(state->fpr[1]);
	}
	
	void MathLib_atanl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = atan(state->fpr[1]);
	}
	
	void MathLib_ceil(Globals* globals, MachineState* state)
	{
		state->fpr[1] = ceil(state->fpr[1]);
	}
	
	void MathLib_ceill(Globals* globals, MachineState* state)
	{
		state->fpr[1] = ceil(state->fpr[1]);
	}
	
	void MathLib_compound(Globals* globals, MachineState* state)
	{
	       double rate = state->fpr[1];
               double periods = state->fpr[2];
               state->fpr[1] = std::pow(1.0 + rate, periods);
	}
	
	void MathLib_copysign(Globals* globals, MachineState* state)
	{
		state->fpr[1] = copysign(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_copysignl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = copysign(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_cos(Globals* globals, MachineState* state)
	{
		state->fpr[1] = cos(state->fpr[1]);
	}
	
	void MathLib_cosh(Globals* globals, MachineState* state)
	{
		state->fpr[1] = cosh(state->fpr[1]);
	}
	
	void MathLib_coshl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = cosh(state->fpr[1]);
	}
	
	void MathLib_cosl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = cos(state->fpr[1]);
	}
	
	void MathLib_dec2f(Globals* globals, MachineState* state)
	{
	        // Obtém os ponteiros da memória emulada a partir dos registos PPC
    Decimal* srcDec = globals->allocator.ToPointer<Decimal>(state->r3);
    Common::Real32* dstFloat = globals->allocator.ToPointer<Common::Real32>(state->r4);

    if (!srcDec || !dstFloat)
    {
        return;
    }

    // 1. Reconstrói a cadeia de dígitos salvaguardando o limite de 31 caracteres
    int sigLength = srcDec->sig[0];
    if (sigLength > 31) sigLength = 31;

    std::string digits;
    for (int i = 1; i <= sigLength; ++i)
    {
        digits += static_cast<char>(srcDec->sig[i]);
    }

    if (digits.empty())
    {
        digits = "0";
    }

    // 2. Transforma a mantissa decimal num float do host
    float value = 0.0f;
    try {
        value = std::stof(digits);
    } catch (...) {
        value = 0.0f;
    }

    // 3. Aplica o expoente armazenado (srcDec->exp faz o byteswap implícito para o host)
    value *= std::pow(10.0f, static_cast<float>(static_cast<int16_t>(srcDec->exp)));

    // 4. Restaura o sinal correto (1 = Negativo)
    if (srcDec->sgn == 1)
    {
        value = -value;
    }

    // 5. O operator=(float) da classe Real32 trata automaticamente do byteswap para Big-Endian
    *dstFloat = value;
	}
	
	void MathLib_dec2l(Globals* globals, MachineState* state)
	{
		// Na arquitetura G3, 'long double' de destino é estruturalmente idêntico a um 'double' de 64 bits.
    // Usamos o Real64 para converter e gravar com segurança na memória da VM.
    Decimal* srcDec = globals->allocator.ToPointer<Decimal>(state->r3);
    Common::Real64* dstDouble = globals->allocator.ToPointer<Common::Real64>(state->r4);

    if (!srcDec || !dstDouble)
    {
        return;
    }

    int sigLength = srcDec->sig[0];
    if (sigLength > 31) sigLength = 31;

    std::string digits;
    for (int i = 1; i <= sigLength; ++i)
    {
        digits += static_cast<char>(srcDec->sig[i]);
    }

    if (digits.empty())
    {
        digits = "0";
    }

    double value = 0.0;
    try {
        value = std::stod(digits);
    } catch (...) {
        value = 0.0;
    }

    value *= std::pow(10.0, static_cast<int16_t>(srcDec->exp));

    if (srcDec->sgn == 1)
    {
        value = -value;
    }

    // O operator=(double) da classe Real64 trata automaticamente do byteswap para Big-Endian
    *dstDouble = value;
	}
	
	void MathLib_dec2num(Globals* globals, MachineState* state)
	{
		// r3 aponta para a struct Decimal de origem na memória da VM
    Decimal* srcDec = globals->allocator.ToPointer<Decimal>(state->r3);

    if (!srcDec)
    {
        state->fpr[1] = 0.0;
        return;
    }

    // Reconstrói a cadeia de dígitos salvaguardando o limite de 31 caracteres
    int sigLength = srcDec->sig[0];
    if (sigLength > 31) sigLength = 31;

    std::string digits;
    for (int i = 1; i <= sigLength; ++i)
    {
        digits += static_cast<char>(srcDec->sig[i]);
    }

    if (digits.empty())
    {
        digits = "0";
    }

    // Transforma a mantissa decimal extraída num número double real do host
    double value = 0.0;
    try {
        value = std::stod(digits);
    } catch (...) {
        value = 0.0;
    }

    // Aplica o expoente armazenado (srcDec->exp faz o byteswap implícito para o host)
    value *= std::pow(10.0, static_cast<int16_t>(srcDec->exp));

    // Restaura o sinal correto do número real (1 = Negativo)
    if (srcDec->sgn == 1)
    {
        value = -value;
    }

    // Devolve o número real descodificado no registo de retorno flutuante r1
    state->fpr[1] = value;
	}
	
	void MathLib_dec2numl(Globals* globals, MachineState* state)
	{
		MathLib_dec2num(globals, state);
	}
	
	void MathLib_dec2s(Globals* globals, MachineState* state)
	{
		// No ecossistema PowerPC G3, a conversão para 'single' (dec2s) 
    // mapeia exatamente o mesmo comportamento de 'dec2f' (conversão para float de 32 bits).
    Decimal* srcDec = globals->allocator.ToPointer<Decimal>(state->r3);
    Common::Real32* dstFloat = globals->allocator.ToPointer<Common::Real32>(state->r4);

    if (!srcDec || !dstFloat)
    {
        return;
    }

    // 1. Extrai a cadeia de dígitos limitando ao teto histórico de 31 caracteres
    int sigLength = srcDec->sig[0];
    if (sigLength > 31) sigLength = 31;

    std::string digits;
    for (int i = 1; i <= sigLength; ++i)
    {
        digits += static_cast<char>(srcDec->sig[i]);
    }

    if (digits.empty())
    {
        digits = "0";
    }

    // 2. Transforma a mantissa decimal extraída num float nativo do host
    float value = 0.0f;
    try {
        value = std::stof(digits);
    } catch (...) {
        value = 0.0f;
    }

    // 3. Aplica o expoente de base 10 (srcDec->exp faz o byteswap implícito para o host)
    value *= std::pow(10.0f, static_cast<float>(static_cast<int16_t>(srcDec->exp)));

    // 4. Restaura o sinal correto (1 = Negativo)
    if (srcDec->sgn == 1)
    {
        value = -value;
    }

    // 5. O operator=(float) da classe Real32 trata automaticamente do byteswap para Big-Endian
    *dstFloat = value;
	}
	
	void MathLib_dec2str(Globals* globals, MachineState* state)
	{
	  //Get the flags from the registers
          DecForm* form = globals->allocator.ToPointer<DecForm>(state->r3);
          Decimal* dec  = globals->allocator.ToPointer<Decimal>(state->r4);
          char* dstStr  = globals->allocator.ToPointer<char>(state->r5);

         if (!form || !dec || !dstStr)
         {
           return;
         }

       // 1. Rebuild the number from the Pascal string
       // sig[0] storr the numver of digits
        int sigLength = dec->sig[0];
        if (sigLength > 31) sigLength = 31; // Maximum lenght

        std::string digits;
        for (int i = 1; i <= sigLength; ++i)
        {
         digits += static_cast<char>(dec->sig[i]);
        }

       if (digits.empty())
       {
        digits = "0";
       }

       // Convert the digits to a native double
       double value = 0.0;
       try {
        value = std::stod(digits);
       } catch (...) {
        value = 0.0;
       }

      // Apply base 10 exponent (dec->exp make byteswap)
       value *= std::pow(10.0, static_cast<int16_t>(dec->exp));

       // Apply the sign (dec->sgn == 1 means negative)
       if (dec->sgn == 1)
       {
        value = -value;
        }

      // 2. Format the number by sent DecForm
      char buffer[256];
      int16_t style = form->style;   // 0 = Float/Scientific, 1 = Fixed
      int16_t digitsCount = form->digits;

       if (style == 0)
       {
         // Scientific Format (like %e from printf)
        std::snprintf(buffer, sizeof(buffer), "%.*e", digitsCount, value);
       }
       else
       {
        // Decimal Format (like %f from printf)
        std::snprintf(buffer, sizeof(buffer), "%.*f", digitsCount, value);
      }

    // 3. Write the final string
    std::strcpy(dstStr, buffer);
      }
	
	void MathLib_dtox80(Globals* globals, MachineState* state)
	{
	  
           Common::Real64* src = globals->allocator.ToPointer<Common::Real64>(state->r3);
          extended80* dst = globals->allocator.ToPointer<extended80>(state->r4);

         if (src && dst)
         {
          double nativeDouble = *src;
        long double localHostLD = static_cast<long double>(nativeDouble);
        uint8_t* hostBytes = reinterpret_cast<uint8_t*>(&localHostLD);

         for (int i = 0; i < 10; ++i)
          {
            dst->bytes[i] = hostBytes[9 - i];
          }
         }
	}
	
	void MathLib_erf(Globals* globals, MachineState* state)
	{
		state->fpr[1] = erf(state->fpr[1]);
	}
	
	void MathLib_erfc(Globals* globals, MachineState* state)
	{
		state->fpr[1] = erfc(state->fpr[1]);
	}
	
	void MathLib_erfcl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = erfc(state->fpr[1]);
	}
	
	void MathLib_erfl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = erf(state->fpr[1]);
	}
	
	void MathLib_exp(Globals* globals, MachineState* state)
	{
		state->fpr[1] = exp(state->fpr[1]);
	}
	
	void MathLib_exp2(Globals* globals, MachineState* state)
	{
		state->fpr[1] = exp2(state->fpr[1]);
	}
	
	void MathLib_exp2l(Globals* globals, MachineState* state)
	{
		state->fpr[1] = exp2(state->fpr[1]);
	}
	
	void MathLib_expl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = exp(state->fpr[1]);
	}
	
	void MathLib_expm1(Globals* globals, MachineState* state)
	{
		state->fpr[1] = expm1(state->fpr[1]);
	}
	
	void MathLib_expm1l(Globals* globals, MachineState* state)
	{
		state->fpr[1] = expm1(state->fpr[1]);
	}
	
	void MathLib_fabs(Globals* globals, MachineState* state)
	{
		state->fpr[1] = fabs(state->fpr[1]);
	}
	
	void MathLib_fabsl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = fabs(state->fpr[1]);
	}
	
	void MathLib_fdim(Globals* globals, MachineState* state)
	{
		state->fpr[1] = fdim(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_fdiml(Globals* globals, MachineState* state)
	{
		state->fpr[1] = fdim(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_feclearexcept(Globals* globals, MachineState* state)
        {
          uint32_t excepts = state->r3;
    
    // Limpeza direta dos bits de exceção do PowerPC no campo hex global
    if (excepts & (1 << 0)) state->fpscr.hex &= ~(1 << 27); // Mapeamento correto do bit VX
    if (excepts & (1 << 1)) state->fpscr.hex &= ~(1 << 26); // OX
    if (excepts & (1 << 2)) state->fpscr.hex &= ~(1 << 25); // UX
    if (excepts & (1 << 3)) state->fpscr.hex &= ~(1 << 24); // ZX
    if (excepts & (1 << 4)) state->fpscr.hex &= ~(1 << 23); // XX
    
         state->r3 = 0;
         }
	
	void MathLib_fegetenv(Globals* globals, MachineState* state)
	{
              Common::UInt32* envPtr = globals->allocator.ToPointer<Common::UInt32>(state->r3);
               if (envPtr)
                {
                 *envPtr = state->fpscr.hex;
                 state->r3 = 0; //Done
                }
               else
                {
                 state->r3 = 1; // Fail
                }
	}
	
	void MathLib_fegetexcept(Globals* globals, MachineState* state)
	{
		uint32_t enabledExcepts = 0;
    
                 // On PowerPC, XE, ZE, UE, OE, VE control the interrupt activations:
                if (state->fpscr.VE) enabledExcepts |= (1 << 0); // VE = Invalid Operation Enable (Map toVX)
                if (state->fpscr.OE) enabledExcepts |= (1 << 1); // OE = Overflow Enable (Map to OX)
                if (state->fpscr.UE) enabledExcepts |= (1 << 2); // UE = Underflow Enable (Map to UX)
                if (state->fpscr.ZE) enabledExcepts |= (1 << 3); // ZE = Zero Divide Enable (Map to ZX)
                if (state->fpscr.XE) enabledExcepts |= (1 << 4); // XE = Inexact Enable (Map to XX)
    
               // Return the interrup table to r3 register
                state->r3 = enabledExcepts;
	}
	
	void MathLib_fegetround(Globals* globals, MachineState* state)
	{
		state->r3 = state->fpscr.RN;
	}
	
	void MathLib_feholdexcept(Globals* globals, MachineState* state)
	{
		// Store the current state on r3 pointer
                MathLib_fegetenv(globals, state);
    
                // Then clear all exceptions and return
                state->fpscr.VX = 0;
                state->fpscr.OX = 0;
                state->fpscr.UX = 0;
                state->fpscr.ZX = 0;
                state->fpscr.XX = 0;
    
                state->r3 = 0;
	}
	
	void MathLib_feraiseexcept(Globals* globals, MachineState* state)
	{
         uint32_t excepts = state->r3;
    
         if (excepts & (1 << 0)) state->fpscr.VX = 1;
         if (excepts & (1 << 1)) state->fpscr.OX = 1;
         if (excepts & (1 << 2)) state->fpscr.UX = 1;
         if (excepts & (1 << 3)) state->fpscr.ZX = 1;
         if (excepts & (1 << 4)) state->fpscr.XX = 1;
    
         state->r3 = 0;
	}
	
	void MathLib_fesetenv(Globals* globals, MachineState* state)
	{
		Common::UInt32* envPtr = globals->allocator.ToPointer<Common::UInt32>(state->r3);
                if (envPtr)
                {
                 state->fpscr.hex = *envPtr;
                 state->r3 = 0;
                }
                else
                {
                 state->r3 = 1;
                }
	}
	
	void MathLib_fesetexcept(Globals* globals, MachineState* state)
	{
		// Exception table from r3 register
                uint32_t exceptsToEnable = state->r3;

                // Enable (1) ou Disable (0) the trap bits from FPU accordling to the exception table
                state->fpscr.VE = (exceptsToEnable & (1 << 0)) ? 1 : 0; // VE control Invalid Operation
                state->fpscr.OE = (exceptsToEnable & (1 << 1)) ? 1 : 0; // OE control Overflow
                state->fpscr.UE = (exceptsToEnable & (1 << 2)) ? 1 : 0; // UE control Underflow
                state->fpscr.ZE = (exceptsToEnable & (1 << 3)) ? 1 : 0; // ZE control Zero Divide
                state->fpscr.XE = (exceptsToEnable & (1 << 4)) ? 1 : 0; // XE control Inexact

                // Return Success
                 state->r3 = 0;
	}
	
	void MathLib_fesetround(Globals* globals, MachineState* state)
	{
		// Define mode
                uint32_t roundingMode = state->r3;
    
                // O PowerPC only support mode 0 and 3, otherwise it fail
                if (roundingMode <= 3)
                 {
                   state->fpscr.RN = roundingMode;
                   state->r3 = 0; // Mac OS will return 0 in case of success)
                 }
                else
                 {
                   state->r3 = 1; // Fail
                 }
	}
	
	void MathLib_fetestexcept(Globals* globals, MachineState* state)
	{
		uint32_t mask = state->r3;
                uint32_t currentExcepts = 0;
    
                if (state->fpscr.VX) currentExcepts |= (1 << 0);
                if (state->fpscr.OX) currentExcepts |= (1 << 1);
                if (state->fpscr.UX) currentExcepts |= (1 << 2);
                if (state->fpscr.ZX) currentExcepts |= (1 << 3);
                if (state->fpscr.XX) currentExcepts |= (1 << 4);
                state->r3 = currentExcepts & mask;
	}
	
	void MathLib_feupdateenv(Globals* globals, MachineState* state)
	{
		// This routine merges the status stored in r3 with the actual exceptions 
                  uint32_t currentVX = state->fpscr.VX;
                  uint32_t currentOX = state->fpscr.OX;
                  uint32_t currentUX = state->fpscr.UX;
                  uint32_t currentZX = state->fpscr.ZX;
                  uint32_t currentXX = state->fpscr.XX;
    
                 // Restores the old status
                  MathLib_fesetenv(globals, state);
    
                 // Restores the old exceptions
                  if (currentVX) state->fpscr.VX = 1;
                  if (currentOX) state->fpscr.OX = 1;
                  if (currentUX) state->fpscr.UX = 1;
                  if (currentZX) state->fpscr.ZX = 1;
                  if (currentXX) state->fpscr.XX = 1;
    
                  state->r3 = 0;
	}
	
	void MathLib_floor(Globals* globals, MachineState* state)
	{
		state->fpr[1] = floor(state->fpr[1]);
	}
	
	void MathLib_floorl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = floor(state->fpr[1]);
	}
	
	void MathLib_fmax(Globals* globals, MachineState* state)
	{
		state->fpr[1] = fmax(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_fmaxl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = fmax(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_fmin(Globals* globals, MachineState* state)
	{
		state->fpr[1] = fmin(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_fminl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = fmin(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_fmod(Globals* globals, MachineState* state)
	{
		state->fpr[1] = fmod(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_frexp(Globals* globals, MachineState* state)
	{
		int intpart;
		state->fpr[1] = frexp(state->fpr[1], &intpart);
		Common::SInt32* out = globals->allocator.ToPointer<Common::SInt32>(state->r4);
		if (out)
                {
                 *out = intpart; 
                }
	}
	
	void MathLib_frexpl(Globals* globals, MachineState* state)
	{
		MathLib_frexp(globals,state);
	}
	
	void MathLib_gamma(Globals* globals, MachineState* state)
	{
		state->fpr[1] = tgamma(state->fpr[1]);
	}
	
	void MathLib_gammal(Globals* globals, MachineState* state)
	{
		state->fpr[1] = tgamma(state->fpr[1]);
	}
	
	void MathLib_hypot(Globals* globals, MachineState* state)
	{
		state->fpr[1] = hypot(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_hypotl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = hypot(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_ldexp(Globals* globals, MachineState* state)
	{
		state->fpr[1] = ldexp(state->fpr[1], state->r4);
	}
	
	void MathLib_ldexpl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = ldexp(state->fpr[1], state->r4);
	}
	
	void MathLib_ldtox80(Globals* globals, MachineState* state)
	{
		MathLib_dtox80(globals, state);
	}
	
	void MathLib_lgamma(Globals* globals, MachineState* state)
	{
		state->fpr[1] = lgamma(state->fpr[1]);
	}
	
	void MathLib_lgammal(Globals* globals, MachineState* state)
	{
		state->fpr[1] = lgamma(state->fpr[1]);
	}
	
	void MathLib_log(Globals* globals, MachineState* state)
	{
		state->fpr[1] = log(state->fpr[1]);
	}
	
	void MathLib_log10(Globals* globals, MachineState* state)
	{
		state->fpr[1] = log10(state->fpr[1]);
	}
	
	void MathLib_log10l(Globals* globals, MachineState* state)
	{
		state->fpr[1] = log10(state->fpr[1]);
	}
	
	void MathLib_log1p(Globals* globals, MachineState* state)
	{
		state->fpr[1] = log1p(state->fpr[1]);
	}
	
	void MathLib_log1pl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = log1p(state->fpr[1]);
	}
	
	void MathLib_log2(Globals* globals, MachineState* state)
	{
		state->fpr[1] = log2(state->fpr[1]);
	}
	
	void MathLib_log2l(Globals* globals, MachineState* state)
	{
		state->fpr[1] = log2(state->fpr[1]);
	}
	
	void MathLib_logb(Globals* globals, MachineState* state)
	{
		state->fpr[1] = logb(state->fpr[1]);
	}
	
	void MathLib_logbl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = logb(state->fpr[1]);
	}
	
	void MathLib_logl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = log(state->fpr[1]);
	}
	
	void MathLib_modf(Globals* globals, MachineState* state)
	{
	       double intpart;
               state->fpr[1] = std::modf(state->fpr[1], &intpart);
               Common::Real64* out = globals->allocator.ToPointer<Common::Real64>(state->r4);
               if (out)
               {
                *out = intpart;
               }
	}
	
	void MathLib_modff(Globals* globals, MachineState* state)
	{
	       double intpart;
               state->fpr[1] = std::modf(state->fpr[1], &intpart);
               Common::Real32* out = globals->allocator.ToPointer<Common::Real32>(state->r4);
               if (out)
               {
                *out = intpart;
               }
	}
	
	void MathLib_modfl(Globals* globals, MachineState* state)
	{
		MathLib_modf(globals, state);
	}
	
	void MathLib_nan(Globals* globals, MachineState* state)
	{
		state->fpr[1] = NAN;
	}
	
	void MathLib_nanf(Globals* globals, MachineState* state)
	{
		state->fpr[1] = NAN;
	}
	
	void MathLib_nanl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = NAN;
	}
	
	void MathLib_nearbyint(Globals* globals, MachineState* state)
	{
		state->fpr[1] = nearbyint(state->fpr[1]);
	}
	
	void MathLib_nearbyintl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = nearbyint(state->fpr[1]);
	}
	
	void MathLib_nextafterd(Globals* globals, MachineState* state)
	{
		state->fpr[1] = std::nextafter(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_nextafterf(Globals* globals, MachineState* state)
	{
		state->fpr[1] = std::nextafter(static_cast<float>(state->fpr[1]), static_cast<float>(state->fpr[2]));
	}
	
	void MathLib_nextafterl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = std::nextafter(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_num2dec(Globals* globals, MachineState* state)
	{
		// Obtém os ponteiros da memória emulada a partir dos registos PPC
    DecForm* form = globals->allocator.ToPointer<DecForm>(state->r3);
    Decimal* dstDec = globals->allocator.ToPointer<Decimal>(state->r4);
    // O número real de origem está contido no primeiro registo flutuante
    double value = state->fpr[1];

    if (!form || !dstDec)
    {
        return;
    }

    // Inicializa a estrutura de destino
    dstDec->sgn = (std::signbit(value)) ? 1 : 0;
    dstDec->unused = 0;
    dstDec->exp = 0;
    dstDec->sig[0] = 0; // String Pascal vazia inicialmente

    if (std::isnan(value) || std::isinf(value))
    {
        // Tratamento básico para NaNs e Infinitos: define uma mantissa padrão "0"
        dstDec->sig[0] = 1;
        dstDec->sig[1] = '0';
        return;
    }

    // Isola o valor absoluto para extração dos dígitos decimais
    double absValue = std::abs(value);
    if (absValue == 0.0)
    {
        dstDec->sig[0] = 1;
        dstDec->sig[1] = '0';
        dstDec->exp = 0;
        return;
    }

    // Calcula o expoente base 10 aproximado para normalizar o número
    int32_t exponent = static_cast<int32_t>(std::floor(std::log10(absValue)));
    
    // Escala o valor para que a parte inteira contenha os dígitos significativos desejados
    // O limite máximo da struct Decimal da Apple é de 31 dígitos significativos
    int16_t requestedDigits = form->digits;
    if (requestedDigits <= 0 || requestedDigits > 31) requestedDigits = 15; // Fallback seguro para precisão double

    // Ajusta o expoente dinamicamente com base no estilo de formatação (0 = Científico, 1 = Fixo)
    if (form->style == 1)
    {
        // No estilo Fixo, o expoente final é determinado estritamente pelas casas decimais solicitadas
        exponent = -requestedDigits;
    }
    else
    {
        // No estilo Científico, ajustamos o expoente para manter o número na forma d.dddd...
        exponent = exponent - (requestedDigits - 1);
    }

    // Multiplica ou divide para mover o ponto decimal e isolar os dígitos como um inteiro grande
    double scaledValue = value * std::pow(10.0, -exponent);
    double integralPart;
    std::modf(std::round(scaledValue), &integralPart); // Arredonda para evitar perdas de precisão na conversão

    // Converte a parte inteira obtida numa string de texto C++ para popular a mantissa
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.0f", std::abs(integralPart));
    std::string digitsStr(buffer);

    // Trunca a string caso exceda a capacidade máxima histórica de 31 bytes
    size_t finalLen = digitsStr.size();
    if (finalLen > 31)
    {
        int32_t diff = static_cast<int32_t>(finalLen - 31);
        exponent += diff; // Ajusta o expoente proporcionalmente à truncagem
        finalLen = 31;
    }

    // Escreve os resultados finais na memória da VM PPC (com byteswap automático no exp)
    dstDec->exp = static_cast<int16_t>(exponent);
    dstDec->sig[0] = static_cast<uint8_t>(finalLen); // Primeiro byte armazena o tamanho (Pascal String)
    for (size_t i = 0; i < finalLen; ++i)
    {
        dstDec->sig[i + 1] = static_cast<uint8_t>(digitsStr[i]);
    }
	}
	
	void MathLib_num2decl(Globals* globals, MachineState* state)
	{
		MathLib_num2dec(globals, state);
	}
	
	void MathLib_pow(Globals* globals, MachineState* state)
	{
		state->fpr[1] = pow(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_powl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = pow(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_randomx(Globals* globals, MachineState* state)
	{
		// state->r3 contém o ponteiro para o double da semente na memória da VM
    Common::Real64* seedPtr = globals->allocator.ToPointer<Common::Real64>(state->r3);

    if (!seedPtr)
    {
        state->fpr[1] = 0.0;
        return;
    }

    // Lê a semente numérica (o operator double() faz o byteswap implícito)
    double currentSeed = *seedPtr;

    // Constantes matemáticas clássicas da especificação SANE da Apple
    double multiplier = 16807.0;
    double modulus = 2147483647.0; // 2^31 - 1

    // Calcula o próximo passo do gerador linear congruente
    double nextSeed = std::fmod(currentSeed * multiplier, modulus);
    if (nextSeed < 0.0)
    {
        nextSeed += modulus;
    }

    // Atualiza a semente na memória da VM (o operator= faz o byteswap reverso para Big-Endian)
    *seedPtr = nextSeed;

    // Devolve o número pseudo-aleatório gerado no registo flutuante r1
    state->fpr[1] = nextSeed;
	}
	
	void MathLib_relation(Globals* globals, MachineState* state)
	{
		// Argumentos sequenciais populam os registos consecutivos da FPU do PPC
    double x = state->fpr[1];
    double y = state->fpr[2];

    // Mapeamento estrito dos valores enumerados exigidos pelo PowerPC Numerics da Apple
    if (std::isnan(x) || std::isnan(y))
    {
        state->r3 = 4; // UNORDERED
    }
    else if (x > y)
    {
        state->r3 = 1; // GREATERTHAN
    }
    else if (x < y)
    {
        state->r3 = 2; // LESSTHAN
    }
    else
    {
        state->r3 = 3; // EQUALTO
    }
	}
	
	void MathLib_relationl(Globals* globals, MachineState* state)
	{
		MathLib_relation(globals, state);
	}
	
	void MathLib_remainder(Globals* globals, MachineState* state)
	{
		state->fpr[1] = remainder(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_remainderl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = remainder(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_remquo(Globals* globals, MachineState* state)
	{
		int remquoOut;
		state->fpr[1] = remquo(state->fpr[1], state->fpr[2], &remquoOut);
		Common::UInt32* out = globals->allocator.ToPointer<Common::UInt32>(state->r5);
                if (out)
                 {
                  *out = static_cast<uint32_t>(remquoOut);
                 }
	}
	
	void MathLib_remquol(Globals* globals, MachineState* state)
	{
		MathLib_remquo(globals,state);
	}
	
	void MathLib_rint(Globals* globals, MachineState* state)
	{
		state->fpr[1] = rint(state->fpr[1]);
	}
	
	void MathLib_rintl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = rint(state->fpr[1]);
	}
	
	void MathLib_rinttol(Globals* globals, MachineState* state)
	{
		// std::rint respeita o modo de arredondamento ativo (definido via fenv ou fpscr)
    double rounded = std::rint(state->fpr[1]);
    
    // Converte para inteiro de 32 bits com sinal e guarda no registo r3
    state->r3 = static_cast<int32_t>(rounded);
	}
	
	void MathLib_rinttoll(Globals* globals, MachineState* state)
	{
		double rounded = std::rint(state->fpr[1]);
    int64_t result64 = static_cast<int64_t>(rounded);
    
    // Convenção de chamada do PowerPC de 32-bits para retornos de 64-bits:
    // r3 recebe a metade superior (bits 32-63) e r4 recebe a metade inferior (bits 0-31)
    state->r3 = static_cast<uint32_t>(result64 >> 32);
    state->r4 = static_cast<uint32_t>(result64 & 0xFFFFFFFF);
	}
	
	void MathLib_round(Globals* globals, MachineState* state)
	{
		state->fpr[1] = round(state->fpr[1]);
	}
	
	void MathLib_roundl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = round(state->fpr[1]);
	}
	
	void MathLib_roundtol(Globals* globals, MachineState* state)
	{
		// std::round arredonda para longe de zero em casos de .5, ignorando o fenv corrente
                double rounded = std::round(state->fpr[1]);
    
    // Devolve o inteiro de 32-bits com sinal no registo r3
    state->r3 = static_cast<int32_t>(rounded);
	}
	
	void MathLib_roundtoll(Globals* globals, MachineState* state)
	{
		double rounded = std::round(state->fpr[1]);
    int64_t result64 = static_cast<int64_t>(rounded);
    
    // Convenção estrita de chamadas PowerPC de 32-bits para retornos de 64-bits:
    // r3 recebe a metade superior (bits mais significativos)
    state->r3 = static_cast<uint32_t>(result64 >> 32);
    // r4 recebe a metade inferior (bits menos significativos)
    state->r4 = static_cast<uint32_t>(result64 & 0xFFFFFFFF);
	}
	
	void MathLib_scalb(Globals* globals, MachineState* state)
	{
		state->fpr[1] = scalb(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_scalbl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = scalb(state->fpr[1], state->fpr[2]);
	}
	
	void MathLib_sin(Globals* globals, MachineState* state)
	{
		state->fpr[1] = sin(state->fpr[1]);
	}
	
	void MathLib_sinh(Globals* globals, MachineState* state)
	{
		state->fpr[1] = sinh(state->fpr[1]);
	}
	
	void MathLib_sinhl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = sinh(state->fpr[1]);
	}
	
	void MathLib_sinl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = sin(state->fpr[1]);
	}
	
	void MathLib_sqrt(Globals* globals, MachineState* state)
	{
		state->fpr[1] = sqrt(state->fpr[1]);
	}
	
	void MathLib_sqrtl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = sqrt(state->fpr[1]);
	}
	
	void MathLib_str2dec(Globals* globals, MachineState* state)
	{
		//Obtain the parameters
        const char* srcStr    = globals->allocator.ToPointer<const char>(state->r3);
        Common::SInt32* vIdx  = globals->allocator.ToPointer<Common::SInt32>(state->r4);
        Decimal* dstDec       = globals->allocator.ToPointer<Decimal>(state->r5);

        if (!srcStr || !dstDec)
        {
         return;
        }

        // Initialization
        dstDec->sgn = 0;
        dstDec->unused = 0;
        dstDec->exp = 0;
        dstDec->sig[0] = 0; 

        std::string str(srcStr);
        if (str.empty())
        {
         if (vIdx) *vIdx = 0;
         return;
        }

       // 1. Manual parsing of sign and digits
       size_t idx = 0;
       // Remove white spaces
        while (idx < str.size() && std::isspace(static_cast<unsigned char>(str[idx])))
       {
        idx++;
        }

        if (idx >= str.size())
       {
        if (vIdx) *vIdx = static_cast<int32_t>(idx);
        return;
       }

    // Handle sign
        if (str[idx] == '-')
       {
        dstDec->sgn = 1; // 1 = Negative
        idx++;
       }
       else if (str[idx] == '+')
      {
        idx++;
       }

    // Handle mantissa and decimal point
    std::string rawDigits;
    int32_t decimalPointOffset = -1;
    bool foundDigits = false;

    while (idx < str.size())
    {
        char c = str[idx];
        if (std::isdigit(static_cast<unsigned char>(c)))
        {
            // Remove left zeroes
            if (!rawDigits.empty() || c != '0')
            {
                rawDigits += c;
            }
            foundDigits = true;
            idx++;
        }
        else if (c == '.' && decimalPointOffset == -1)
        {
            // Register the mantissa
            decimalPointOffset = static_cast<int32_t>(rawDigits.size());
            idx++;
        }
        else
        {
            // Invalid string
            break;
        }
    }

    // If no valid string, it aborts
    if (!foundDigits)
    {
        if (vIdx) *vIdx = static_cast<int32_t>(idx);
        return;
    }

    // A set of zeroes, return 0
    if (rawDigits.empty())
    {
        rawDigits = "0";
    }

    // 2. Base-10 exponential (exp)
    int32_t exponent = 0;
    if (decimalPointOffset != -1)
    {
        // A decimal point will reduce the exponent
        exponent = decimalPointOffset - static_cast<int32_t>(rawDigits.size());
    }

    // Set maximum mantissa size
    size_t finalLen = rawDigits.size();
    if (finalLen > 31)
    {
        // Remove digit excess
        int32_t truncatedDiff = static_cast<int32_t>(finalLen - 31);
        exponent += truncatedDiff;
        finalLen = 31;
    }

    // 3. Store the data on emulated DecStr
    dstDec->exp = static_cast<int16_t>(exponent);

    // Buid the Pascal String
    dstDec->sig[0] = static_cast<uint8_t>(finalLen); // Primeiro byte store lenght
    for (size_t i = 0; i < finalLen; ++i)
    {
        dstDec->sig[i + 1] = static_cast<uint8_t>(rawDigits[i]);
    }

    // 4. Update the validation flag
    if (vIdx)
    {
        // BigEndian autofix
        *vIdx = static_cast<int32_t>(idx);
    }
	}
	
	void MathLib_tan(Globals* globals, MachineState* state)
	{
		state->fpr[1] = tan(state->fpr[1]);
	}
	
	void MathLib_tanh(Globals* globals, MachineState* state)
	{
		state->fpr[1] = tanh(state->fpr[1]);
	}
	
	void MathLib_tanhl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = tanh(state->fpr[1]);
	}
	
	void MathLib_tanl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = tan(state->fpr[1]);
	}
	
	void MathLib_trunc(Globals* globals, MachineState* state)
	{
		state->fpr[1] = trunc(state->fpr[1]);
	}
	
	void MathLib_truncl(Globals* globals, MachineState* state)
	{
		state->fpr[1] = trunc(state->fpr[1]);
	}
	
	void MathLib_x80tod(Globals* globals, MachineState* state)
        {
         extended80* src = globals->allocator.ToPointer<extended80>(state->r3);
         Common::Real64* dst = globals->allocator.ToPointer<Common::Real64>(state->r4);

         if (src && dst)
        {
        // Extração explícita de bits baseada no layout SANE (1 bit sinal, 15 bits expoente, 64 bits mantissa)
        uint8_t sign = (src->bytes[0] & 0x80) >> 7;
        uint16_t exp = ((static_cast<uint16_t>(src->bytes[0] & 0x7F)) << 8) | src->bytes[1];
        
        uint64_t mantissa = 0;
        for (int i = 0; i < 8; ++i)
        {
            mantissa |= (static_cast<uint64_t>(src->bytes[2 + i]) << (56 - (i * 8)));
        }

        // Casos Especiais SANE
        if (exp == 0 && mantissa == 0)
        {
            *dst = sign ? -0.0 : 0.0;
            return;
        }
        if (exp == 0x7FFF)
        {
            // Infinito ou NaN (SANE define o bit mais significativo da mantissa para Quiet/Signaling)
            *dst = (mantissa & 0x4000000000000000ull) ? std::numeric_limits<double>::quiet_NaN() : std::numeric_limits<double>::infinity();
            if (sign) *dst = -(*dst);
            return;
        }

        // Reconstrução matemática do Double (IEEE 754 de 64-bits) independente da CPU do host
        double result = static_cast<double>(mantissa) * std::pow(2.0, static_cast<int>(exp) - 16383 - 63);
        if (sign) result = -result;

        *dst = result;
         }
        }
	
	void MathLib_x80told(Globals* globals, MachineState* state)
	{
		MathLib_x80tod(globals, state); //Callback
	}
}
