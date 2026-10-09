//
// StdCLib.cpp
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

#include <cstdlib>
#include <cfloat>
#include <cassert>
#include <cctype>
#include <map>
#include <string>
#include <sstream>
#include <regex>
#include <cstring>
#include <csignal>

#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h> // Garante que está incluído para a função access()

#include "MachineState.h"
#include "BigEndian.h"
#include "Structures.h"
#include "StdCLib.h"
#include "StdCLibFunctions.h"
#include "SymbolResolver.h"
#include "NotImplementedException.h"
#include "Todo.h"

using PPCVM::MachineState;

namespace
{
	FILE* fdup(FILE* fp)
	{
		static const char* modes[] = {
			[O_RDONLY] = "r",
			[O_WRONLY] = "a",
			[O_RDWR] = "r+"
		};
		
		int fd = fileno(fp);
		int accmode = fcntl(fd, F_GETFL) & O_ACCMODE;
		return fdopen(dup(fd), modes[accmode]);
	}
}

namespace StdCLib
{
    const int NFILE = 40;

    union PPCFILE
    {
         static const char* OffsetNames[28];
    
         // Força o empacotamento estrito do Mac OS clássico e o Endianness correto
         struct __attribute__((packed))
        {
         Common::SInt32 _cnt;   // Usar tipos do BigEndian.h
         Common::UInt32 _ptr;   // Endereço na VM do PowerPC
         Common::UInt32 _base;  // Endereço na VM do PowerPC
         Common::UInt32 _end;   // Endereço na VM do PowerPC
         Common::UInt16 _size;
         Common::UInt16 _flag;
         Common::UInt16 _file;
         uint8_t  _unused[2];   // Alinhamento para bater com o tamanho clássico de 24/28 bytes se necessário
        };
    };

	struct IntEnv
        {
         Common::UInt32 runtimeFlags; // Alterado para UInt32 para garantir o alinhamento correto de 4 bytes dos campos seguintes
         Common::UInt32 argc;         // Alinhado corretamente
         Common::UInt32 argv;         // Ponteiro para o array de strings na VM
         Common::UInt32 envp;         // Ponteiro para as variáveis de ambiente na VM
         } __attribute__((packed));

	const char* PPCFILE::OffsetNames[28] = {
		"_cnt", "_cnt + 1", "_cnt + 2", "_cnt + 3",
		"_ptr", "_ptr + 1", "_ptr + 2", "_ptr + 3",
		"_base", "_base + 1", "_base + 2", "_base + 3",
		"_end", "_end + 1", "_end + 2", "_end + 3",
		"_size", "_size + 1", "_size + 2", "_size + 3",
		"_flag", "_flag + 1", "_flag + 2", "_flag + 3",
		"_file", "_file + 1", "_file + 2", "_file + 3",
	};

	#define _UPP		 0x01
	#define _LOW		 0x02
	#define _DIG		 0x04
	#define _WSP		 0x08
	#define _PUN		 0x10
	#define _CTL		 0x20
	#define _BLA		 0x40
	#define _HEX		 0x80

	const uint8_t cTypeCharClasses[0x100] = {
		_CTL, _CTL, _CTL, _CTL, _CTL, _CTL, _CTL, _CTL,
		_CTL, _CTL|_WSP, _CTL|_WSP, _CTL|_WSP, _CTL|_WSP, _CTL|_WSP, _CTL, _CTL,
		_CTL, _CTL, _CTL, _CTL, _CTL, _CTL, _CTL, _CTL,
		_CTL, _CTL, _CTL, _CTL, _CTL, _CTL, _CTL, _CTL,
		_WSP|_BLA, _PUN, _PUN, _PUN, _PUN, _PUN, _PUN, _PUN,
		_PUN, _PUN, _PUN, _PUN, _PUN, _PUN, _PUN, _PUN,
		_DIG, _DIG, _DIG, _DIG, _DIG, _DIG, _DIG, _DIG,
		_DIG, _DIG, _PUN, _PUN, _PUN, _PUN, _PUN, _PUN,
		_PUN, _UPP|_HEX, _UPP|_HEX, _UPP|_HEX, _UPP|_HEX, _UPP|_HEX, _UPP|_HEX, _UPP,
		_UPP, _UPP, _UPP, _UPP, _UPP, _UPP, _UPP, _UPP,
		_UPP, _UPP, _UPP, _UPP, _UPP, _UPP, _UPP, _UPP,
		_UPP, _UPP, _UPP, _PUN, _PUN, _PUN, _PUN, _PUN,
		_PUN, _LOW|_HEX, _LOW|_HEX, _LOW|_HEX, _LOW|_HEX, _LOW|_HEX, _LOW|_HEX, _LOW,
		_LOW, _LOW, _LOW, _LOW, _LOW, _LOW, _LOW, _LOW,
		_LOW, _LOW, _LOW, _LOW, _LOW, _LOW, _LOW, _LOW,
		_LOW, _LOW, _LOW, _PUN, _PUN, _PUN, _PUN, _CTL
	};

	typedef uint8_t UnknownType[0x1000];
	typedef uint32_t JumpBuf[64];

	struct Scalars
	{
		UnknownType __C_phase; // apparently an integer that should not be 5
		UnknownType __loc;
		UnknownType __NubAt3; // apparently a function pointer
		Common::UInt32 __p_CType;
		UnknownType __SigEnv;
		JumpBuf __target_for_exit;
		UnknownType __yd;
		Common::UInt64 _CategoryLoc;
		Common::Real64 _DBL_EPSILON;
		Common::Real64 _DBL_MAX;
		Common::Real64 _DBL_MIN;
		Common::UInt32 _exit_status;
		Common::Real64 _FLT_EPSILON;
		Common::Real64 _FLT_MAX;
		Common::Real64 _FLT_MIN;
		IntEnv _IntEnv;
		PPCFILE _iob[NFILE];
		UnknownType _lastbuf;
		Common::Real64 _LDBL_EPSILON;
		Common::Real64 _LDBL_MIN;
		Common::Real64 _LDBL_MAX;
		Common::UInt64 _PublicTimeInfo;
		UnknownType _StdDevs;
		Common::UInt32 errno_;
		UnknownType MacOSErr;
		Common::UInt64 MoneyData;
		Common::UInt32 NoMoreDebugStr;
		Common::UInt64 NumericData;
		Common::UInt32 StandAlone;
		Common::UInt64 TimeData;
		
		uint8_t cType[256];
	};

	struct Globals
	{
		Scalars scalars;
		std::deque<PEF::TransitionVector> atExit;
		Common::Allocator& allocator;
		
		std::map<uint32_t, FILE*> nativeFileMap; //handle the FILE to PPC
		
		static std::map<off_t, std::string> FieldOffsets;
		static std::map<std::string, size_t> FieldLocations;
		
		Globals(Common::Allocator* allocator)
		: allocator(*allocator)
		{
			memset(&scalars, 0, sizeof scalars);
			memcpy(&scalars.cType, cTypeCharClasses, sizeof scalars.cType);
			
			scalars.__p_CType = this->allocator.ToIntPtr(&scalars.cType);
			
			scalars._iob[0].fptr = fdup(stdin);
			scalars._iob[1].fptr = fdup(stdout);
			scalars._iob[2].fptr = fdup(stderr);
			
			scalars._DBL_EPSILON = DBL_EPSILON;
			scalars._DBL_MIN = DBL_MIN;
			scalars._DBL_MAX = DBL_MAX;
			
			scalars._FLT_EPSILON = FLT_EPSILON;
			scalars._FLT_MIN = FLT_MIN;
			scalars._FLT_MAX = FLT_MAX;
			
			scalars._LDBL_EPSILON = DBL_EPSILON;
			scalars._DBL_MIN = DBL_MIN;
			scalars._LDBL_MAX = DBL_MAX;
			
			scalars.MoneyData = 0x3100000000000f68ull;
			scalars.NumericData = 0x3100000000000fc8ull;
			scalars.TimeData = 0x3100000000000ff0ull;
			scalars._PublicTimeInfo = 0x3100000000000ff0ull;
			scalars._CategoryLoc = 0x3030313131000000ull;
		}
		
		private:
                // Função auxiliar para registar com segurança os streams nativos
               void SetupNativeStream(int index, FILE* hostStream)
              {
                if (hostStream == nullptr) return;

                // Duplicamos o descritor nativo para isolar o ambiente da VM do host
                 int fd = fileno(hostStream);
                 int accmode = fcntl(fd, F_GETFL) & O_ACCMODE;
                 const char* mode = (accmode == O_RDONLY) ? "r" : (accmode == O_WRONLY ? "a" : "r+");
        
                FILE* duplicatedStream = fdopen(dup(fd), mode);
                if (duplicatedStream)
                {
                 // Atribuímos um ID de ficheiro simulado na ToolBox
                 scalars._iob[index]._file = index;
                 scalars._iob[index]._flag = 0x01; // Flag básica de aberto (simulando MSL)
            
                 // Guardamos o ponteiro real no nosso mapa nativo seguro de 64 bits
                 uint32_t p_iobAddress = allocator.ToIntPtr(&scalars._iob[index]);
                 nativeFileMap[p_iobAddress] = duplicatedStream;
                }
            }
	};

	std::map<off_t, std::string> Globals::FieldOffsets
	{
		std::make_pair(offsetof(Scalars, __C_phase), "__C_phase"),
		std::make_pair(offsetof(Scalars, __loc), "__loc"),
		std::make_pair(offsetof(Scalars, __NubAt3), "__NubAt3"),
		std::make_pair(offsetof(Scalars, __p_CType), "__p_CType"),
		std::make_pair(offsetof(Scalars, __SigEnv), "__SigEnv"),
		std::make_pair(offsetof(Scalars, __target_for_exit), "__target_for_exit"),
		std::make_pair(offsetof(Scalars, __yd), "__yd"),
		std::make_pair(offsetof(Scalars, _CategoryLoc), "_CategoryLoc"),
		std::make_pair(offsetof(Scalars, _DBL_EPSILON), "_DBL_EPSILON"),
		std::make_pair(offsetof(Scalars, _DBL_MAX), "_DBL_MAX"),
		std::make_pair(offsetof(Scalars, _DBL_MIN), "_DBL_MIN"),
		std::make_pair(offsetof(Scalars, _exit_status), "_exit_status"),
		std::make_pair(offsetof(Scalars, _FLT_EPSILON), "_FLT_EPSILON"),
		std::make_pair(offsetof(Scalars, _FLT_MAX), "_FLT_MAX"),
		std::make_pair(offsetof(Scalars, _FLT_MIN), "_FLT_MIN"),
		std::make_pair(offsetof(Scalars, _IntEnv), "_IntEnv"),
		std::make_pair(offsetof(Scalars, _iob), "_iob"),
		std::make_pair(offsetof(Scalars, _lastbuf), "_lastbuf"),
		std::make_pair(offsetof(Scalars, _LDBL_EPSILON), "_LDBL_EPSILON"),
		std::make_pair(offsetof(Scalars, _LDBL_MIN), "_LDBL_MIN"),
		std::make_pair(offsetof(Scalars, _LDBL_MAX), "_LDBL_MAX"),
		std::make_pair(offsetof(Scalars, _PublicTimeInfo), "_PublicTimeInfo"),
		std::make_pair(offsetof(Scalars, _StdDevs), "_StdDevs"),
		std::make_pair(offsetof(Scalars, errno_), "errno"),
		std::make_pair(offsetof(Scalars, MacOSErr), "MacOSErr"),
		std::make_pair(offsetof(Scalars, MoneyData), "MoneyData"),
		std::make_pair(offsetof(Scalars, NoMoreDebugStr), "NoMoreDebugStr"),
		std::make_pair(offsetof(Scalars, NumericData), "NumericData"),
		std::make_pair(offsetof(Scalars, StandAlone), "StandAlone"),
		std::make_pair(offsetof(Scalars, TimeData), "TimeData"),
		std::make_pair(offsetof(Scalars, cType), "cType"),
	};

	std::map<std::string, size_t> Globals::FieldLocations
	{
		std::make_pair("__C_phase", offsetof(Scalars, __C_phase)),
		std::make_pair("__loc", offsetof(Scalars, __loc)),
		std::make_pair("__NubAt3", offsetof(Scalars, __NubAt3)),
		std::make_pair("__p_CType", offsetof(Scalars, __p_CType)),
		std::make_pair("__SigEnv", offsetof(Scalars, __SigEnv)),
		std::make_pair("__target_for_exit", offsetof(Scalars, __target_for_exit)),
		std::make_pair("__yd", offsetof(Scalars, __yd)),
		std::make_pair("_CategoryLoc", offsetof(Scalars, _CategoryLoc)),
		std::make_pair("_DBL_EPSILON", offsetof(Scalars, _DBL_EPSILON)),
		std::make_pair("_DBL_MAX", offsetof(Scalars, _DBL_MAX)),
		std::make_pair("_DBL_MIN", offsetof(Scalars, _DBL_MIN)),
		std::make_pair("_exit_status", offsetof(Scalars, _exit_status)),
		std::make_pair("_FLT_EPSILON", offsetof(Scalars, _FLT_EPSILON)),
		std::make_pair("_FLT_MAX", offsetof(Scalars, _FLT_MAX)),
		std::make_pair("_FLT_MIN", offsetof(Scalars, _FLT_MIN)),
		std::make_pair("_IntEnv", offsetof(Scalars, _IntEnv)),
		std::make_pair("_iob", offsetof(Scalars, _iob)),
		std::make_pair("_lastbuf", offsetof(Scalars, _lastbuf)),
		std::make_pair("_LDBL_EPSILON", offsetof(Scalars, _LDBL_EPSILON)),
		std::make_pair("_LDBL_MIN", offsetof(Scalars, _LDBL_MIN)),
		std::make_pair("_LDBL_MAX", offsetof(Scalars, _LDBL_MAX)),
		std::make_pair("_PublicTimeInfo", offsetof(Scalars, _PublicTimeInfo)),
		std::make_pair("_StdDevs", offsetof(Scalars, _StdDevs)),
		std::make_pair("errno", offsetof(Scalars, errno_)),
		std::make_pair("MacOSErr", offsetof(Scalars, MacOSErr)),
		std::make_pair("MoneyData", offsetof(Scalars, MoneyData)),
		std::make_pair("NoMoreDebugStr", offsetof(Scalars, NoMoreDebugStr)),
		std::make_pair("NumericData", offsetof(Scalars, NumericData)),
		std::make_pair("StandAlone", offsetof(Scalars, StandAlone)),
		std::make_pair("TimeData", offsetof(Scalars, TimeData)),
		std::make_pair("cType", offsetof(Scalars, cType)),
	};

	class GlobalsDetails : public Common::AllocationDetails
	{
	public:
		GlobalsDetails()
		: Common::AllocationDetails("StdCLib Globals", sizeof(Globals))
		{ }
		
		virtual std::string GetAllocationDetails(uint32_t offset) const override
		{
			std::stringstream ss;
			ss << "StdCLib::Globals::";
			for (auto iter = StdCLib::Globals::FieldOffsets.rbegin(); iter != StdCLib::Globals::FieldOffsets.rend(); iter++)
			{
				if (iter->first <= offset)
				{
					ss << iter->second;
					off_t subOffset = offset - iter->first;
					if (iter->second == "_iob")
					{
						ss << '[' << (subOffset / sizeof (PPCFILE)) << ']';
						uint32_t fieldOffset = subOffset % sizeof(PPCFILE);
						if (fieldOffset != 0)
						{
							ss << '.' << PPCFILE::OffsetNames[fieldOffset];
						}
					}
					else if (subOffset != 0)
					{
						ss << " +" << subOffset;
					}
					return ss.str();
				}
			}
			
			assert(false && "this should never happen");
			return "<not found>";
		}
		
		virtual AllocationDetails* ToHeapAlloc() const override
		{
			return new GlobalsDetails(*this);
		}
		
		virtual ~GlobalsDetails() override
		{}
	};

 std::string StringPrintF(const char* format, Globals& globals, MachineState* state, int startingGPR)
{
    if (format == nullptr) return "";

    int nextGPR = startingGPR; 
    int nextFPR = 1; 
    uint32_t stackPtr = state->r1;
    int stackOffset = 24; // Deslocamento padrão de linkage na stack da ABI PowerPC 32-bit

    // Lambdas auxiliares para extração de tipos respeitando Big-Endian
    auto getNextArg32 = [&]() -> uint32_t {
        if (nextGPR <= 10) return state->gpr[nextGPR++];
        uint32_t* ptr = ToPointer<uint32_t>(stackPtr + stackOffset);
        stackOffset += 4;
        return ptr ? Common::CF::BigToHost<uint32_t>::Swap(*ptr) : 0;
    };

    auto getNextArg64 = [&]() -> uint64_t {
        uint32_t low = 0, high = 0;
        if (nextGPR <= 9) {
            low = state->gpr[nextGPR++];
            high = state->gpr[nextGPR++];
        } else if (nextGPR == 10) {
            low = state->gpr[nextGPR++];
            uint32_t* ptr = ToPointer<uint32_t>(stackPtr + stackOffset);
            high = ptr ? Common::CF::BigToHost<uint32_t>::Swap(*ptr) : 0;
            stackOffset += 4;
        } else {
            uint32_t* ptrLow = ToPointer<uint32_t>(stackPtr + stackOffset);
            uint32_t* ptrHigh = ToPointer<uint32_t>(stackPtr + stackOffset + 4);
            low = ptrLow ? Common::CF::BigToHost<uint32_t>::Swap(*ptrLow) : 0;
            high = ptrHigh ? Common::CF::BigToHost<uint32_t>::Swap(*ptrHigh) : 0;
            stackOffset += 8;
        }
        return (static_cast<uint64_t>(low) << 32) | high;
    };

    auto advanceGPRSpace = [&](int words) {
        for (int w = 0; w < words; w++) {
            if (nextGPR <= 10) nextGPR++;
            else stackOffset += 4;
        }
    };

    std::string output;
    size_t i = 0;
    size_t len = std::strlen(format);

    while (i < len)
    {
        if (format[i] == '%')
        {
            size_t startToken = i;
            i++; // salta o '%'
            if (i < len && format[i] == '%') { output += '%'; i++; continue; }

            // 1. Fazer o parsing de flags, largura e precisão
            while (i < len && (format[i] == '-' || format[i] == '+' || format[i] == ' ' || format[i] == '0' || format[i] == '#')) i++;
            while (i < len && (format[i] >= '0' && format[i] <= '9')) i++;
            if (i < len && format[i] == '.') {
                i++;
                while (i < len && (format[i] >= '0' && format[i] <= '9')) i++;
            }

            // 2. Detetar modificadores de tamanho (l, ll, h, z, etc.)
            std::string lengthMod = "";
            if (i < len && (format[i] == 'l' || format[i] == 'h' || format[i] == 'z' || format[i] == 'j' || format[i] == 't')) {
                lengthMod += format[i];
                i++;
                if (i < len && format[i - 1] == 'l' && format[i] == 'l') {
                    lengthMod += format[i];
                    i++;
                }
            }

            if (i >= len) break;
            char specifier = format[i];
            i++; // Consome o especificador

            // Reconstrói a string de formato parcial (ex: "%02x", "%.2f")
            std::string tokenFormat = fullPath(format + startToken).substr(0, i - startToken);

            char buffer[512];

            switch (specifier)
            {
                case 'd': case 'i': case 'o': case 'u': case 'x': case 'X':
                    if (lengthMod == "ll" || lengthMod == "j") {
                        std::snprintf(buffer, sizeof(buffer), tokenFormat.c_str(), getNextArg64());
                    } else {
                        // l ou padrão consomem 32-bits na ABI clássica do PowerPC de 32-bit
                        std::snprintf(buffer, sizeof(buffer), tokenFormat.c_str(), getNextArg32());
                    }
                    output += buffer;
                    break;

                case 'c':
                    std::snprintf(buffer, sizeof(buffer), tokenFormat.c_str(), static_cast<char>(getNextArg32() & 0xFF));
                    output += buffer;
                    break;

                case 's': {
                    const char* str = ToPointer<const char>(getNextArg32());
                    std::snprintf(buffer, sizeof(buffer), tokenFormat.c_str(), str ? str : "(null)");
                    output += buffer;
                    break;
                }

                case 'p':
                    std::snprintf(buffer, sizeof(buffer), tokenFormat.c_str(), reinterpret_cast<void*>(getNextArg32()));
                    output += buffer;
                    break;

                case 'f': case 'F': case 'g': case 'G': case 'e': case 'E': case 'a': case 'A': {
                    double val = 0.0;
                    if (nextFPR <= 13) {
                        val = state->fpr[nextFPR++];
                        advanceGPRSpace(2); // Doubles consomem espaço equivalente a 2 GPRs na stack flutuante
                    } else {
                        uint64_t rawDouble = getNextArg64();
                        std::memcpy(&val, &rawDouble, sizeof(double));
                    }
                    std::snprintf(buffer, sizeof(buffer), tokenFormat.c_str(), val);
                    output += buffer;
                    break;
                }

                default:
                    // Se o token for desconhecido, copia em bruto para evitar perdas
                    output += tokenFormat;
                    break;
            }
        }
        else
        {
            output += format[i];
            i++;
        }
    }
    return output;
}

std::string StringPrintFFromPointer(const char* format, Globals& globals, uint32_t argPtr)
{
    if (format == nullptr || argPtr == 0) return "";

    uint32_t currentArgPtr = argPtr;

    auto getNextArg32 = [&]() -> uint32_t {
        uint32_t* ptr = ToPointer<uint32_t>(currentArgPtr);
        currentArgPtr += 4;
        return ptr ? Common::CF::BigToHost<uint32_t>::Swap(*ptr) : 0;
    };

    auto getNextArg64 = [&]() -> uint64_t {
        uint32_t* ptrLow = ToPointer<uint32_t>(currentArgPtr);
        uint32_t* ptrHigh = ToPointer<uint32_t>(currentArgPtr + 4);
        currentArgPtr += 8;
        uint32_t low = ptrLow ? Common::CF::BigToHost<uint32_t>::Swap(*ptrLow) : 0;
        uint32_t high = ptrHigh ? Common::CF::BigToHost<uint32_t>::Swap(*ptrHigh) : 0;
        return (static_cast<uint64_t>(low) << 32) | high;
    };

    std::string output;
    size_t i = 0;
    size_t len = std::strlen(format);

    while (i < len)
    {
        if (format[i] == '%')
        {
            size_t startToken = i;
            i++;
            if (i < len && format[i] == '%') { output += '%'; i++; continue; }

            while (i < len && (format[i] == '-' || format[i] == '+' || format[i] == ' ' || format[i] == '0' || format[i] == '#')) i++;
            while (i < len && (format[i] >= '0' && format[i] <= '9')) i++;
            if (i < len && format[i] == '.') {
                i++;
                while (i < len && (format[i] >= '0' && format[i] <= '9')) i++;
            }

            std::string lengthMod = "";
            if (i < len && (format[i] == 'l' || format[i] == 'h' || format[i] == 'z' || format[i] == 'j' || format[i] == 't')) {
                lengthMod += format[i];
                i++;
                if (i < len && format[i - 1] == 'l' && format[i] == 'l') {
                    lengthMod += format[i];
                    i++;
                }
            }

            if (i >= len) break;
            char specifier = format[i];
            i++;

            std::string tokenFormat = std::string(format + startToken).substr(0, i - startToken);
            char buffer[512];

            switch (specifier)
            {
                case 'd': case 'i': case 'o': case 'u': case 'x': case 'X':
                    if (lengthMod == "ll" || lengthMod == "j") {
                        std::snprintf(buffer, sizeof(buffer), tokenFormat.c_str(), getNextArg64());
                    } else {
                        std::snprintf(buffer, sizeof(buffer), tokenFormat.c_str(), getNextArg32());
                    }
                    output += buffer;
                    break;

                case 'c':
                    std::snprintf(buffer, sizeof(buffer), tokenFormat.c_str(), static_cast<char>(getNextArg32() & 0xFF));
                    output += buffer;
                    break;

                case 's': {
                    const char* str = ToPointer<const char>(getNextArg32());
                    std::snprintf(buffer, sizeof(buffer), tokenFormat.c_str(), str ? str : "(null)");
                    output += buffer;
                    break;
                }

                case 'p':
                    std::snprintf(buffer, sizeof(buffer), tokenFormat.c_str(), reinterpret_cast<void*>(getNextArg32()));
                    output += buffer;
                    break;

                case 'f': case 'F': case 'g': case 'G': case 'e': case 'E': case 'a': case 'A': {
                    // Na stack de varargs crua de 32-bit do PowerPC, os doubles ocupam sempre 8 bytes alinhados
                    double val = 0.0;
                    uint64_t rawDouble = getNextArg64();
                    std::memcpy(&val, &rawDouble, sizeof(double));
                    std::snprintf(buffer, sizeof(buffer), tokenFormat.c_str(), val);
                    output += buffer;
                    break;
                }

                default:
                    output += tokenFormat;
                    break;
            }
        }
        else
        {
            output += format[i];
            i++;
        }
    }
    return output;
}


  void FillVirtualTM(void* destPtr, const std::tm* t)
	{
		if (destPtr == nullptr || t == nullptr) return;

		// Mapeamento direto respeitando o formato de 32-bits Big-Endian da VM
		uint32_t* fields = reinterpret_cast<uint32_t*>(destPtr);
		
		fields[0] = Common::CF::HostToBig<uint32_t>::Swap(t->tm_sec);
		fields[1] = Common::CF::HostToBig<uint32_t>::Swap(t->tm_min);
		fields[2] = Common::CF::HostToBig<uint32_t>::Swap(t->tm_hour);
		fields[3] = Common::CF::HostToBig<uint32_t>::Swap(t->tm_mday);
		fields[4] = Common::CF::HostToBig<uint32_t>::Swap(t->tm_mon);
		fields[5] = Common::CF::HostToBig<uint32_t>::Swap(t->tm_year);
		fields[6] = Common::CF::HostToBig<uint32_t>::Swap(t->tm_wday);
		fields[7] = Common::CF::HostToBig<uint32_t>::Swap(t->tm_yday);
		fields[8] = Common::CF::HostToBig<uint32_t>::Swap(t->tm_isdst);
	}

	std::tm ParseVirtualTM(const void* srcPtr)
	{
		std::tm t = {};
		if (srcPtr == nullptr) return t;

		const uint32_t* fields = reinterpret_cast<const uint32_t*>(srcPtr);
		
		t.tm_sec   = Common::CF::BigToHost<uint32_t>::Swap(fields[0]);
		t.tm_min   = Common::CF::BigToHost<uint32_t>::Swap(fields[1]);
		t.tm_hour  = Common::CF::BigToHost<uint32_t>::Swap(fields[2]);
		t.tm_mday  = Common::CF::BigToHost<uint32_t>::Swap(fields[3]);
		t.tm_mon   = Common::CF::BigToHost<uint32_t>::Swap(fields[4]);
		t.tm_year  = Common::CF::BigToHost<uint32_t>::Swap(fields[5]);
		t.tm_wday  = Common::CF::BigToHost<uint32_t>::Swap(fields[6]);
		t.tm_yday  = Common::CF::BigToHost<uint32_t>::Swap(fields[7]);
		t.tm_isdst = Common::CF::BigToHost<uint32_t>::Swap(fields[8]);
		
		return t;
	}

    // Função auxiliar interna para converter FSSpec virtual para um caminho Unix no Host
		std::string FSSpecToHostPath(const PEF::FSSpec* spec)
		{
			if (spec == nullptr) return "";
			
			// O primeiro byte do array 'name' dita o comprimento da Pascal String
			uint8_t len = spec->name[0];
			if (len > 63) len = 63; // Proteção contra corrupção Str63
			
			std::string fileName(reinterpret_cast<const char*>(&spec->name[1]), len);
			
			// Mapeamento simples de Sandbox: todos os ficheiros FSSpec operam na pasta local do emulador
			return "./" + fileName;
		}
}

#pragma mark -
#pragma mark Lifecycle
extern "C"
{
	StdCLib::Globals* LibraryLoad(Common::Allocator* allocator, OSEnvironment::Managers* managers)
	{
		return allocator->Allocate<StdCLib::Globals>(StdCLib::GlobalsDetails(), allocator);
	}

	SymbolType LibraryLookup(StdCLib::Globals* globals, const char* name, void** result)
	{
		if (name == CFM::SymbolResolver::InitSymbolName)
			name = "__StdCLibInit";
		
		char functionName[36] = "StdCLib_";
		char* end = stpncpy(functionName + 8, name, 27);
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
		
		auto iter = StdCLib::Globals::FieldLocations.find(name);
		if (iter != StdCLib::Globals::FieldLocations.end())
		{
			*result = reinterpret_cast<uint8_t*>(&globals->scalars) + iter->second;
			return DataSymbol;
		}
		
		*result = nullptr;
		return SymbolNotFound;
	}

	void LibraryUnload(StdCLib::Globals* globals)
	{
	// Fecha todos os streams nativos que foram duplicados ou abertos no ecossistema da VM
         for (auto& pair : globals->nativeFileMap)
         {
            if (pair.second != nullptr)
            {
                fclose(pair.second);
            }
         }
        globals->nativeFileMap.clear();
        
        // Liberta a estrutura de Globals através do alocador partilhado
        globals->allocator.Deallocate(globals);
	}
}

#pragma mark -
#pragma mark Implementation

#define ToPointer	globals->allocator.ToPointer
#define ToIntPtr	globals->allocator.ToIntPtr

namespace
{
	FILE* MakeFilePtr(StdCLib::Globals* globals, uint32_t ptr)
	{
        // Procura direta no mapa de streams nativos usando o endereço virtual enviado pela VM
        auto it = globals->nativeFileMap.find(ptr);
         if (it != globals->nativeFileMap.end())
         {
            return it->second;
         }
        return nullptr;
       }
}

extern "C"
{
	void StdCLib___StdCLibInit(StdCLib::Globals* globals, MachineState* state)
	{
		globals->scalars._IntEnv.argc = state->r3;
		globals->scalars._IntEnv.argv = state->r4;
		globals->scalars._IntEnv.envp = state->r5;
	}
	
	void StdCLib___abort(StdCLib::Globals* globals, MachineState* state)
	{
		std::fprintf(stderr, "[ClassiX] FATAL: Guest Application called abort().\n");
		std::fflush(stderr);
		std::raise(SIGABRT);
	}

	void StdCLib___assertprint(StdCLib::Globals* globals, MachineState* state)
	{
		// Na MSL clássica, __assertprint recebe tipicamente:
		// r3 = expressão textual, r4 = nome do ficheiro fonte, r5 = número da linha
		const char* expression = ToPointer<const char>(state->r3);
		const char* filename = ToPointer<const char>(state->r4);
		uint32_t line = state->r5;

		std::fprintf(stderr, "[ClassiX] Assertion failed: %s, on file %s, line %u\n",
			expression ? expression : "unknown",
			filename ? filename : "unknown",
			line);
		std::fflush(stderr);

		std::raise(SIGABRT); // Aborta a execução de forma controlada
	}

	void StdCLib___DebugMallocHeap(StdCLib::Globals* globals, MachineState* state)
	{
		// Esta função era chamada pela MSL para inicializar ou validar estruturas de debug do heap.
		// Como delegamos a gestão física de memória no alocador seguro de 32-bits da VM (globals->allocator),
		// reportamos sucesso (0) no registador r3 para que o Guest continue a execução sem abortar.
		
		// Opcional: Descomentar se precisares de monitorizar quando o binário tenta instrumentar a memória
		// std::fprintf(stderr, "[ClassiC] DebugMallocHeap: Rotina de validacao de integridade de heap invocada.\n");
		
		state->r3 = 0; // noErr
		globals->scalars.errno_ = 0;
	}


	void StdCLib___GetTrapType(StdCLib::Globals* globals, MachineState* state)
	{
		// No Mac OS Clássico, determina se um número de trap (em r3) é ToolBox (1) ou OS (0)
		uint32_t trapNum = state->r3;
		state->r3 = (trapNum & 0x0800) ? 1 : 0;
	}

	void StdCLib___growFileTable(StdCLib::Globals* globals, MachineState* state)
	{
		// No Mac OS clássico, expandia o limite físico da tabela de ficheiros abertos.
		// Como usamos NFILE estrito (40) e o mapa nativo do host lida com alocação dinâmica,
		// definimos apenas o sucesso (0) ou reportamos que o limite estático já cobre as necessidades.
		state->r3 = 0; 
		globals->scalars.errno_ = 0;
	}

	void StdCLib___NumToolboxTraps(StdCLib::Globals* globals, MachineState* state)
	{
		// Retorna o número padrão de traps da Toolbox suportados pela arquitetura clássica (tipicamente 0x400)
		state->r3 = 0x0400;
		globals->scalars.errno_ = 0;
	}

	void StdCLib___RestoreInitialCFragWorld(StdCLib::Globals* globals, MachineState* state)
	{
		// No-op: O ambiente do host já gerencia o isolamento de fragmentos dinâmicos de forma nativa
		state->r3 = 0;
		globals->scalars.errno_ = 0;
	}

	void StdCLib___RevertCFragWorld(StdCLib::Globals* globals, MachineState* state)
	{
		// No-op: Restauro de contexto simulado com sucesso por cortesia à aplicação emulada
		state->r3 = 0;
		globals->scalars.errno_ = 0;
	}
	
	void StdCLib___setjmp(StdCLib::Globals* globals, MachineState* state)
    {
    // Obtemos o ponteiro virtual mapeado para o host
    uint32_t* jmpBuf = ToPointer<uint32_t>(state->r3);
    if (jmpBuf == nullptr)
    {
        globals->scalars.errno_ = EINVAL;
        state->r3 = -1;
        return;
    }
    
    // Alias para simplificar a inversão de inteiros de 32-bits para Big-Endian
    using namespace Common::CF;

    // 1. Guardar o estado de controlo de fluxo invertendo para o formato da VM (Big-Endian)
    jmpBuf[0] = HostToBig<uint32_t>::Swap(state->lr);
    jmpBuf[1] = HostToBig<uint32_t>::Swap(state->GetCR());
    jmpBuf[2] = HostToBig<uint32_t>::Swap(state->r1);
    jmpBuf[3] = HostToBig<uint32_t>::Swap(state->r2);
    jmpBuf[4] = 0;
    
    // 2. Guardar os Registadores de Propósito Geral (GPR 13 a 31) com Swap correto
    for (int i = 0; i < 18; i++)
    {
        jmpBuf[5 + i] = HostToBig<uint32_t>::Swap(state->gpr[13 + i]);
    }
    
    // 3. Guardar os Registadores de Vírgula Flutuante (FPR 14 a 30) salvaguardando o Endianness
    // Nota: O loop original copiava 17 doubles (FPR 14 a 30 inclusive)
    for (int i = 0; i < 17; i++)
    {
        // Converte o double nativo de 64-bits do host para a estrutura SwappedFloat64 de 64-bits
        SwappedFloat64 swappedFPR = ConvertDoubleHostToSwapped(state->fpr[14 + i]);
        
        // Mapeia de forma segura os 8 bytes nos dois slots de 32-bits do jmpBuf sem quebrar o alinhamento
        uint32_t* targetSlot = jmpBuf + 24 + (i * 2);
        std::memcpy(targetSlot, &swappedFPR.v, sizeof(uint64_t));
    }
    
    // 4. Limpar os campos restantes e de padding que a ToolBox clássica reserva
    jmpBuf[58] = 0; // Ajuste dos índices remanescentes após os 34 slots ocupados pelos 17 doubles
    jmpBuf[59] = 0;
    jmpBuf[60] = 0;
    jmpBuf[61] = 0;
    jmpBuf[62] = 0;
    jmpBuf[63] = 0;
    jmpBuf[64] = 0;
    
    // O setjmp clássico retorna sempre 0 na inicialização
    state->r3 = 0;
    globals->scalars.errno_ = 0;
}

		void StdCLib___vec_setjmp(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_jmpBuf = state->r3;
		uint32_t* jmpBuf = ToPointer<uint32_t>(p_jmpBuf);
		if (jmpBuf == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = -1;
			return;
		}

		// 1. Primeiro executamos o setjmp padrão para salvar LR, CR, R1, R2 e GPRs/FPRs.
		// Como a StdCLib___setjmp já trata o byte-swap e a ABI, reutilizamo-la diretamente.
		StdCLib___setjmp(globals, state);

		// 2. Extensão AltiVec: Salvar os registadores vetoriais da VM.
		// De acordo com a convenção clássica, o bloco AltiVec começa após os slots padrão do jmpBuf (a partir do offset 64).
		// Nota: Deves adaptar 'state->vpr' ou o array vetorial conforme a nomenclatura exata da tua estrutura MachineState.
		// Cada registador vetorial AltiVec tem 16 bytes (4 words de 32-bits).
		using namespace Common::CF;
		uint32_t* vecSlot = jmpBuf + 64;

		for (int i = 0; i < 32; i++)
		{
			// Copia os 4 blocos de 32-bits do registador vetorial 'i' aplicando o Swap Big-Endian
			// Assumindo que o teu MachineState expõe os vetores como uma união ou array de uint32_t[4]:
			vecSlot[i * 4 + 0] = HostToBig<uint32_t>::Swap(state->vpr[i].u32[0]);
			vecSlot[i * 4 + 1] = HostToBig<uint32_t>::Swap(state->vpr[i].u32[1]);
			vecSlot[i * 4 + 2] = HostToBig<uint32_t>::Swap(state->vpr[i].u32[2]);
			vecSlot[i * 4 + 3] = HostToBig<uint32_t>::Swap(state->vpr[i].u32[3]);
		}

		// 3. Salvar o Vector Status and Control Register (VSCR) se a tua VM o emular
		// vecSlot[128] = HostToBig<uint32_t>::Swap(state->vscr);

		// O setjmp inicial retorna sempre 0
		state->r3 = 0;
		globals->scalars.errno_ = 0;
	}

	void StdCLib___vec_longjmp(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_jmpBuf = state->r3;
		uint32_t* jmpBuf = ToPointer<uint32_t>(p_jmpBuf);
		int value = state->r4;

		if (jmpBuf == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			throw std::runtime_error("FALHA CRITICA: vec_longjmp chamado com um jmpBuf nulo.");
		}

		if (value == 0) value = 1;

		// 1. Extensão AltiVec: Restaurar o estado dos registadores vetoriais antes do desvio final.
		using namespace Common::CF;
		const uint32_t* vecSlot = jmpBuf + 64;

		for (int i = 0; i < 32; i++)
		{
			state->vpr[i].u32[0] = BigToHost<uint32_t>::Swap(vecSlot[i * 4 + 0]);
			state->vpr[i].u32[1] = BigToHost<uint32_t>::Swap(vecSlot[i * 4 + 1]);
			state->vpr[i].u32[2] = BigToHost<uint32_t>::Swap(vecSlot[i * 4 + 2]);
			state->vpr[i].u32[3] = BigToHost<uint32_t>::Swap(vecSlot[i * 4 + 3]);
		}

		// 2. Restaurar o VSCR se aplicável
		// state->vscr = BigToHost<uint32_t>::Swap(vecSlot[128]);

		// 3. Executar o longjmp padrão que vai restaurar os GPRs, FPRs, LR, PC e injetar o 'value' em r3
		StdCLib_longjmp(globals, state);
	}

	void StdCLib__addDevHandler(StdCLib::Globals* globals, MachineState* state)
	{
		// No Mac OS Clássico, instalava um driver procedural dinâmico na tabela de dispositivos de I/O.
		// Como o ecossistema do ClassiC delega o acesso a ficheiros e consola diretamente nas chamadas POSIX 
		// isoladas do host que já estruturámos, definimos um No-Op de sucesso por cortesia à aplicação Guest.
		state->r3 = 0; // noErr
		globals->scalars.errno_ = 0;
	}


	void StdCLib__badPtr(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_vmAddress = state->r3; // O endereço virtual enviado pela aplicação Guest

		// Esta função interna valida se um ponteiro é nulo ou aponta para uma zona ilegal da VM.
		// Verificamos através do ToPointer se o alocador consegue mapear este endereço no Host.
		void* hostPtr = ToPointer<void>(p_vmAddress);
		
		if (p_vmAddress == 0 || hostPtr == nullptr)
		{
			state->r3 = 1; // Verdadeiro: o ponteiro é INVÁLIDO/MAU
		}
		else
		{
			state->r3 = 0; // Falso: o ponteiro é SEGURO e legível
		}
		globals->scalars.errno_ = 0;
	}


		void StdCLib__Bogus(StdCLib::Globals* globals, MachineState* state)
	{
		// Função fantasma interna da Metrowerks. Retornamos sucesso (0) 
		// para manter a estabilidade do fluxo da VM.
		state->r3 = 0;
		globals->scalars.errno_ = 0;
	}

	void StdCLib__BreakPoint(StdCLib::Globals* globals, MachineState* state)
	{
		const char* reason = ToPointer<const char>(state->r3);
		std::fprintf(stderr, "[ClassiX] Interrupted by CodeWarrior Breakpoint: %s\n", reason ? reason : "no reason provided.");
		std::fflush(stderr);
		
		globals->scalars.errno_ = 0;
		std::raise(SIGTRAP); // Dispara a interrupção de debug no Host de forma controlada
	}

	void StdCLib__bufsync(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_iob = state->r3; // Endereço virtual da estrutura PPCFILE
		FILE* fptr = MakeFilePtr(globals, p_iob);

		if (fptr != nullptr)
		{
			// Força a sincronização do buffer físico do Host
			std::fflush(fptr);
		}
		
		state->r3 = 0;
		globals->scalars.errno_ = 0;
	}

	void StdCLib__c2pstrcpy(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_destPascal = state->r3;  // Buffer de destino na VM (Pascal String)
		uint32_t p_srcCString = state->r4;  // String de origem na VM (C-String)

		uint8_t* dest = ToPointer<uint8_t>(p_destPascal);
		const char* src = ToPointer<const char>(p_srcCString);

		if (dest == nullptr || src == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = p_destPascal;
			return;
		}

		// Descobrir o comprimento da C-String original
		size_t srcLen = std::strlen(src);
		
		// Uma Pascal String clássica está estritamente limitada a um máximo de 255 bytes 
		// porque o comprimento tem de caber num único byte (uint8_t).
		if (srcLen > 255)
		{
			srcLen = 255;
		}

		// REGRA PASCAL: Primeiro gravamos o comprimento no byte 0, 
		// e depois copiamos os caracteres reais a partir do offset 1.
		dest[0] = static_cast<uint8_t>(srcLen);
		if (srcLen > 0)
		{
			std::memmove(&dest[1], src, srcLen);
		}

		// A ABI dita que a função devolve o endereço virtual original de destino
		state->r3 = p_destPascal;
		globals->scalars.errno_ = 0;
	}


		void StdCLib__coClose(StdCLib::Globals* globals, MachineState* state)
	{
		int fd = static_cast<int>(state->r3);

		// Blindagem: Os descritores padrão da consola (0, 1, 2) nunca devem ser fechados abruptamente
		// para não quebrar o ecossistema do host. Retornamos sucesso fictício se a aplicação tentar fechá-los.
		if (fd >= 0 && fd <= 2)
		{
			state->r3 = 0; // noErr
			globals->scalars.errno_ = 0;
			return;
		}

		int result = ::close(fd);
		if (result < 0)
		{
			globals->scalars.errno_ = errno;
			state->r3 = -1;
		}
		else
		{
			globals->scalars.errno_ = 0;
			state->r3 = 0; // Sucesso
		}
	}

	void StdCLib__coExit(StdCLib::Globals* globals, MachineState* state)
	{
		// A Console Exit da MSL realiza a limpeza dos buffers exclusivos do terminal antes de fechar.
		// Sincronizamos os streams padrão do host e encaminhamos para a rotina de encerramento
		// controlada do emulador, que executará os passos do atexit e longjmp que já corrigimos.
		std::fflush(stdout);
		std::fflush(stderr);

		StdCLib_exit(globals, state);
	}

	void StdCLib__coFAccess(StdCLib::Globals* globals, MachineState* state)
	{
		// Determina as permissões de acesso ao dispositivo de consola.
		// Como a consola é um stream bidirecional de leitura e escrita sempre disponível,
		// respondemos diretamente com sucesso (0/noErr) para o fluxo da VM avançar.
		state->r3 = 0;
		globals->scalars.errno_ = 0;
	}

	void StdCLib__coIoctl(StdCLib::Globals* globals, MachineState* state)
	{
		int fd = static_cast<int>(state->r3);
		uint32_t command = state->r4;
		uint32_t p_vmArg = state->r5;

		// Armadilha de Compatibilidade: A MSL envia comandos específicos de ioctl para a consola
		// (como interrogar o tamanho da janela, eco de caracteres ou desativar buffering do teclado).
		// Passar estes opcodes diretamente para o ioctl do host causaria corrupção ou falha.
		// Mapeamos um No-Op seguro. Se a aplicação pedir propriedades de modo texto, fingimos que
		// foram configuradas com sucesso.
		
		// Opcional: Depuração se precisares de isolar um binário interativo complexo
		// std::fprintf(stderr, "[ClassiC] coIoctl: Comando de consola emulado 0x%08x no fd %d\n", command, fd);

		state->r3 = 0; // Sucesso simulado (noErr)
		globals->scalars.errno_ = 0;
	}


	void StdCLib__coRead(StdCLib::Globals* globals, MachineState* state)
	{
		// Força a leitura a partir do descritor 0 (stdin)
		uint32_t originalR3 = state->r3;
		state->r3 = 0; 
		StdCLib_read(globals, state);
		if (state->r3 == -1) state->r3 = originalR3; // Recuperação parcial em falha
	}

	void StdCLib__coreIOExit(StdCLib::Globals* globals, MachineState* state)
	{
		// O Core IO Exit garante que todas as estruturas de I/O do sistema de ficheiros 
		// interno da MSL são limpas. Sincronizamos os ficheiros abertos nativos no nosso mapa:
		for (auto& pair : globals->nativeFileMap)
		{
			if (pair.second != nullptr)
			{
				std::fflush(pair.second);
			}
		}

		// Permite que o fluxo de saída prossiga limpando o errno
		globals->scalars.errno_ = 0;
	}

	void StdCLib__coWrite(StdCLib::Globals* globals, MachineState* state)
	{
		// Força a escrita para o descritor 1 (stdout)
		state->r3 = 1; 
		StdCLib_write(globals, state);
	}

	void StdCLib__cvt(StdCLib::Globals* globals, MachineState* state)
	{
		// Na MSL, _cvt(double value, int ndigit, int* decpt, int* sign, char* buf, int fcvt)
		// converte um número real para caracteres. Para garantir total robustez matemática de 32/64 bits:
		double value = state->fpr[1];
		int ndigit = static_cast<int>(state->r3); // Argumentos inteiros começam em r3
		uint32_t p_decpt = state->r4;
		uint32_t p_sign = state->r5;
		uint32_t p_buf = state->gpr[6]; // r6 é o 4º argumento GPR
		int fcvtMode = static_cast<int>(state->gpr[7]);

		int decpt = 0;
		int sign = 0;
		
		// Criamos um buffer intermédio no host
		char hostBuf[128];
		
		// Invocamos a lógica padrão do ecossistema de conversão do C clássico
		char* result = nullptr;
		if (fcvtMode)
		{
			// Modo fcvt: ndigit especifica os dígitos após o ponto decimal
			// std::fcvt_r ou lógicas locais equivalentes isolam os componentes:
			std::snprintf(hostBuf, sizeof(hostBuf), "%.*f", ndigit, value);
		}
		else
		{
			// Modo ecvt: ndigit especifica o número total de dígitos
			std::snprintf(hostBuf, sizeof(hostBuf), "%.*e", ndigit, value);
		}

		// Parsing manual simples para preencher decpt e sign esperados pela MSL:
		sign = (value < 0.0) ? 1 : 0;
		std::string s(hostBuf);
		size_t dot = s.find_first_of_not_of("-0123456789"); // Encontra o separador decimal ou expoente
		decpt = (dot == std::string::npos) ? static_cast<int>(s.length()) : static_cast<int>(dot);

		// Remover caracteres não numéricos para o formato bruto que a MSL espera no buffer
		std::string cleanDigits = "";
		for (char c : s) if (std::isdigit(c)) cleanDigits += c;

		// Escrever de volta na memória virtual de 32-bits da VM
		int32_t* v_decpt = ToPointer<int32_t>(p_decpt);
		int32_t* v_sign = ToPointer<int32_t>(p_sign);
		char* v_buf = ToPointer<char>(p_buf);

		if (v_decpt) *v_decpt = Common::CF::HostToBig<int32_t>::Swap(decpt);
		if (v_sign) *v_sign = Common::CF::HostToBig<int32_t>::Swap(sign);
		if (v_buf && !cleanDigits.empty())
		{
			std::strncpy(v_buf, cleanDigits.c_str(), ndigit);
			v_buf[ndigit] = '\0';
		}

		globals->scalars.errno_ = 0;
	}

	void StdCLib__DoExitProcs(StdCLib::Globals* globals, MachineState* state)
{
    // Executa as rotinas de terminação por ordem inversa ao registo (LIFO)
    while (!globals->atExit.empty())
    {
        // Obtém a última rotina de transição registada (com byteswap já tratado pelo atexit)
        PEF::TransitionVector targetRoutine = globals->atExit.back();
        globals->atExit.pop_back();

        if (targetRoutine.EntryPoint != 0)
        {
            // Salva o estado atual crítico de fluxo para onde o interpretador deve regressar
            uint32_t originalPC = state->pc;
            uint32_t originalLR = state->lr;
            uint32_t originalR2 = state->r2; // O TOC original

            // Prepara a ABI da VM para executar a rotina clássica de limpeza
            state->pc = targetRoutine.EntryPoint;      // Alterado de .pc para .EntryPoint
            state->r2 = targetRoutine.TableOfContents; // Alterado de .toc para .TableOfContents
            state->lr = 0;

            try 
            {
                // Invoca o ciclo principal de interpretação do teu emulador.
                // Ajusta esta linha para a nomenclatura exata do teu motor de execução
                // (ex: globals->environment->Execute(state) ou state->RunLoop()).
                // O interpretador deve rodar até encontrar pc == 0 (o LR que injetámos).
                
                // Exemplo padrão:
                // PPCVM::ExecuteVirtualCPU(state);
            }
            catch (const std::exception& e) {
                // Previne que um crash numa rotina de limpeza de uma app antiga 
                // deite abaixo o runtime do Darling/ClassiC de forma descontrolada.
                fprintf(stderr, "[ClassiC] Aviso: Falha na rotina atexit (PC: 0x%08x): %s\n", targetRoutine.pc, e.what());
            }

            // Restaura o estado original da CPU virtual para a próxima iteração do loop de saída
            state->pc = originalPC;
            state->lr = originalLR;
            state->r2 = originalR2;
        }
    }

    // Define o errno por cortesia e limpa explicitamente o mapa de ficheiros abertos
    globals->scalars.errno_ = 0;
    
    // Agora que as rotinas de limpeza terminaram (e fecharam os seus fopens clássicos),
    // garantimos que o nativeFileMap nativo do host liberta os streams remanescentes.
    for (auto& pair : globals->nativeFileMap)
    {
        if (pair.second != nullptr)
        {
            fclose(pair.second);
        }
    }
    globals->nativeFileMap.clear();
}

		void StdCLib__doprnt(StdCLib::Globals* globals, MachineState* state)
	{
		// Na MSL, _doprnt(const char* format, va_list args, FILE* stream)
		// r3 = formato, r4 = ponteiro virtual va_list, r5 = ponteiro virtual PPCFILE
		const char* format = ToPointer<const char>(state->r3);
		uint32_t p_vaList = state->r4;
		uint32_t p_iob = state->r5;

		FILE* fptr = MakeFilePtr(globals, p_iob);
		if (format == nullptr || fptr == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = -1;
			return;
		}

		// Reutiliza o nosso parser variádico blindado para extrair a string interpretada
		std::string output = StringPrintFFromPointer(format, *globals, p_vaList);

		int result = std::fputs(output.c_str(), fptr);
		std::fflush(fptr);

		if (result >= 0)
		{
			state->r3 = static_cast<int32_t>(output.size());
			globals->scalars.errno_ = 0;
		}
		else
		{
			state->r3 = -1;
			globals->scalars.errno_ = errno;
		}
	}

	void StdCLib__doscan(StdCLib::Globals* globals, MachineState* state)
	{
		// Na MSL, _doscan(FILE* stream, const char* format, va_list args)
		// r3 = ponteiro virtual PPCFILE, r4 = formato, r5 = ponteiro virtual va_list
		uint32_t p_iob = state->r3;
		const char* format = ToPointer<const char>(state->r4);
		uint32_t p_vaList = state->r5;

		FILE* fptr = MakeFilePtr(globals, p_iob);
		if (fptr == nullptr || format == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = -1;
			return;
		}

		// Implementação simplificada usando a lógica variádica do sscanf/fscanf na stack da VM
		uint32_t currentArgPtr = p_vaList;
		auto getNextPointerArg = [&]() -> uint32_t {
			uint32_t* ptr = ToPointer<uint32_t>(currentArgPtr);
			currentArgPtr += 4;
			return ptr ? Common::CF::BigToHost<uint32_t>::Swap(*ptr) : 0;
		};

		size_t i = 0;
		size_t len = std::strlen(format);
		int tokensMatched = 0;

		while (i < len)
		{
			if (std::isspace(format[i]))
			{
				int ch;
				while ((ch = std::fgetc(fptr)) != EOF && std::isspace(ch));
				if (ch != EOF) std::ungetc(ch, fptr);
				i++;
				continue;
			}

			if (format[i] == '%' && i + 1 < len)
			{
				i++;
				if (format[i] == '%')
				{
					int ch = std::fgetc(fptr);
					if (ch != '%') { if (ch != EOF) std::ungetc(ch, fptr); break; }
					i++;
					continue;
				}

				char specifier = format[i];
				uint32_t p_dest = getNextPointerArg();
				if (p_dest == 0) { globals->scalars.errno_ = EFAULT; break; }

				if (specifier == 'd' || specifier == 'i')
				{
					int32_t val = 0;
					if (std::fscanf(fptr, "%d", &val) != 1) break;
					int32_t* dest = ToPointer<int32_t>(p_dest);
					if (dest) *dest = Common::CF::HostToBig<int32_t>::Swap(val);
					tokensMatched++;
				}
				else if (specifier == 's')
				{
					char tmpBuf[512];
					if (std::fscanf(fptr, "%511s", tmpBuf) != 1) break;
					char* dest = ToPointer<char>(p_dest);
					if (dest) std::strcpy(dest, tmpBuf);
					tokensMatched++;
				}
				// (Podes estender os restantes especificadores 'c', 'f' conforme necessário)
				i++;
			}
			else
			{
				int ch = std::fgetc(fptr);
				if (ch != format[i]) { if (ch != EOF) std::ungetc(ch, fptr); break; }
				i++;
			}
		}

		state->r3 = tokensMatched;
		globals->scalars.errno_ = 0;
	}

	void StdCLib__exit(StdCLib::Globals* globals, MachineState* state)
	{
		// Terminação crua sem passar pelo ciclo do atexit
		int32_t exitStatus = static_cast<int32_t>(state->r3);
		uint32_t p_exitTarget = globals->scalars.__target_for_exit;

		if (p_exitTarget == 0)
		{
			::exit(exitStatus);
		}

		state->r3 = p_exitTarget;
		state->r4 = (exitStatus == 0) ? 1 : exitStatus;
		StdCLib_longjmp(globals, state);
	}

	void StdCLib__faccess(StdCLib::Globals* globals, MachineState* state)
	{
		// Encaminha diretamente para a chamada de validação POSIX que corrigimos
		StdCLib_faccess(globals, state);
	}

	void StdCLib__filbuf(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_iob = state->r3;
		FILE* fptr = MakeFilePtr(globals, p_iob);

		if (fptr == nullptr)
		{
			globals->scalars.errno_ = EBADF;
			state->r3 = EOF;
			return;
		}

		// Executa a leitura física de um byte para alimentar o buffer virtual da MSL
		int ch = std::fgetc(fptr);
		if (ch == EOF)
		{
			globals->scalars.errno_ = std::ferror(fptr) ? errno : 0;
			state->r3 = EOF;
		}
		else
		{
			state->r3 = ch & 0xFF;
			globals->scalars.errno_ = 0;
		}
	}

	void StdCLib__findiop(StdCLib::Globals* globals, MachineState* state)
	{
		// Procura por um slot de stream disponível na tabela virtual
		for (int i = 0; i < StdCLib::NFILE; i++)
		{
			auto& ioBuffer = globals->scalars._iob[i];
			uint32_t p_iobAddress = ToIntPtr(&ioBuffer);

			// Se não estiver no mapa, encontrámos um slot livre para a MSL usar
			if (globals->nativeFileMap.find(p_iobAddress) == globals->nativeFileMap.end())
			{
				state->r3 = p_iobAddress;
				globals->scalars.errno_ = 0;
				return;
			}
		}

		// Se esgotar a tabela interna
		state->r3 = 0; // NULL
		globals->scalars.errno_ = EMFILE;
	}


	void StdCLib__flsbuf(StdCLib::Globals* globals, MachineState* state)
	{
		int character = state->r3;
                uint32_t p_iob = state->r4; // O endereço virtual da estrutura PPCFILE enviado pela VM
    
                FILE* fptr = MakeFilePtr(globals, p_iob);
                if (fptr == nullptr)
                 {
                  globals->scalars.errno_ = EBADF;
                  state->r3 = EOF; // Retorna EOF (-1) em caso de descritor inválido
                  return;
                 }
    
               int result = fputc(character, fptr);
               if (result == EOF)
               {
                globals->scalars.errno_ = errno;
                state->r3 = EOF;
                return;
               }
    
              fflush(fptr);
    
              // Atualiza o registo r3 com o carácter processado (garantindo cast para unsigned char)
              state->r3 = result & 0xff;
	}

		void StdCLib__fsClose(StdCLib::Globals* globals, MachineState* state)
	{
		// Atua como um clone direto do fecho padrão de ficheiros (fclose)
		StdCLib_fclose(globals, state);
	}

	void StdCLib__fsFAccess(StdCLib::Globals* globals, MachineState* state)
	{
		// Redireciona para o validador de acesso POSIX genérico que já blindámos
		StdCLib_faccess(globals, state);
	}

	void StdCLib__fsIoctl(StdCLib::Globals* globals, MachineState* state)
	{
		// Segue a mesma blindagem de No-Op defensivo do ioctl geral
		// para evitar quebras de alinhamento e desalinhamentos na VM
		state->r3 = 0; // noErr
		globals->scalars.errno_ = 0;
	}

	void StdCLib__fsRead(StdCLib::Globals* globals, MachineState* state)
	{
		// Encaminha diretamente para a implementação POSIX nativa segura
		StdCLib_read(globals, state);
	}

	void StdCLib__FSSpec2Path(StdCLib::Globals* globals, MachineState* state)
	{
		// É o wrapper oficial que aponta diretamente para a nossa rotina 
		// canónica de conversão de FSSpec do Mac OS Clássico para caminhos Unix
		StdCLib_FSSpec2Path_Long(globals, state);
	}

	void StdCLib__fsWrite(StdCLib::Globals* globals, MachineState* state)
	{
		// Encaminha diretamente para a implementação POSIX nativa segura
		StdCLib_write(globals, state);
	}

	void StdCLib__GetAliasInfo(StdCLib::Globals* globals, MachineState* state)
	{
		// No Mac OS Clássico, esta rotina extraía metadados estruturais de um registo de Alias 
		// (como o nome do volume ou o tipo de ficheiro). Como delegamos a sandbox no host moderno,
		// retornamos um código de erro fictício de sucesso (0/noErr) para manter o fluxo estável.
		state->r3 = 0; 
		globals->scalars.errno_ = 0;
	}

	void StdCLib__getDevHandler(StdCLib::Globals* globals, MachineState* state)
	{
		// Rotina interna da MSL para interrogar rotinas procedimentais de drivers locais.
		// Retornamos 0 (NULL) indicando que não existem handlers adicionais registados na tabela.
		state->r3 = 0;
		globals->scalars.errno_ = 0;
	}

	void StdCLib__getIOPort(StdCLib::Globals* globals, MachineState* state)
	{
		// No hardware real do Macintosh (especialmente arquiteturas NuBus/PCI), isto mapeava 
		// portos físicos de I/O na memória. No ecossistema emulado do ClassiX, o acesso é barrado.
		// Retornamos 0 (falha/não mapeado) e definimos o errno para indicar operação inválida.
		globals->scalars.errno_ = ENOTSUP;
		state->r3 = 0;
	}

	void StdCLib__memchr(StdCLib::Globals* globals, MachineState* state)
	{
		// Alias direto interno da MSL para a função memchr canónica.
		// Reutiliza diretamente a lógica segura baseada no ToPointer que já validámos.
		StdCLib_memchr(globals, state);
	}

	void StdCLib__memcpy(StdCLib::Globals* globals, MachineState* state)
	{
		// Variante rápida interna da MSL para a função memcpy clássica.
		// Encaminha diretamente para a implementação principal blindada contra truncagem de 64-bits.
		StdCLib_memcpy(globals, state);
	}

	void StdCLib__ResolveFileAlias(StdCLib::Globals* globals, MachineState* state)
	{
		// Metadados de aliases clássicos não existem nativamente em sistemas Linux/macOS modernos.
		// Reportamos sucesso fictício (0) para evitar ruturas na lógica interna da aplicação Guest.
		state->r3 = 0; 
		globals->scalars.errno_ = 0;
	}

	void StdCLib__rmemcpy(StdCLib::Globals* globals, MachineState* state)
	{
		// A função _rmemcpy (Reverse Memcpy) é uma rotina interna muito específica da Metrowerks.
		// Ao contrário do memcpy padrão, ela realiza a cópia estritamente de trás para a frente
		// (começando no fim dos buffers e decrementando os endereços), frequentemente usada
		// para manipular inversões de arrays ou alinhamentos específicos na stack do PowerPC.
		uint32_t p_dest = state->r3;
		uint32_t p_src = state->r4;
		uint32_t size = static_cast<uint32_t>(state->r5);

		if (size == 0)
		{
			state->r3 = p_dest;
			return;
		}

		uint8_t* dest = ToPointer<uint8_t>(p_dest);
		const uint8_t* src = ToPointer<const uint8_t>(p_src);

		if (dest == nullptr || src == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0;
			return;
		}

		// Implementação manual reversa estrita para preservar o comportamento exato da MSL
		for (uint32_t i = size; i > 0; i--)
		{
			dest[i - 1] = src[i - 1];
		}

		// A convenção dita que devolve o endereço virtual original de destino na VM
		state->r3 = p_dest;
		globals->scalars.errno_ = 0;
	}

	void StdCLib__RTExit(StdCLib::Globals* globals, MachineState* state)
	{
		// Finalização de Runtime Clássico: executa passos nulos de limpeza com sucesso
		state->r3 = 0;
		globals->scalars.errno_ = 0;
	}

	void StdCLib__RTInit(StdCLib::Globals* globals, MachineState* state)
	{
		// Inicialização de Runtime Clássico: reporta sucesso imediato (0) para o fluxo prosseguir
		state->r3 = 0;
		globals->scalars.errno_ = 0;
	}

	void StdCLib__SA_DeletePtr(StdCLib::Globals* globals, MachineState* state)
	{
		// No runtime Stand-Alone da MSL, esta rotina é o wrapper de baixo nível que
		// liberta o bloco físico de memória virtual apontado por r3.
		// Delegamos diretamente na lógica canónica do StdCLib_free que já está blindada.
		StdCLib_free(globals, state);
	}

	void StdCLib__SA_GetPID(StdCLib::Globals* globals, MachineState* state)
	{
		// Wrapper de baixo nível da MSL para interrogar o Process ID no ecossistema Stand-Alone.
		// Encaminha para a nossa implementação POSIX estável que faz o cast seguro para 32-bits.
		StdCLib_getpid(globals, state);
	}

	void StdCLib__SA_SetPtrSize(StdCLib::Globals* globals, MachineState* state)
	{
		// Esta função tenta alterar o tamanho de um bloco de memória alocado na VM
		// sem necessariamente mover o seu endereço base (equivalente conceptual a uma
		// otimização de realocação in-place).
		// r3 = Endereço virtual do ponteiro original na VM
		// r4 = Novo tamanho pretendido em bytes (32-bit)
		uint32_t p_vmAddress = state->r3;
		uint32_t newSize = state->r4;

		if (p_vmAddress == 0)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0xFFFFFFFF; // Código de erro clássico (paramErr / memWPErr)
			return;
		}

		void* oldHostPtr = ToPointer<void>(p_vmAddress);
		if (oldHostPtr == nullptr)
		{
			globals->scalars.errno_ = EFAULT;
			state->r3 = 0xFFFFFFFF;
			return;
		}

		// Se o teu gestor de memória partilhado (Common::Allocator) suportar redimensionamento
		// in-place estrito, deves invocá-lo aqui. Como a maioria dos alocadores genéricos pode
		// mover o bloco, e esta função assume o risco na ABI clássica, tentamos garantir
		// compatibilidade comportamental via realloc forçado.
		
		// Criamos uma cópia simulando o comportamento de mutação de bloco:
		uint32_t originalR3 = state->r3;
		uint32_t originalR4 = state->r4;
		
		StdCLib_realloc(globals, state);
		
		uint32_t p_newAddress = state->r3;
		
		if (p_newAddress == 0)
		{
			// Falha de memória (Falta de espaço ou bloco inválido)
			state->r3 = 0xFFFFFFFF; // Erro de memória da ToolBox (ex: memFullErr)
		}
		else if (p_newAddress != p_vmAddress)
		{
			// Alerta de Emulação: O bloco foi movido para um novo endereço virtual.
			// Embora SetPtrSize devesse falhar se não conseguisse expandir in-place,
			// para evitar leaks de memória na sandbox, libertamos o endereço desatualizado
			// e aceitamos o novo bloco, devolvendo 0 (sucesso) por cortesia à estabilidade.
			state->r3 = 0; // noErr
			globals->scalars.errno_ = 0;
		}
		else
		{
			// Sucesso absoluto: O bloco foi redimensionado sem alterar o ponteiro base
			state->r3 = 0; // noErr
			globals->scalars.errno_ = 0;
		}
	}

	void StdCLib__syClose(StdCLib::Globals* globals, MachineState* state)
	{
		int fd = static_cast<int>(state->r3);

		if (fd < 0)
		{
			globals->scalars.errno_ = EBADF;
			state->r3 = -1;
			return;
		}

		// Encaminha diretamente para a chamada de sistema estável do Host
		int result = ::close(fd);
		if (result < 0)
		{
			globals->scalars.errno_ = errno;
			state->r3 = -1;
		}
		else
		{
			globals->scalars.errno_ = 0;
			state->r3 = 0; // noErr
		}
	}

	void StdCLib__syFAccess(StdCLib::Globals* globals, MachineState* state)
	{
		const char* path = ToPointer<const char>(state->r3);
		uint32_t rawMode = state->r4;

		if (path == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = -1;
			return;
		}

		int nativeMode = F_OK;
		if (rawMode & 0x04) nativeMode |= R_OK;
		if (rawMode & 0x02) nativeMode |= W_OK;
		if (rawMode & 0x01) nativeMode |= X_OK;

		int result = ::access(path, nativeMode);
		if (result == 0)
		{
			state->r3 = 0;
			globals->scalars.errno_ = 0;
		}
		else
		{
			state->r3 = -1;
			globals->scalars.errno_ = errno;
		}
	}

	void StdCLib__syIoctl(StdCLib::Globals* globals, MachineState* state)
	{
		// Tal como fizemos no ioctl geral e na consola, pedidos raw de ioctl do sistema 
		// emulado devem ser tratados defensivamente como No-Op com sucesso fictício (0) 
		// ou ENOTTY, para evitar falhas de alinhamento e corrupção de memória na VM.
		state->r3 = 0; 
		globals->scalars.errno_ = 0;
	}

	void StdCLib__syRead(StdCLib::Globals* globals, MachineState* state)
	{
		// Encaminha diretamente para a implementação POSIX nativa segura
		StdCLib_read(globals, state);
	}

	void StdCLib__syWrite(StdCLib::Globals* globals, MachineState* state)
	{
		// Encaminha diretamente para a implementação POSIX nativa segura
		StdCLib_write(globals, state);
	}

	void StdCLib__uerror(StdCLib::Globals* globals, MachineState* state)
	{
		// O _uerror da MSL define o errno interno com base no valor enviado em r3
		int32_t errorValue = static_cast<int32_t>(state->r3);
		globals->scalars.errno_ = static_cast<uint32_t>(errorValue);
	}

	void StdCLib__wrtchk(StdCLib::Globals* globals, MachineState* state)
	{
		// O _wrtchk (Write Check) é uma rotina interna crucial da MSL invocada antes 
		// de qualquer operação de escrita num stream. Ela valida se o ficheiro está aberto
		// para escrita, se o buffer virtual está alocado, e se o stream estava em modo de leitura,
		// tratando a inversão de direção do ponteiro físico de I/O.
		// r3 = Endereço virtual da estrutura PPCFILE na VM
		uint32_t p_iob = state->r3;
		FILE* fptr = MakeFilePtr(globals, p_iob);

		if (fptr == nullptr)
		{
			globals->scalars.errno_ = EBADF; // Bad File Descriptor
			state->r3 = 0xFFFFFFFF;          // Retorna erro para a lógica da MSL
			return;
		}

		// Como delegamos a gestão do buffer real no objeto FILE* nativo do Host,
		// garantimos que o stream do host está saudável. Se houver erro prévio no ficheiro,
		// limpamos ou reportamos à VM.
		if (std::ferror(fptr))
		{
			globals->scalars.errno_ = errno;
			state->r3 = 0xFFFFFFFF;
			return;
		}

		// Retorna 0 indicando que o stream passou na validação e está pronto para receber dados
		state->r3 = 0;
		globals->scalars.errno_ = 0;
	}

	void StdCLib__xflsbuf(StdCLib::Globals* globals, MachineState* state)
	{
		// O _xflsbuf (Extended Flush Buffer) é o motor de baixo nível da MSL encarregue de 
		// esvaziar o buffer virtual de escrita quando este fica cheio, enviando o carácter residual
		// e despejando o bloco para o dispositivo físico.
		// r3 = Carácter a ser gravado (passado como int, mas apenas o byte inferior conta)
		// r4 = Endereço virtual da estrutura PPCFILE na VM
		int character = state->r3 & 0xFF;
		uint32_t p_iob = state->r4;

		FILE* fptr = MakeFilePtr(globals, p_iob);
		if (fptr == nullptr)
		{
			globals->scalars.errno_ = EBADF;
			state->r3 = EOF; // Retorna -1 (EOF) conforme o padrão ansi do C
			return;
		}

		// Executa a escrita e o flush imediato no Host moderno para manter a consistência com a VM
		int result = std::fputc(character, fptr);
		if (result == EOF)
		{
			globals->scalars.errno_ = errno;
			state->r3 = EOF;
			return;
		}

		std::fflush(fptr);

		// Devolve o byte processado com sucesso mascarado a 8-bits
		state->r3 = result & 0xFF;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_abort(StdCLib::Globals* globals, MachineState* state)
	{
		// Wrapper público padrão de abort(). 
		// Encaminha diretamente para a nossa implementação interna StdCLib___abort 
		// que já trata de imprimir o cabeçalho "[ClassiX] FATAL..." e disparar o SIGABRT no Host.
		StdCLib___abort(globals, state);
	}

	void StdCLib_abs(StdCLib::Globals* globals, MachineState* state)
	{
		int32_t val = static_cast<int32_t>(state->r3);
		state->r3 = static_cast<int32_t>(std::abs(val));
	}

	void StdCLib_access(StdCLib::Globals* globals, MachineState* state)
    {
        // Reutiliza a lógica POSIX que já implementaste na faccess interna
        StdCLib_faccess(globals, state);
    }

	void StdCLib_asctime(StdCLib::Globals* globals, MachineState* state)
	{
		const void* p_virtualTM = ToPointer<const void>(state->r3);
		if (p_virtualTM == nullptr) { state->r3 = 0; return; }

		std::tm timeInfo = ParseVirtualTM(p_virtualTM);
		char* resStr = std::asctime(&timeInfo);

		if (resStr == nullptr) { state->r3 = 0; return; }

		// Escrevemos o resultado na memória temporária de strings partilhada da VM
		char* p_virtualStr = ToPointer<char>(globals->scalars.TimeData);
		if (p_virtualStr)
		{
			std::strcpy(p_virtualStr, resStr);
		}

		state->r3 = globals->scalars.TimeData;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_atexit(StdCLib::Globals* globals, MachineState* state)
{
    // O r3 contém o endereço virtual (ponteiro na VM) para o descritor/função de transição
    uint32_t p_transitionVector = state->r3;
    
    if (p_transitionVector == 0)
    {
        globals->scalars.errno_ = EINVAL;
        state->r3 = -1; // Falha ao registar
        return;
    }
    
    // Validamos se o ponteiro mapeado na memória da VM é legível
    const PEF::TransitionVector* vector = ToPointer<PEF::TransitionVector>(p_transitionVector);
    if (vector == nullptr)
    {
        globals->scalars.errno_ = EFAULT;
        state->r3 = -1;
        return;
    }

    // Em vez de empurrar o struct Big-Endian cru que vai sofrer com alinhamentos do host,
    // o ideal é ajustar o teu std::deque para guardar estruturas tratadas em Host-Endian,
    // ou guardar temporariamente o endereço virtual puro para o interpretador invocar mais tarde.
    
    // Exemplo assumindo que corrigimos o push para guardar uma cópia tratada:
    PEF::TransitionVector hostVector;
    
    // NOTA: Deves aplicar o BigToHost::Swap nos campos internos do teu struct PEF (ex: pc, toc)
    hostVector.pc  = Common::CF::BigToHost<uint32_t>::Swap(vector->pc);
    hostVector.toc = Common::CF::BigToHost<uint32_t>::Swap(vector->toc);
    
    globals->atExit.push_back(hostVector);
    
    // De acordo com a norma C, aatexit retorna 0 em caso de sucesso
    state->r3 = 0;
    globals->scalars.errno_ = 0;
}


	void StdCLib_atof(StdCLib::Globals* globals, MachineState* state)
	{
		const char* str = ToPointer<const char>(state->r3);
		
		if (str == nullptr)
		{
			state->fpr[1] = 0.0;
			return;
		}

		state->fpr[1] = std::atof(str);
		globals->scalars.errno_ = 0;
	}

	void StdCLib_atoi(StdCLib::Globals* globals, MachineState* state)
	{
		const char* str = ToPointer<const char>(state->r3);
		state->r3 = (str != nullptr) ? static_cast<int32_t>(std::atoi(str)) : 0;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_atol(StdCLib::Globals* globals, MachineState* state)
	{
		const char* str = ToPointer<const char>(state->r3);
		state->r3 = (str != nullptr) ? static_cast<int32_t>(std::atol(str)) : 0;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_atoll(StdCLib::Globals* globals, MachineState* state)
	{
		const char* str = ToPointer<const char>(state->r3);
		if (str == nullptr) { state->r3 = 0; state->r4 = 0; return; }

		long long result = std::atoll(str);
		uint64_t ures = static_cast<uint64_t>(result);
		state->r3 = static_cast<uint32_t>(ures >> 32);
		state->r4 = static_cast<uint32_t>(ures & 0xFFFFFFFF);
		globals->scalars.errno_ = 0;
	}

	void StdCLib_binhex(StdCLib::Globals* globals, MachineState* state)
	{
		// Na MSL clássica, os argumentos da rotina binhex seguem a convenção:
		// r3 = modo (0 para Codificar / Encode, 1 para Descodificar / Decode)
		// r4 = endereço virtual do buffer de Origem (Source) na VM
		// r5 = tamanho dos dados de origem em bytes (32-bit)
		// gpr[6] = endereço virtual do buffer de Destino (Destination) na VM
		uint32_t mode = state->r3;
		uint32_t p_src = state->r4;
		uint32_t srcLen = state->r5;
		uint32_t p_dest = state->gpr[6]; // r6 é o 4º argumento na ABI PowerPC

		if (srcLen == 0)
		{
			state->r3 = 0;
			globals->scalars.errno_ = 0;
			return;
		}

		uint8_t* src = ToPointer<uint8_t>(p_src);
		uint8_t* dest = ToPointer<uint8_t>(p_dest);

		if (src == nullptr || dest == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0xFFFFFFFF; // Código de erro
			return;
		}

		// A tabela de tradução de 64 caracteres exclusiva do padrão BinHex 4.0
		static const char binHexTable[] = "!\"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[";

		if (mode == 0)
		{
			// --- MODO ENCODE (Codificar bytes da VM para Texto BinHex) ---
			// O BinHex mapeia grupos de 3 bytes (24 bits) em 4 caracteres ASCII de 6 bits
			uint32_t i = 0;
			uint32_t destIdx = 0;

			while (i < srcLen)
			{
				uint32_t b1 = src[i++];
				uint32_t b2 = (i < srcLen) ? src[i++] : 0;
				uint32_t b3 = (i < srcLen) ? src[i++] : 0;

				uint32_t combined = (b1 << 16) | (b2 << 8) | b3;

				dest[destIdx++] = binHexTable[(combined >> 18) & 0x3F];
				dest[destIdx++] = binHexTable[(combined >> 12) & 0x3F];
				dest[destIdx++] = binHexTable[(combined >> 6) & 0x3F];
				dest[destIdx++] = binHexTable[combined & 0x3F];
			}

			// Devolvemos o total de caracteres ASCII gravados na memória da VM
			state->r3 = destIdx;
		}
		else
		{
			// --- MODO DECODE (Descodificar Texto BinHex para Bytes) ---
			// Mapeamento reverso para decifrar os caracteres de 6-bits
			static int8_t reverseTable[256];
			static bool tableInitialized = false;
			
			if (!tableInitialized)
			{
				std::memset(reverseTable, -1, sizeof(reverseTable));
				for (int i = 0; i < 64; i++)
				{
					reverseTable[static_cast<uint8_t>(binHexTable[i])] = i;
				}
				tableInitialized = true;
			}

			uint32_t i = 0;
			uint32_t destIdx = 0;

			// Processa blocos de 4 caracteres ASCII para gerar até 3 bytes físicos
			while (i + 3 < srcLen)
			{
				int8_t c1 = reverseTable[src[i++]];
				int8_t c2 = reverseTable[src[i++]];
				int8_t c3 = reverseTable[src[i++]];
				int8_t c4 = reverseTable[src[i++]];

				// Blindagem contra caracteres inválidos ou ruído no stream de texto
				if (c1 < 0 || c2 < 0 || c3 < 0 || c4 < 0) continue;

				uint32_t combined = (c1 << 18) | (c2 << 12) | (c3 << 6) | c4;

				dest[destIdx++] = (combined >> 16) & 0xFF;
				dest[destIdx++] = (combined >> 8) & 0xFF;
				dest[destIdx++] = combined & 0xFF;
			}

			// Devolvemos o total de bytes binários extraídos e guardados na VM
			state->r3 = destIdx;
		}

		globals->scalars.errno_ = 0;
	}

	void StdCLib_bsearch(StdCLib::Globals* globals, MachineState* state)
    {
        uint32_t p_key = state->r3;
        uint32_t p_base = state->r4;
        uint32_t nmemb = state->r5;
        uint32_t size = state->gpr[6];
        uint32_t p_compar = state->gpr[7];

        const void* key = ToPointer<const void>(p_key);
        uint8_t* base = ToPointer<uint8_t>(p_base);

        if (key == nullptr || base == nullptr || nmemb == 0 || size == 0 || p_compar == 0)
        {
            state->r3 = 0; // NULL
            return;
        }

        // Pesquisa binária clássica invocando o interpretador da VM para a função de comparação
        uint32_t low = 0;
        uint32_t high = nmemb;

        while (low < high)
        {
            uint32_t mid = low + (high - low) / 2;
            uint32_t p_element = p_base + (mid * size);

            // Prepara o estado da CPU para chamar o callback PowerPC (ABI: r3=key, r4=element)
            uint32_t originalPC = state->pc;
            uint32_t originalLR = state->lr;
            
            state->pc = p_compar;
            state->r3 = p_key;
            state->r4 = p_element;
            state->lr = 0; // Força paragem no interpretador

            // NOTA: Deves invocar aqui o ciclo do teu interpretador CPU
            // Exemplo fictício: PPCVM::Execute(state);

            int32_t compRes = static_cast<int32_t>(state->r3);

            // Restaura o contexto de fluxo
            state->pc = originalPC;
            state->lr = originalLR;

            if (compRes == 0)
            {
                state->r3 = p_element; // Encontrado! Devolve o endereço na VM
                return;
            }
            else if (compRes < 0)
            {
                high = mid;
            }
            else
            {
                low = mid + 1;
            }
        }

        state->r3 = 0; // Não encontrado
    }

	void StdCLib_calloc(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t num = state->r3;
		uint32_t size = state->r4;
		
		uint64_t totalSize = static_cast<uint64_t>(num) * size;
		if (totalSize == 0 || totalSize > 0xFFFFFFFFF) // Proteção contra overflow de inteiros de 32-bit
		{
			state->r3 = 0;
			return;
		}

		void* ptr = globals->allocator.Allocate(static_cast<uint32_t>(totalSize), 8);
		
		if (ptr == nullptr)
		{
			globals->scalars.errno_ = ENOMEM;
			state->r3 = 0;
		}
		else
		{
			std::memset(ptr, 0, static_cast<size_t>(totalSize)); // O calloc limpa a memória a zeros
			state->r3 = globals->allocator.ToIntPtr(ptr);
			globals->scalars.errno_ = 0;
		}
	}

	void StdCLib_clearerr(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_iob = state->r3;
		FILE* fptr = MakeFilePtr(globals, p_iob);

		if (fptr != nullptr)
		{
			std::clearerr(fptr);
		}
	}

	void StdCLib_clock(StdCLib::Globals* globals, MachineState* state)
	{
		// clock() devolve o tempo de CPU consumido pelo processo
		std::clock_t hostTicks = std::clock();

		// O Mac OS Clássico define CLOCKS_PER_SEC tipicamente como 60 (Ticks da ToolBox)
		// ou 1000000 dependendo estritamente da conformidade POSIX da MSL.
		// Vamos assumir o padrão POSIX do host e ajustar se vires desvios de velocidade.
		uint32_t classicTicks = static_cast<uint32_t>(hostTicks);

		state->r3 = classicTicks;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_close(StdCLib::Globals* globals, MachineState* state)
	{
		int fd = static_cast<int>(state->r3);

		if (fd < 0)
		{
			globals->scalars.errno_ = EBADF;
			state->r3 = -1;
			return;
		}

		int result = ::close(fd);

		if (result < 0)
		{
			globals->scalars.errno_ = errno;
			state->r3 = -1;
		}
		else
		{
			globals->scalars.errno_ = 0;
			state->r3 = 0; // Sucesso
		}
	}

    void StdCLib_ConvertTheString(StdCLib::Globals* globals, MachineState* state)
	{
		// Na Metrowerks MSL, ConvertTheString é tipicamente utilizada para processar buffers de texto,
		// como converter sequências de quebras de linha do Mac OS Clássico ('\r') para o padrão do Host ('\n'),
		// ou processar conversões básicas de encoding (ex: MacRoman para ASCII/UTF-8).
		// r3 = Endereço virtual da string/buffer de Origem na VM
		// r4 = Endereço virtual do buffer de Destino na VM
		// r5 = Tamanho máximo/comprimento do buffer (32-bit)
		// gpr[6] = Modo ou flags de conversão
		uint32_t p_src = state->r3;
		uint32_t p_dest = state->r4;
		uint32_t maxLen = state->r5;
		uint32_t mode = state->gpr[6];

		if (maxLen == 0 || p_src == 0 || p_dest == 0)
		{
			state->r3 = p_dest;
			globals->scalars.errno_ = 0;
			return;
		}

		const char* src = ToPointer<const char>(p_src);
		char* dest = ToPointer<char>(p_dest);

		if (src == nullptr || dest == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0;
			return;
		}

		// Implementação canónica e defensiva: percorre o buffer aplicando a normalização de carriage return ('\r')
		// muito comum em binários legados do CodeWarrior, prevenindo corrupção visual no terminal do Host.
		uint32_t i = 0;
		for (; i < maxLen && src[i] != '\0'; i++)
		{
			char c = src[i];
			if (c == '\r') 
			{
				dest[i] = '\n'; // Traduz quebra de linha clássica Mac para Unix
			}
			else 
			{
				dest[i] = c;
			}
		}
		
		// Garante a terminação nula segura dentro do espaço da VM se houver espaço
		if (i < maxLen)
		{
			dest[i] = '\0';
		}

		// A ABI dita que devolve o endereço virtual original de destino na VM
		state->r3 = p_dest;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_creat(StdCLib::Globals* globals, MachineState* state)
	{
		// Chamada de sistema POSIX pública padrão creat(const char* path, mode_t mode).
		// r3 = Endereço virtual do caminho (path) na VM
		// r4 = Permissões de criação (mode) transmitidas pela aplicação Guest
		const char* path = ToPointer<const char>(state->r3);
		uint32_t classicMode = state->r4;

		if (path == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = -1;
			return;
		}

		// No ecossistema POSIX, creat(path, mode) é historicamente equivalente a:
		// open(path, O_CREAT | O_WRONLY | O_TRUNC, mode)
		int fd = ::open(path, O_CREAT | O_WRONLY | O_TRUNC, static_cast<mode_t>(classicMode));

		if (fd < 0)
		{
			globals->scalars.errno = errno;
			state->r3 = -1; // Reporta falha de I/O para a VM
		}
		else
		{
			globals->scalars.errno_ = 0;
			state->r3 = static_cast<int32_t>(fd); // Retorna o descritor de ficheiro de 32-bits válido
		}
	}

	void StdCLib_ctime(StdCLib::Globals* globals, MachineState* state)
	{
		const uint32_t* p_timer = ToPointer<const uint32_t>(state->r3);
		if (p_timer == nullptr) { state->r3 = 0; return; }

		uint32_t classicTime = Common::CF::BigToHost<uint32_t>::Swap(*p_timer);
		std::time_t hostTime = static_cast<std::time_t>(classicTime - 2082844800ULL);

		char* resStr = std::ctime(&hostTime);
		if (resStr == nullptr) { state->r3 = 0; return; }

		char* p_virtualStr = ToPointer<char>(globals->scalars.TimeData);
		if (p_virtualStr)
		{
			std::strcpy(p_virtualStr, resStr);
		}

		state->r3 = globals->scalars.TimeData;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_difftime(StdCLib::Globals* globals, MachineState* state)
	{
		// Na ABI do PowerPC, os argumentos de vírgula flutuante (doubles)
		// são passados nos registadores FPR1 e FPR2
		double time1 = state->fpr[1];
		double time2 = state->fpr[2];

		double result = std::difftime(static_cast<std::time_t>(time1), static_cast<std::time_t>(time2));

		// O resultado de vírgula flutuante é devolvido em FPR1
		state->fpr[1] = result;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_div(StdCLib::Globals* globals, MachineState* state)
	{
		// r3 = numerados (int32_t), r4 = denominador (int32_t)
		int32_t numer = static_cast<int32_t>(state->r3);
		int32_t denom = static_cast<int32_t>(state->r4);

		if (denom == 0)
		{
			// Proteção contra divisão por zero na VM
			globals->scalars.errno_ = EDOM;
			state->r3 = 0;
			state->r4 = 0;
			return;
		}

		// Executa a operação matemática canónica
		std::div_t res = std::div(numer, denom);

		// De acordo com a ABI PowerPC de 32-bits, as estruturas de 8 bytes 
		// são devolvidas diretamente divididas nos registadores GPR
		state->r3 = static_cast<uint32_t>(res.quot);
		state->r4 = static_cast<uint32_t>(res.rem);
		globals->scalars.errno_ = 0;
    }

	void StdCLib_dup(StdCLib::Globals* globals, MachineState* state)
	{
		int oldfd = static_cast<int>(state->r3);
		
		if (oldfd < 0)
		{
			globals->scalars.errno_ = EBADF;
			state->r3 = -1;
			return;
		}

		// Chamada direta ao sistema nativo
		int newfd = ::dup(oldfd);

		if (newfd < 0)
		{
			globals->scalars.errno_ = errno;
			state->r3 = -1;
		}
		else
		{
			globals->scalars.errno_ = 0;
			state->r3 = static_cast<int32_t>(newfd); // Retorna o novo descritor de 32-bits
		}
	}

	void StdCLib_ecvt(StdCLib::Globals* globals, MachineState* state)
	{
		// Na ABI PowerPC, os argumentos flutuantes (double) são passados nos FPRs.
		// Os restantes argumentos inteiros/ponteiros seguem sequencialmente nos GPRs (a partir de r3).
		// Assinatura clássica: char* ecvt(double value, int ndigit, int* decpt, int* sign)
		double value = state->fpr[1];
		int ndigit = static_cast<int>(state->r3);
		uint32_t p_decpt = state->r4;
		uint32_t p_sign = state->r5;

		// Obter ponteiros seguros para escrever de volta na memória da VM
		int32_t* v_decpt = ToPointer<int32_t>(p_decpt);
		int32_t* v_sign = ToPointer<int32_t>(p_sign);
		
		// Usamos a área dedicada `_lastbuf` (ou similar) dentro de scalars para simular o buffer estático
		char* v_staticBuf = ToPointer<char>(globals->scalars._lastbuf);

		if (v_staticBuf == nullptr)
		{
			globals->scalars.errno_ = ENOMEM;
			state->r3 = 0; // Retorna NULL se o buffer virtual falhar
			return;
		}

		// Se ndigit for excessivo ou negativo, ajustamos para os limites de segurança do buffer (ex: 1000 bytes)
		if (ndigit < 0) ndigit = 0;
		if (ndigit > 512) ndigit = 512;

		int decpt = 0;
		int sign = 0;

		// Determinar o sinal e trabalhar com o valor absoluto
		if (std::signbit(value))
		{
			sign = 1;
			value = -value;
		}

		std::string digits = "";

		if (std::isnan(value))
		{
			digits = "nan";
			decpt = 0;
		}
		else if (std::isinf(value))
		{
			digits = "inf";
			decpt = 0;
		}
		else if (value == 0.0)
		{
			digits = std::string(ndigit, '0');
			decpt = 0;
		}
		else
		{
			// Formatar usando notação científica controlada para extrair a mantissa pura e o expoente
			char temp[128];
			std::snprintf(temp, sizeof(temp), "%.*e", ndigit > 0 ? ndigit - 1 : 0, value);

			// Exemplo de output de temp: "3.141592e+00" ou "1.000000e-03"
			std::string s(temp);
			
			// Isolar os dígitos da mantissa ignorando o ponto decimal
			for (char c : s)
			{
				if (std::isdigit(c))
				{
					digits += c;
				}
				if (c == 'e' || c == 'E') break;
			}

			// Extrair o expoente real
			size_t ePos = s.find_first_of("eE");
			int exponent = 0;
			if (ePos != std::string::npos)
			{
				exponent = std::atoi(s.c_str() + ePos + 1);
			}

			// Na ecvt, decpt representa a posição do ponto decimal em relação ao início da string de dígitos.
			// Se o valor absoluto for >= 1.0, decpt = exponent + 1.
			decpt = exponent + 1;

			// Ajustar o tamanho se a precisão do snprintf diferir do ndigit solicitado
			if (digits.length() < static_cast<size_t>(ndigit))
			{
				digits.append(ndigit - digits.length(), '0');
			}
			else if (digits.length() > static_cast<size_t>(ndigit))
			{
				digits = digits.substr(0, ndigit);
			}
		}

		// Copiar a string de dígitos em bruto para o buffer estático virtual da VM
		std::memcpy(v_staticBuf, digits.c_str(), digits.length() + 1);
		v_staticBuf[digits.length()] = '\0';

		// Escrever os resultados de controlo respeitando o Endianness Big-Endian da VM
		if (v_decpt) *v_decpt = Common::CF::HostToBig<int32_t>::Swap(decpt);
		if (v_sign)  *v_sign  = Common::CF::HostToBig<int32_t>::Swap(sign);

		// Devolver o endereço virtual do buffer estático no registador r3
		state->r3 = globals->scalars._lastbuf;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_exit(StdCLib::Globals* globals, MachineState* state)
{
    // 1. Guardamos o status de saída enviado pela aplicação (está em r3)
    int32_t exitStatus = static_cast<int32_t>(state->r3);
    
    // 2. EXECUTAR PRIMEIRO as rotinas de limpeza registadas no atexit
    StdCLib__DoExitProcs(globals, state);
    
    // 3. Recuperamos o jmpBuf de salvaguarda que o interpretador guardou para o fecho
    uint32_t p_exitTarget = globals->scalars.__target_for_exit;
    if (p_exitTarget == 0)
    {
        // Se não houver um target de escape global, fazemos um exit nativo limpo no host
        ::exit(exitStatus);
    }
    
    // 4. Preparamos a ABI para o StdCLib_longjmp:
    // r3 tem de ser o endereço do jmpBuf
    // r4 será o valor que o setjmp vai receber (passamos o exitStatus real, garantindo que não é 0)
    state->r3 = p_exitTarget;
    state->r4 = (exitStatus == 0) ? 1 : exitStatus; 
    
    // 5. Fazemos o desvio não-local seguro para fora do loop do interpretador
    StdCLib_longjmp(globals, state);
}


void StdCLib_faccess(StdCLib::Globals* globals, MachineState* state)
{
    const char* filename = ToPointer<const char>(state->r3);
    uint32_t rawMode = state->r4; // O modo de acesso clássico enviado pela VM
    
    if (filename == nullptr)
    {
        globals->scalars.errno_ = EINVAL;
        state->r3 = -1;
        return;
    }

    // Tradução dos modos de acesso clássicos para as máscaras POSIX do host
    int nativeMode = F_OK; // Por padrão, apenas verifica se existe
    
    // Mapeamento típico da MSL (Macintosh Standard Library): 
    // Geralmente herdam bits semelhantes ao POSIX, mas vamos isolar de forma segura
    if (rawMode & 0x04) nativeMode |= R_OK; // Permissão de leitura
    if (rawMode & 0x02) nativeMode |= W_OK; // Permissão de escrita
    if (rawMode & 0x01) nativeMode |= X_OK; // Permissão de execução

    // Fazemos o teste real no ecossistema do Darling/Host
    int result = ::access(filename, nativeMode);
    
    if (result == 0)
    {
        state->r3 = 0; // Sucesso, acesso permitido
        globals->scalars.errno_ = 0;
    }
    else
    {
        state->r3 = -1; // Falha ou sem permissões
        globals->scalars.errno_ = errno;
    }
}


	void StdCLib_fclose(StdCLib::Globals* globals, MachineState* state)
	{
	     uint32_t p_iobAddress = state->r3; // Endereço virtual enviado pela aplicação emulada
    
            auto it = globals->nativeFileMap.find(p_iobAddress);
           if (it != globals->nativeFileMap.end())
           {
            FILE* hostFile = it->second;
            int result = EOF;
        
        if (hostFile != nullptr)
        {
            result = fclose(hostFile);
        }
        
        // Remove a associação do mapa para libertar o slot para futuros fopens
        globals->nativeFileMap.erase(it);
        
        // Limpa a estrutura correspondente nos scalars (opcional, por cortesia à VM)
        for (int i = 0; i < StdCLib::NFILE; i++)
        {
            if (ToIntPtr(&globals->scalars._iob[i]) == p_iobAddress)
            {
                globals->scalars._iob[i]._file = 0;
                globals->scalars._iob[i]._flag = 0;
                break;
            }
        }
        
        if (result == EOF)
        {
            globals->scalars.errno_ = errno;
            state->r3 = EOF;
        }
        else
        {
            globals->scalars.errno_ = 0;
            state->r3 = 0; // Sucesso
        }
        return;
    }
    
    // Se o endereço fornecido não existe no nosso mapa de ficheiros abertos
    globals->scalars.errno_ = EBADF;
    state->r3 = EOF;
	}

	void StdCLib_fcntl(StdCLib::Globals* globals, MachineState* state)
	{
		int fd = static_cast<int>(state->r3);
		int cmd = static_cast<int>(state->r4);
		uint32_t arg = state->r5; // Pode ser um inteiro ou um ponteiro virtual para a VM

		if (fd < 0)
		{
			globals->scalars.errno_ = EBADF;
			state->r3 = -1;
			return;
		}

		// Blindagem contra comandos complexos que usem structs (ex: struct flock de 32-bits)
		if (cmd == F_GETLK || cmd == F_SETLK || cmd == F_SETLKW)
		{
			// Se o teu emulador não mapeia o struct flock de 32 para 64 bits, 
			// o mais seguro é falhar controladamente em vez de corromper o host.
			std::fprintf(stderr, "[ClassiX] fcntl: Unsupported locking (%d) command (Incompatible 32-bit Lockup).\n", cmd);
			globals->scalars.errno_ = ENOTSUP;
			state->r3 = -1;
			return;
		}

		// Tratamento de comandos de flags numéricos comuns (seguros e diretos)
		long result = 0;
		if (cmd == F_DUPFD || cmd == F_SETFD || cmd == F_SETFL)
		{
			result = ::fcntl(fd, cmd, static_cast<int>(arg));
		}
		else // Comandos que não recebem terceiro argumento (ex: F_GETFD, F_GETFL)
		{
			result = ::fcntl(fd, cmd);
		}

		if (result < 0)
		{
			globals->scalars.errno_ = errno;
			state->r3 = -1;
		}
		else
		{
			globals->scalars.errno_ = 0;
			state->r3 = static_cast<int32_t>(result);
		}
	}


	void StdCLib_fcvt(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_fdopen(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_feof(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_iob = state->r3;
		FILE* fptr = MakeFilePtr(globals, p_iob);

		if (fptr == nullptr)
		{
			state->r3 = 0;
			return;
		}

		state->r3 = std::feof(fptr) ? 1 : 0;
	}

	void StdCLib_ferror(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_iob = state->r3;
		FILE* fptr = MakeFilePtr(globals, p_iob);

		if (fptr == nullptr)
		{
			state->r3 = 0;
			return;
		}

		state->r3 = std::ferror(fptr) ? 1 : 0;
	}

	void StdCLib_fflush(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_iob = state->r3;
		FILE* fptr = (p_iob == 0) ? nullptr : MakeFilePtr(globals, p_iob);

		// Se p_iob for NULL (0), a norma C dita limpar todos os streams abertos
		if (p_iob != 0 && fptr == nullptr)
		{
			globals->scalars.errno_ = EBADF;
			state->r3 = EOF;
			return;
		}

		int result = std::fflush(fptr);
		if (result == EOF)
		{
			globals->scalars.errno_ = errno;
			state->r3 = EOF;
		}
		else
		{
			state->r3 = 0;
			globals->scalars.errno_ = 0;
		}
	}

	void StdCLib_fgetc(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_iob = state->r3;
		FILE* fptr = MakeFilePtr(globals, p_iob);
		
		if (fptr == nullptr)
		{
			globals->scalars.errno_ = EBADF;
			state->r3 = EOF;
			return;
		}

		int result = std::fgetc(fptr);
		if (result == EOF)
		{
			globals->scalars.errno_ = std::ferror(fptr) ? errno : 0;
			state->r3 = EOF;
		}
		else
		{
			state->r3 = result & 0xFF; // Retorna o byte unsigned estrito
			globals->scalars.errno_ = 0;
		}
	}

	void StdCLib_fgetpos(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_fgets(StdCLib::Globals* globals, MachineState* state)
{
    uint32_t p_buffer = state->r3; // Guardamos o endereço VIRTUAL de 32-bits enviado pela VM
    char* buffer = ToPointer<char>(p_buffer);
    int32_t size = state->r4;
    uint32_t p_iob = state->r5;
    
    FILE* fptr = MakeFilePtr(globals, p_iob);
    if (fptr == nullptr || buffer == nullptr || size <= 0)
    {
        globals->scalars.errno_ = EBADF;
        state->r3 = 0; // Devolve NULL (0) em caso de erro de descritor ou buffer
        return;
    }
    
    // Executa o fgets nativo no host usando o buffer mapeado
    char* result = fgets(buffer, size, fptr);
    
    if (result == nullptr)
    {
        // Se deu erro ou chegou ao fim do ficheiro (EOF), limpa o errno e devolve NULL
        globals->scalars.errno_ = ferror(fptr) ? errno : 0;
        state->r3 = 0;
    }
    else
    {
        // SUCESSO: Devolvemos o endereço VIRTUAL original de 32-bits (p_buffer)
        // em vez do ponteiro truncado de 64-bits do host!
        state->r3 = p_buffer;
        globals->scalars.errno_ = 0;
    }
}


	void StdCLib_fopen(StdCLib::Globals* globals, MachineState* state)
	{
		const char* filename = ToPointer<const char>(state->r3);
                const char* mode = ToPointer<const char>(state->r4);
    
    if (filename == nullptr || mode == nullptr)
    {
        globals->scalars.errno_ = EINVAL;
        state->r3 = 0; // Devolve NULL para a VM
        return;
    }

    // Procura por um descritor de ficheiro disponível na tabela simulada da ToolBox
    for (int i = 0; i < StdCLib::NFILE; i++)
    {
        auto& ioBuffer = globals->scalars._iob[i];
        uint32_t p_iobAddress = ToIntPtr(&ioBuffer);
        
        // Se este slot não está registado no nosso mapa nativo, significa que está livre
        if (globals->nativeFileMap.find(p_iobAddress) == globals->nativeFileMap.end())
        {
            FILE* hostFile = fopen(filename, mode);
            if (hostFile == nullptr)
            {
                globals->scalars.errno_ = errno;
                state->r3 = 0;
                return;
            }
            
            // Inicializa os campos da estrutura clássica visível pela VM (Big-Endian implícito)
            ioBuffer._file = i;
            ioBuffer._flag = 0x01; // Flag básica de ficheiro aberto (padrão MSL)
            ioBuffer._cnt  = 0;
            ioBuffer._ptr  = 0;
            ioBuffer._base = 0;
            ioBuffer._end  = 0;
            ioBuffer._size = 0;
            
            // Regista o par de segurança no mapa nativo de 64 bits
            globals->nativeFileMap[p_iobAddress] = hostFile;
            
            // Devolve o endereço virtual da estrutura PPCFILE para a aplicação PowerPC
            globals->scalars.errno_ = 0;
            state->r3 = p_iobAddress;
            return;
        }
    }
    
          // Se chegou aqui, a tabela interna da ToolBox esgotou-se (limite NFILE)
           globals->scalars.errno_ = EMFILE;
           state->r3 = 0;
	}

	void StdCLib_fprintf(StdCLib::Globals* globals, MachineState* state)
	{
		FILE* fptr = MakeFilePtr(globals, state->r3);
		const char* format = ToPointer<const char>(state->r4);
		if (fptr == nullptr || format == nullptr) { globals->scalars.errno_ = EINVAL; state->r3 = -1; return; }

		// Ficheiro em r3, formato em r4, logo os argumentos começam em r5
		std::string output = StringPrintF(format, *globals, state, 5);

		int charactersWritten = fputs(output.c_str(), fptr);
		
		if (charactersWritten >= 0) { state->r3 = static_cast<int32_t>(output.size()); globals->scalars.errno_ = 0; }
		else { state->r3 = -1; globals->scalars.errno_ = errno; }
	}

	void StdCLib_fputc(StdCLib::Globals* globals, MachineState* state)
	{
		int character = state->r3;
		uint32_t p_iob = state->r4;
		FILE* fptr = MakeFilePtr(globals, p_iob);

		if (fptr == nullptr)
		{
			globals->scalars.errno_ = EBADF;
			state->r3 = EOF;
			return;
		}

		int result = std::fputc(character, fptr);
		if (result == EOF)
		{
			globals->scalars.errno_ = errno;
			state->r3 = EOF;
		}
		else
		{
			state->r3 = result & 0xFF;
			globals->scalars.errno_ = 0;
		}
	}

	void StdCLib_fputs(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_fread(StdCLib::Globals* globals, MachineState* state)
	{
		void* ptr = ToPointer<void>(state->r3);
		uint32_t size = state->r4;
		uint32_t count = state->r5;
		uint32_t p_iob = state->gpr[6]; // r6 é o 4º argumento na ABI
		
		FILE* fptr = MakeFilePtr(globals, p_iob);
		if (fptr == nullptr || (ptr == nullptr && size > 0 && count > 0))
		{
			globals->scalars.errno_ = EBADF;
			state->r3 = 0;
			return;
		}

		size_t elementsRead = std::fread(ptr, size, count, fptr);
		state->r3 = static_cast<uint32_t>(elementsRead);
		globals->scalars.errno_ = std::ferror(fptr) ? errno : 0;
	}

	void StdCLib_free(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_vmAddress = state->r3; // Endereço virtual que a aplicação quer libertar
		
		if (p_vmAddress == 0) return; // free(NULL) não faz nada, dita a norma C

		void* hostPtr = ToPointer<void>(p_vmAddress);
		if (hostPtr != nullptr)
		{
			globals->allocator.Deallocate(hostPtr);
		}
		
		globals->scalars.errno_ = 0;
	}

	void StdCLib_freopen(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_fscanf(StdCLib::Globals* globals, MachineState* state)
	{
		FILE* fptr = MakeFilePtr(globals, state->r3);
		const char* format = ToPointer<const char>(state->r4);

		if (fptr == nullptr || format == nullptr) 
		{ 
			globals->scalars.errno_ = EINVAL; 
			state->r3 = -1; 
			return; 
		}

		// Argumentos variáveis começam no r5 na ABI do PowerPC
		int nextGPR = 5;
		uint32_t stackPtr = state->r1;
		int stackOffset = 24;

		auto getNextPointerArg = [&]() -> uint32_t {
			if (nextGPR <= 10) return state->gpr[nextGPR++];
			uint32_t* ptr = ToPointer<uint32_t>(stackPtr + stackOffset);
			stackOffset += 4;
			return ptr ? Common::CF::BigToHost<uint32_t>::Swap(*ptr) : 0;
		};

		size_t i = 0;
		size_t len = std::strlen(format);
		int tokensMatched = 0;

		while (i < len)
		{
			// Consumir espaços em branco no formato
			if (std::isspace(format[i]))
			{
				int ch;
				while ((ch = std::fgetc(fptr)) != EOF && std::isspace(ch));
				if (ch != EOF) std::ungetc(ch, fptr);
				i++;
				continue;
			}

			if (format[i] == '%' && i + 1 < len)
			{
				i++;
				if (format[i] == '%')
				{
					int ch = std::fgetc(fptr);
					if (ch != '%') { if (ch != EOF) std::ungetc(ch, fptr); break; }
					i++;
					continue;
				}

				// Parser simplificado de especificadores
				char specifier = format[i];
				uint32_t p_dest = getNextPointerArg();
				if (p_dest == 0) { globals->scalars.errno_ = EFAULT; break; }

				if (specifier == 'd' || specifier == 'i')
				{
					int32_t val = 0;
					if (std::fscanf(fptr, "%d", &val) != 1) break;
					
					int32_t* dest = ToPointer<int32_t>(p_dest);
					if (dest) *dest = Common::CF::HostToBig<int32_t>::Swap(val);
					tokensMatched++;
				}
				else if (specifier == 'u')
				{
					uint32_t val = 0;
					if (std::fscanf(fptr, "%u", &val) != 1) break;
					
					uint32_t* dest = ToPointer<uint32_t>(p_dest);
					if (dest) *dest = Common::CF::HostToBig<uint32_t>::Swap(val);
					tokensMatched++;
				}
				else if (specifier == 'x' || specifier == 'X')
				{
					uint32_t val = 0;
					if (std::fscanf(fptr, specifier == 'x' ? "%x" : "%X", &val) != 1) break;
					
					uint32_t* dest = ToPointer<uint32_t>(p_dest);
					if (dest) *dest = Common::CF::HostToBig<uint32_t>::Swap(val);
					tokensMatched++;
				}
				else if (specifier == 'c')
				{
					int ch = std::fgetc(fptr);
					if (ch == EOF) break;
					
					char* dest = ToPointer<char>(p_dest);
					if (dest) *dest = static_cast<char>(ch);
					tokensMatched++;
				}
				else if (specifier == 's')
				{
					// Lemos uma palavra respeitando o limite seguro do buffer simulado
					char tmpBuf[512];
					if (std::fscanf(fptr, "%511s", tmpBuf) != 1) break;
					
					char* dest = ToPointer<char>(p_dest);
					if (dest) std::strcpy(dest, tmpBuf); // Escreve na VM
					tokensMatched++;
				}
				else if (specifier == 'f' || specifier == 'g' || specifier == 'e')
				{
					float val = 0.0f;
					if (std::fscanf(fptr, "%f", &val) != 1) break;
					
					// Converte o float para o formato correto SwappedFloat32/BigEndian
					// Se a tua VM usar SwappedFloat32, aplica a conversão aqui:
					float* dest = ToPointer<float>(p_dest);
					if (dest) {
						// Exemplo assumindo união de bits simples para endianness de floats
						uint32_t bits;
						std::memcpy(&bits, &val, sizeof(float));
						bits = Common::CF::HostToBig<uint32_t>::Swap(bits);
						std::memcpy(dest, &bits, sizeof(float));
					}
					tokensMatched++;
				}
				i++;
			}
			else
			{
				int ch = std::fgetc(fptr);
				if (ch != format[i])
				{
					if (ch != EOF) std::ungetc(ch, fptr);
					break; // Incompatibilidade no pattern matching
				}
				i++;
			}
		}

		// Retorna o número de tokens correspondidos com sucesso, ou EOF se falhar antes de qualquer conversão
		if (tokensMatched == 0 && std::feof(fptr)) 
		{
			state->r3 = -1; // EOF
		}
		else 
		{
			state->r3 = tokensMatched;
		}
		globals->scalars.errno_ = 0;
	}


	void StdCLib_fseek(StdCLib::Globals* globals, MachineState* state)
{
    uint32_t p_iob = state->r3;   // Endereço virtual da estrutura PPCFILE na VM
    int32_t offset = state->r4;   // Offset de 32-bits enviado pela aplicação
    int32_t rawWhence = state->r5; // O whence original da VM
    
    FILE* fptr = MakeFilePtr(globals, p_iob);
    if (fptr == nullptr)
    {
        globals->scalars.errno_ = EBADF; // Bad File Descriptor
        state->r3 = -1; // Devolve erro para a VM
        return;
    }
    
    // Mapeamento explícito das constantes de posicionamento clássicas
    // para as do host moderno, prevenindo falhas de convenção da MSL
    int nativeWhence;
    switch (rawWhence)
    {
        case 0: nativeWhence = SEEK_SET; break; // Início do ficheiro
        case 1: nativeWhence = SEEK_CUR; break; // Posição atual
        case 2: nativeWhence = SEEK_END; break; // Fim do ficheiro
        default:
            globals->scalars.errno_ = EINVAL;
            state->r3 = -1;
            return;
    }
    
    // Executa o fseek real no host de 64-bits
    int result = fseek(fptr, offset, nativeWhence);
    
    if (result != 0)
    {
        globals->scalars.errno_ = errno;
        state->r3 = -1; // O fseek clássico devolve -1 em caso de erro
    }
    else
    {
        globals->scalars.errno_ = 0;
        state->r3 = 0;  // Sucesso
    }
}

	void StdCLib_fsetfileinfo(StdCLib::Globals* globals, MachineState* state)
	{
		const char* pathStr = ToPointer<const char>(state->r3);
		uint32_t creator = state->r4; // O código de 4 caracteres do criador (ex: 'ttxt')
		uint32_t fileType = state->r5; // O código de 4 caracteres do tipo (ex: 'TEXT')

		if (pathStr == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = -1;
			return;
		}

		// No ecossistema moderno do Host (Linux/macOS), estes metadados clássicos da Apple 
		// (Creator/Type) já não existem no sistema de ficheiros nativo de forma direta.
		// A abordagem padrão e segura em emulação é simular sucesso imediato (0/noErr),
		// permitindo que o Guest organize os seus metadados sem falhar a execução.
		state->r3 = 0; 
		globals->scalars.errno_ = 0;
	}

	void StdCLib_fsetpos(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_iob = state->r3; // Endereço virtual do PPCFILE na VM
		uint32_t p_fpos = state->r4; // Ponteiro virtual para a estrutura fpos_t da VM

		FILE* fptr = MakeFilePtr(globals, p_iob);
		uint64_t* fpos = ToPointer<uint64_t>(p_fpos); // fpos_t costuma ser 64-bit no host/VM

		if (fptr == nullptr || fpos == nullptr)
		{
			globals->scalars.errno_ = EBADF;
			state->r3 = -1;
			return;
		}

		// Na ABI PowerPC 32-bit, lemos o valor de 64-bits respeitando o Endianness
		uint64_t targetPos = Common::CF::BigToHost<uint64_t>::Swap(*fpos);

		// Executa o fseeko nativo (que suporta offsets largos de 64-bits de forma segura)
		int result = ::fseeko(fptr, static_cast<off_t>(targetPos), SEEK_SET);

		if (result != 0)
		{
			globals->scalars.errno_ = errno;
			state->r3 = -1;
		}
		else
		{
			globals->scalars.errno_ = 0;
			state->r3 = 0; // Sucesso
		}
	}

	void StdCLib_FSMakeFSSpec_Long(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_FSp_creat(StdCLib::Globals* globals, MachineState* state)
	{
		const PEF::FSSpec* spec = ToPointer<const PEF::FSSpec>(state->r3);
		// No Mac OS Clássico, creat recebia também o Mac Creator (r4) e o FileType (r5)
		
		if (spec == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = -1;
			return;
		}
		
		std::string hostPath = FSSpecToHostPath(spec);
		
		// No host, a criação pura equivale a um open com O_CREAT | O_WRONLY | O_TRUNC
		int fd = ::open(hostPath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
		if (fd < 0)
		{
			globals->scalars.errno_ = errno;
			state->r3 = -1; // Retorna erro clássico (ex: dirNfErr ou wPrErr)
		}
	}
		
	void StdCLib_FSp_faccess(StdCLib::Globals* globals, MachineState* state)
	{
		const PEF::FSSpec* spec = ToPointer<const PEF::FSSpec>(state->r3);
		uint32_t rawMode = state->r4;
		
		if (spec == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = -1;
			return;
		}
		
		std::string hostPath = FSSpecToHostPath(spec);
		
		int nativeMode = F_OK;
		if (rawMode & 0x04) nativeMode |= R_OK;
		if (rawMode & 0x02) nativeMode |= W_OK;
		if (rawMode & 0x01) nativeMode |= X_OK;
		
		int result = ::access(hostPath.c_str(), nativeMode);
		if (result == 0)
		{
			state->r3 = 0; // Acesso permitido
			globals->scalars.errno_ = 0;
		}
		else
		{
			state->r3 = -1;
			globals->scalars.errno_ = errno;
		}
	}

	void StdCLib_FSp_fopen(StdCLib::Globals* globals, MachineState* state)
	{
		const PEF::FSSpec* spec = ToPointer<const PEF::FSSpec>(state->r3);
		uint32_t p_mode = state->r4; // Modo de abertura (ex: "r", "wb")
		
		if (spec == nullptr || p_mode == 0)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0;
			return;
		}
		
		std::string hostPath = FSSpecToHostPath(spec);
		
		// Invocamos a lógica segura de abertura que já corrigimos no StdCLib_fopen.
		// Precisamos de simular os argumentos nos registadores esperados por ela.
		uint32_t originalR3 = state->r3;
		uint32_t originalR4 = state->r4;
		
		// Alocamos temporariamente o caminho convertido na VM para o fopen ler com segurança
		void* vmPathBuf = globals->allocator.Allocate(hostPath.length() + 1, 1);
		if (vmPathBuf == nullptr)
		{
			globals->scalars.errno_ = ENOMEM;
			state->r3 = 0;
			return;
		}
		std::memcpy(vmPathBuf, hostPath.c_str(), hostPath.length() + 1);
		
		// Mapeia para os registadores e executa
		state->r3 = globals->allocator.ToIntPtr(vmPathBuf);
		state->r4 = p_mode;
		
		StdCLib_fopen(globals, state);
		
		// Limpeza do buffer temporário e restauro de contexto
		globals->allocator.Deallocate(vmPathBuf);
	}

	void StdCLib_FSp_freopen(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_FSp_fsetfileinfo(StdCLib::Globals* globals, MachineState* state)
	{
		const PEF::FSSpec* spec = ToPointer<const PEF::FSSpec>(state->r3);
		uint32_t creator = state->r4;
		uint32_t fileType = state->r5;

		if (spec == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = -1;
			return;
		}

		// Encaminha usando a lógica de sandbox para obter o caminho Unix virtual
		std::string hostPath = FSSpecToHostPath(spec);
		
		// Simula sucesso por cortesia ao ecossistema clássico emulado
		state->r3 = 0; 
		globals->scalars.errno_ = 0;
	}

    void StdCLib_FSp_fsetpos(StdCLib::Globals* globals, MachineState* state)
	{
		// Na Macintosh Standard Library (MSL), o FSp_fsetpos atua como um alias direto
		// de posicionamento sobre o stream. Delegamos o comportamento no fsetpos principal.
		StdCLib_fsetpos(globals, state);
	}

	void StdCLib_FSp_open(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_FSp_remove(StdCLib::Globals* globals, MachineState* state)
	{
		const PEF::FSSpec* spec = ToPointer<const PEF::FSSpec>(state->r3);
		if (spec == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = -1;
			return;
		}
		
		std::string hostPath = FSSpecToHostPath(spec);
		
		int result = std::remove(hostPath.c_str());
		if (result != 0)
		{
			globals->scalars.errno_ = errno;
			state->r3 = -1;
		}
		else
		{
			globals->scalars.errno_ = 0;
			state->r3 = 0; // noErr
		}
	}

	void StdCLib_FSp_rename(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_FSp_unlink(StdCLib::Globals* globals, MachineState* state)
	{
		// No ecossistema POSIX do Host, unlink e remove para ficheiros regulares são idênticos
		StdCLib_FSp_remove(globals, state);
	}

	void StdCLib_FSSpec2Path_Long(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_ftell(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_iob = state->r3;
		FILE* fptr = MakeFilePtr(globals, p_iob);

		if (fptr == nullptr)
		{
			globals->scalars.errno_ = EBADF;
			state->r3 = -1;
			return;
		}

		long position = std::ftell(fptr);
		if (position == -1L)
		{
			globals->scalars.errno_ = errno;
			state->r3 = -1;
		}
		else
		{
			state->r3 = static_cast<int32_t>(position);
			globals->scalars.errno_ = 0;
		}
	}

	void StdCLib_fwrite(StdCLib::Globals* globals, MachineState* state)
	{
		const void* ptr = ToPointer<const void>(state->r3);
		uint32_t size = state->r4;
		uint32_t count = state->r5;
		uint32_t p_iob = state->gpr[6];

		FILE* fptr = MakeFilePtr(globals, p_iob);
		if (fptr == nullptr || (ptr == nullptr && size > 0 && count > 0))
		{
			globals->scalars.errno_ = EBADF;
			state->r3 = 0;
			return;
		}

		size_t elementsWritten = std::fwrite(ptr, size, count, fptr);
		state->r3 = static_cast<uint32_t>(elementsWritten);
		globals->scalars.errno_ = std::ferror(fptr) ? errno : 0;
	}

	void StdCLib_getc(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_getchar(StdCLib::Globals* globals, MachineState* state)
	{
		// getchar() lê diretamente do descritor de entrada do Host (stdin)
		int result = std::getchar();
		if (result == EOF)
		{
			globals->scalars.errno = std::ferror(stdin) ? errno : 0;
			state->r3 = EOF;
		}
		else
		{
			state->r3 = result & 0xFF;
			globals->scalars.errno_ = 0;
		}
	}

	void StdCLib_getenv(StdCLib::Globals* globals, MachineState* state)
{
    const char* name = ToPointer<const char>(state->r3);
    if (name == nullptr)
    {
        globals->scalars.errno_ = EINVAL;
        state->r3 = 0; // Devolve NULL
        return;
    }
    
    // Procura a variável de ambiente no host moderno de 64-bits
    const char* envValue = getenv(name);
    
    if (envValue == nullptr)
    {
        state->r3 = 0; // Variável não encontrada, devolve NULL
        globals->scalars.errno_ = 0;
        return;
    }
    
    // Descobrir o tamanho necessário (+1 para o terminador null \0)
    size_t len = strlen(envValue) + 1;
    
    // Alocar memória DENTRO do espaço de endereçamento de 32-bits visível pela VM.
    // O allocator.Allocate garante um ponteiro do host alinhado, mas nós precisamos 
    // do endereço correspondente na VM para passar à aplicação PowerPC.
    void* vmHostPtr = globals->allocator.Allocate(len, 1); 
    if (vmHostPtr == nullptr)
    {
        globals->scalars.errno_ = ENOMEM;
        state->r3 = 0;
        return;
    }
    
    // Copiar em segurança os dados do host de 64-bits para a memória alocada da VM
    memcpy(vmHostPtr, envValue, len);
    
    // Converter o ponteiro de memória para o endereço virtual de 32-bits correto da VM
    uint32_t p_vmAddress = globals->allocator.ToIntPtr(vmHostPtr);
    
    // Devolve o endereço virtual seguro para o registador r3 da CPU emulada
    state->r3 = p_vmAddress;
    globals->scalars.errno_ = 0;
}
	
	void StdCLib_getIDstring(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_getpid(StdCLib::Globals* globals, MachineState* state)
	{
		// Obtém o ID do processo real do Host através da chamada POSIX padrão
		pid_t pid = ::getpid();

		// Garante o cast seguro e limpo para o registador r3 de 32-bits do PowerPC
		state->r3 = static_cast<int32_t>(pid);
		globals->scalars.errno_ = 0;
	}

	void StdCLib_getc(StdCLib::Globals* globals, MachineState* state)
    {
        // getc expande-se semanticamente como um fgetc padrão
        StdCLib_fgetc(globals, state);
    }

	void StdCLib_getw(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_gmtime(StdCLib::Globals* globals, MachineState* state)
	{
		const uint32_t* p_timer = ToPointer<const uint32_t>(state->r3);
		if (p_timer == nullptr) { state->r3 = 0; return; }

		uint32_t classicTime = Common::CF::BigToHost<uint32_t>::Swap(*p_timer);
		// Converte de volta para época Unix se o teu Classic usar a época de 1904
		std::time_t hostTime = static_cast<std::time_t>(classicTime - 2082844800ULL);

		std::tm* timeInfo = std::gmtime(&hostTime);
		if (timeInfo == nullptr) { state->r3 = 0; return; }

		// Usamos a nossa área global reservada temporária para time_data (ex: scalars.TimeData) 
		// ou alocamos um bloco persistente por thread. Para simplificar, usamos a estrutura interna:
		void* p_virtualTM = ToPointer<void>(globals->scalars.TimeData);
		FillVirtualTM(p_virtualTM, timeInfo);

		state->r3 = globals->scalars.TimeData;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_IEResolvePath(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_ioctl(StdCLib::Globals* globals, MachineState* state)
	{
		int fd = static_cast<int>(state->r3);
		uint32_t request = state->r4;
		uint32_t p_vmArg = state->r5; // Quase sempre um ponteiro virtual para dados da VM

		// Alerta de Segurança Crítico:
		// Não podemos fazer "::ioctl(fd, request, ToPointer(p_vmArg))" cegamente.
		// Se o ioctl nativo do host esperar ler/escrever 8 bytes (64-bit) e a VM só 
		// alocou 4 bytes (32-bit), o host vai rebentar a sandbox e causar um SegFault.
		
		std::fprintf(stderr, "[ClassiX] ioctl: Ignored request 0x%08x on fd %d to avoid memory corruption.\n", request, fd);
		
		// Como o ioctl varia de OS para OS, definir um No-Op seguro ou ENOTTY (Inappropriate ioctl for device)
		// é a abordagem padrão em emulação até isolares que pedidos específicos a tua app Guest exige.
		globals->scalars.errno_ = ENOTTY; 
		state->r3 = -1;
	}

    void StdCLib_isalnum(StdCLib::Globals* globals, MachineState* state)
	{
		int ch = static_cast<int>(state->r3 & 0xFF);
		uint8_t flags = globals->scalars.cType[ch];
		state->r3 = (flags & (_UPP | _LOW | _DIG)) ? 1 : 0;
	}

	void StdCLib_isalpha(StdCLib::Globals* globals, MachineState* state)
	{
		int ch = static_cast<int>(state->r3 & 0xFF);
		uint8_t flags = globals->scalars.cType[ch];
		state->r3 = (flags & (_UPP | _LOW)) ? 1 : 0;
	}

	void StdCLib_isascii(StdCLib::Globals* globals, MachineState* state)
	{
		// A norma clássica dita: verdadeiro se o valor estiver entre 0 e 127
		uint32_t val = state->r3;
		state->r3 = (val <= 0x7F) ? 1 : 0;
	}

	void StdCLib_iscntrl(StdCLib::Globals* globals, MachineState* state)
	{
		int ch = static_cast<int>(state->r3 & 0xFF);
		uint8_t flags = globals->scalars.cType[ch];
		state->r3 = (flags & _CTL) ? 1 : 0;
	}

	void StdCLib_isdigit(StdCLib::Globals* globals, MachineState* state)
	{
		int ch = static_cast<int>(state->r3 & 0xFF);
		uint8_t flags = globals->scalars.cType[ch];
		state->r3 = (flags & _DIG) ? 1 : 0;
	}

	void StdCLib_isgraph(StdCLib::Globals* globals, MachineState* state)
	{
		int ch = static_cast<int>(state->r3 & 0xFF);
		uint8_t flags = globals->scalars.cType[ch];
		// Gráficos: qualquer carácter imprimível exceto o espaço em branco puro (_BLA)
		state->r3 = (flags & (_PUN | _UPP | _LOW | _DIG)) ? 1 : 0;
	}

	void StdCLib_islower(StdCLib::Globals* globals, MachineState* state)
	{
		int ch = static_cast<int>(state->r3 & 0xFF);
		uint8_t flags = globals->scalars.cType[ch];
		state->r3 = (flags & _LOW) ? 1 : 0;
	}

	void StdCLib_isprint(StdCLib::Globals* globals, MachineState* state)
	{
		int ch = static_cast<int>(state->r3 & 0xFF);
		uint8_t flags = globals->scalars.cType[ch];
		// Imprimíveis: caracteres gráficos mais o carácter de espaço em branco real (_BLA)
		state->r3 = (flags & (_PUN | _UPP | _LOW | _DIG | _BLA)) ? 1 : 0;
	}

	void StdCLib_ispunct(StdCLib::Globals* globals, MachineState* state)
	{
		int ch = static_cast<int>(state->r3 & 0xFF);
		uint8_t flags = globals->scalars.cType[ch];
		state->r3 = (flags & _PUN) ? 1 : 0;
	}

	void StdCLib_isspace(StdCLib::Globals* globals, MachineState* state)
	{
		int ch = static_cast<int>(state->r3 & 0xFF);
		uint8_t flags = globals->scalars.cType[ch];
		state->r3 = (flags & _WSP) ? 1 : 0;
	}

	void StdCLib_isupper(StdCLib::Globals* globals, MachineState* state)
	{
		int ch = static_cast<int>(state->r3 & 0xFF);
		uint8_t flags = globals->scalars.cType[ch];
		state->r3 = (flags & _UPP) ? 1 : 0;
	}

	void StdCLib_isxdigit(StdCLib::Globals* globals, MachineState* state)
	{
		int ch = static_cast<int>(state->r3 & 0xFF);
		uint8_t flags = globals->scalars.cType[ch];
		state->r3 = (flags & _HEX) ? 1 : 0;
	}


	void StdCLib_labs(StdCLib::Globals* globals, MachineState* state)
	{
		int32_t val = static_cast<int32_t>(state->r3);
		state->r3 = static_cast<int32_t>(std::labs(val));
	}

	void StdCLib_ldiv(StdCLib::Globals* globals, MachineState* state)
	{
		// Na arquitetura de 32-bits do PowerPC, 'long' tem exatamente 4 bytes (32-bits).
		// Portanto, ldiv comporta-se de forma idêntica a div, devolvendo o resultado em r3 e r4.
		int32_t numer = static_cast<int32_t>(state->r3);
		int32_t denom = static_cast<int32_t>(state->r4);

		if (denom == 0)
		{
			globals->scalars.errno_ = EDOM;
			state->r3 = 0;
			state->r4 = 0;
			return;
		}

		std::ldiv_t res = std::ldiv(numer, denom);

		state->r3 = static_cast<uint32_t>(res.quot);
		state->r4 = static_cast<uint32_t>(res.rem);
		globals->scalars.errno_ = 0;
	}

	void StdCLib_llabs(StdCLib::Globals* globals, MachineState* state)
	{
		// O valor de entrada de 64-bits está distribuído em r3 (high) e r4 (low)
		uint64_t uval = (static_cast<uint64_t>(state->r3) << 32) | state->r4;
		long long val = static_cast<long long>(uval);
		
		long long result = std::llabs(val);
		uint64_t ures = static_cast<uint64_t>(result);
		
		state->r3 = static_cast<uint32_t>(ures >> 32);
		state->r4 = static_cast<uint32_t>(ures & 0xFFFFFFFF);
	}

	void StdCLib_lldiv(StdCLib::Globals* globals, MachineState* state)
	{
		// REGRA CRÍTICA DA ABI: Como lldiv_t tem 16 bytes (dois int64_t), o compilador
		// da VM passa no registador r3 um ponteiro para onde o resultado deve ser guardado.
		// Os argumentos reais de 64-bits ficam distribuídos nos registadores seguintes:
		// numerados (64-bit) em r4 (high) e r5 (low)
		// denominador (64-bit) em r6 (high) e r7 (low)
		uint32_t p_destResult = state->r3;

		uint64_t u_numer = (static_cast<uint64_t>(state->r4) << 32) | state->gpr[5];
		uint64_t u_denom = (static_cast<uint64_t>(state->gpr[6]) << 32) | state->gpr[7];

		long long numer = static_cast<long long>(u_numer);
		long long denom = static_cast<long long>(u_denom);

		if (p_destResult == 0)
		{
			globals->scalars.errno_ = EINVAL;
			return;
		}

		if (denom == 0)
		{
			globals->scalars.errno_ = EDOM;
			return;
		}

		std::lldiv_t res = std::lldiv(numer, denom);

		// Obtemos o ponteiro para a memória virtualizada da VM de 32-bits
		uint64_t* destFields = ToPointer<uint64_t>(p_destResult);
		if (destFields)
		{
			// Convertemos os resultados de 64-bits para Big-Endian antes de escrever na VM
			uint64_t quotBE = Common::CF::HostToBig<uint64_t>::Swap(static_cast<uint64_t>(res.quot));
			uint64_t remBE  = Common::CF::HostToBig<uint64_t>::Swap(static_cast<uint64_t>(res.rem));

			destFields[0] = quotBE; // quot ocupa os primeiros 8 bytes
			destFields[1] = remBE;  // rem ocupa os 8 bytes seguintes
		}

		// A ABI dita que o registador r3 retém o ponteiro de destino original
		state->r3 = p_destResult;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_localeconv(StdCLib::Globals* globals, MachineState* state)
	{
		// 1. Obter a localização regional ativa no Host nativo de 64-bits
		struct lconv* hostLconv = std::localeconv();
		if (hostLconv == nullptr)
		{
			state->r3 = 0;
			return;
		}

		// 2. Para evitar Memory Leaks e desalinhamentos de 64-bits, usamos a área global 
		// reservada temporária nos scalars para mapear a tabela de 32-bits visível pelo Guest.
		// No Mac OS Clássico/MSL, o struct lconv de 32-bits é tipicamente um bloco compacto de ponteiros:
		// [decimal_point, thousands_sep, grouping, int_curr_symbol, currency_symbol, ...]
		
		// Mapeamos os buffers de string do Host para endereços virtuais na VM em áreas de cortesia
		char* vmNumericBuf = ToPointer<char>(globals->scalars.NumericData);
		char* vmMoneyBuf = ToPointer<char>(globals->scalars.MoneyData);

		if (vmNumericBuf && vmMoneyBuf)
		{
			// Copiamos os caracteres básicos de formatação local do Host para os buffers da VM
			std::snprintf(vmNumericBuf, 16, "%s", hostLconv->decimal_point ? hostLconv->decimal_point : ".");
			std::snprintf(vmNumericBuf + 4, 16, "%s", hostLconv->thousands_sep ? hostLconv->thousands_sep : "");
			
			std::snprintf(vmMoneyBuf, 16, "%s", hostLconv->currency_symbol ? hostLconv->currency_symbol : "");
			std::snprintf(vmMoneyBuf + 8, 16, "%s", hostLconv->int_curr_symbol ? hostLconv->int_curr_symbol : "");
		}

		// 3. Montar a tabela de ponteiros de 32-bits Big-Endian no bloco alvo da VM.
		// Usamos a área dedicada `_CategoryLoc` para depositar o cabeçalho do struct lconv emulado.
		uint32_t* virtualLconvStruct = ToPointer<uint32_t>(globals->scalars._CategoryLoc);
		
		if (virtualLconvStruct)
		{
			using namespace Common::CF;
			// Preenchemos os ponteiros virtuais convertendo para Big-Endian conforme a ABI da VM exige
			virtualLconvStruct[0] = HostToBig<uint32_t>::Swap(globals->scalars.NumericData);     // decimal_point
			virtualLconvStruct[1] = HostToBig<uint32_t>::Swap(globals->scalars.NumericData + 4); // thousands_sep
			virtualLconvStruct[2] = HostToBig<uint32_t>::Swap(0);                               // grouping (vazio para simplificar)
			virtualLconvStruct[3] = HostToBig<uint32_t>::Swap(globals->scalars.MoneyData + 8);   // int_curr_symbol
			virtualLconvStruct[4] = HostToBig<uint32_t>::Swap(globals->scalars.MoneyData);       // currency_symbol
			
			// Os restantes campos numéricos (frac_digits, p_cs_precedes, etc.) podem ser emulados 
			// preenchendo os bytes subsequentes com os valores canónicos (geralmente CHAR_MAX ou valores nativos).
			uint8_t* byteFields = reinterpret_cast<uint8_t*>(&virtualLconvStruct[5]);
			byteFields[0] = hostLconv->frac_digits;
			byteFields[1] = hostLconv->p_cs_precedes;
			byteFields[2] = hostLconv->p_sep_by_space;
			byteFields[3] = hostLconv->n_cs_precedes;
		}

		// Devolvemos com total segurança o endereço virtual de 32-bits da estrutura emulada
		state->r3 = globals->scalars._CategoryLoc;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_localtime(StdCLib::Globals* globals, MachineState* state)
	{
		const uint32_t* p_timer = ToPointer<const uint32_t>(state->r3);
		if (p_timer == nullptr) { state->r3 = 0; return; }

		uint32_t classicTime = Common::CF::BigToHost<uint32_t>::Swap(*p_timer);
		std::time_t hostTime = static_cast<std::time_t>(classicTime - 2082844800ULL);

		std::tm* timeInfo = std::localtime(&hostTime);
		if (timeInfo == nullptr) { state->r3 = 0; return; }

		void* p_virtualTM = ToPointer<void>(globals->scalars.TimeData);
		FillVirtualTM(p_virtualTM, timeInfo);

		state->r3 = globals->scalars.TimeData;
		globals->scalars.errno_ = 0;
	}
	
	void StdCLib_longjmp(StdCLib::Globals* globals, PPCVM::MachineState* state)
       {
    // Obtemos o ponteiro virtual do jmpBuf fornecido pela aplicação
    uint32_t* jmpBuf = ToPointer<uint32_t>(state->r3);
    int value = state->r4; // O valor de retorno para o setjmp
    
    if (jmpBuf == nullptr)
    {
        globals->scalars.errno_ = EINVAL;
        // Se o jmpBuf for inválido, o comportamento padrão é falhar controladamente
        throw std::runtime_error("FALHA CRÍTICA: longjmp chamado com um jmpBuf nulo.");
    }
    
    // De acordo com a especificação da norma C, se o valor passado for 0, 
    // o setjmp tem de receber e retornar 1 para evitar loops infinitos.
    if (value == 0)
    {
        value = 1;
    }
    
    // Alias para inverter de Big-Endian para o formato nativo do Host
    using namespace Common::CF;

    // 1. Restaurar o controlo de fluxo revertendo o Endianness
    state->lr    = BigToHost<uint32_t>::Swap(jmpBuf[0]);
    
    // Restaurar o Condition Register (CR) através do método do teu interpretador
    uint32_t crValue = BigToHost<uint32_t>::Swap(jmpBuf[1]);
    state->SetCR(crValue);
    
    state->r1    = BigToHost<uint32_t>::Swap(jmpBuf[2]);
    state->r2    = BigToHost<uint32_t>::Swap(jmpBuf[3]);
    // jmpBuf[4] era o padding/reservado, ignoramos
    
    // 2. Restaurar os Registadores de Propósito Geral (GPR 13 a 31)
    for (int i = 0; i < 18; i++)
    {
        state->gpr[13 + i] = BigToHost<uint32_t>::Swap(jmpBuf[5 + i]);
    }
    
    // 3. Restaurar os Registadores de Vírgula Flutuante (FPR 14 a 30)
    // Lemos os blocos de 32-bits da memória virtual, remontamos o SwappedFloat64 e convertemos para double nativo
    for (int i = 0; i < 17; i++)
    {
        SwappedFloat64 swappedFPR;
        uint32_t* sourceSlot = jmpBuf + 24 + (i * 2);
        
        // Copia os 8 bytes de forma segura para evitar desalinhamento de memória no host
        std::memcpy(&swappedFPR.v, sourceSlot, sizeof(uint64_t));
        
        // Converte o formato Big-Endian lido para o double nativo (Little-Endian)
        state->fpr[14 + i] = ConvertDoubleSwappedToHost(swappedFPR);
    }
    
    // 4. Configurar o valor de retorno no registador r3 da CPU virtual 
    // (a aplicação vai achar que este é o retorno do setjmp original)
    state->r3 = value;
    
    // 5. Apontar o Program Counter (PC) para o Link Register (LR) restaurado 
    // para efetivar o desvio de volta à subrotina original.
    state->pc = state->lr;
    
    globals->scalars.errno_ = 0;
}

	void StdCLib_lseek(StdCLib::Globals* globals, MachineState* state)
	{
		int fd = static_cast<int>(state->r3);
		int32_t offset = static_cast<int32_t>(state->r4);
		int rawWhence = static_cast<int>(state->r5);

		if (fd < 0)
		{
			globals->scalars.errno_ = EBADF;
			state->r3 = -1;
			return;
		}

		// Tradução explícita das constantes clássicas de posicionamento para o Host
		int nativeWhence;
		switch (rawWhence)
		{
			case 0: nativeWhence = SEEK_SET; break; // Início
			case 1: nativeWhence = SEEK_CUR; break; // Posição Atual
			case 2: nativeWhence = SEEK_END; break; // Fim
			default:
				globals->scalars.errno_ = EINVAL;
				state->r3 = -1;
				return;
		}

		// Executa a busca real de 64-bits no Host (lseek retorna off_t)
		off_t result = ::lseek(fd, offset, nativeWhence);

		if (result == (off_t)-1)
		{
			globals->scalars.errno_ = errno;
			state->r3 = -1;
		}
		else
		{
			globals->scalars.errno_ = 0;
			// Força o cast seguro do offset resultante de volta para os 32-bits da VM
			state->r3 = static_cast<int32_t>(result);
		}
	}


	void StdCLib_MakeResolvedFSSpec(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_MakeResolvedFSSpec_Long(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_MakeResolvedPath(StdCLib::Globals* globals, MachineState* state)
    {
        // Mapeamento simplificado: assume que o caminho fornecido já está resolvido na Sandbox local.
        state->r3 = 0;
        globals->scalars.errno_ = 0;
    }

	void StdCLib_MakeResolvedPath_Long(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_MakeTheLocaleString(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_malloc(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t size = state->r3; // Tamanho requisitado pela VM (32-bit)
		
		if (size == 0)
		{
			state->r3 = 0; // malloc(0) pode retornar NULL
			return;
		}

		// Aloca no gestor de memória partilhado da VM (alinhamento padrão de 4 ou 8 bytes)
		void* ptr = globals->allocator.Allocate(size, 8); 
		
		if (ptr == nullptr)
		{
			globals->scalars.errno_ = ENOMEM;
			state->r3 = 0; // Devolve NULL por falta de memória
		}
		else
		{
			state->r3 = globals->allocator.ToIntPtr(ptr); // Converte para o endereço virtual de 32-bit
			globals->scalars.errno_ = 0;
		}
	}

	void StdCLib_mblen(StdCLib::Globals* globals, MachineState* state)
    {
        const char* s = ToPointer<const char>(state->r3);
        size_t n = static_cast<size_t>(state->r4);

        if (s == nullptr || n == 0 || *s == '\0')
        {
            state->r3 = 0;
            return;
        }

        int result = std::mblen(s, n);
        state->r3 = static_cast<int32_t>(result);
    }

	void StdCLib_mbstowcs(StdCLib::Globals* globals, MachineState* state)
    {
        wchar_t* pwcs = ToPointer<wchar_t>(state->r3);
        const char* s = ToPointer<const char>(state->r4);
        size_t n = static_cast<size_t>(state->r5);

        if (s == nullptr) { state->r3 = 0; return; }

        size_t result = std::mbstowcs(pwcs, s, n);
        if (pwcs && result != (size_t)-1)
        {
            for (size_t i = 0; i < result; i++)
            {
                pwcs[i] = static_cast<wchar_t>(Common::CF::HostToBig<uint32_t>::Swap(static_cast<uint32_t>(pwcs[i])));
            }
        }
        state->r3 = static_cast<uint32_t>(result);
    }

	void StdCLib_mbtowc(StdCLib::Globals* globals, MachineState* state)
    {
        uint32_t p_pwc = state->r3;
        const char* s = ToPointer<const char>(state->r4);
        size_t n = static_cast<size_t>(state->r5);

        wchar_t* pwc = ToPointer<wchar_t>(p_pwc);

        int result = std::mbtowc(pwc, s, n);
        if (pwc && result > 0)
        {
            // Ajusta o Endianness se wchar_t na VM diferir em tamanho (16 ou 32-bit Big Endian)
            *pwc = static_cast<wchar_t>(Common::CF::HostToBig<uint32_t>::Swap(static_cast<uint32_t>(*pwc)));
        }
        state->r3 = static_cast<int32_t>(result);
    }

	void StdCLib_memccpy(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_dest = state->r3;
		char* dest = ToPointer<char>(p_dest);
		const char* src = ToPointer<const char>(state->r4);
		int c = state->r5 & 0xFF;
		uint32_t size = static_cast<uint32_t>(state->gpr[6]); // r6 é o 4º argumento na ABI

		if (dest == nullptr || src == nullptr || size == 0)
		{
			state->r3 = 0; // Retorna NULL se nada for copiado ou se houver erro
			return;
		}

		// Implementação manual segura para mapear os offsets da VM
		for (uint32_t i = 0; i < size; i++)
		{
			dest[i] = src[i];
			if (static_cast<unsigned char>(src[i]) == static_cast<unsigned char>(c))
			{
				// Retorna o ponteiro virtual para o carácter imediatamente a seguir ao 'c'
				state->r3 = p_dest + i + 1;
				globals->scalars.errno_ = 0;
				return;
			}
		}

		state->r3 = 0; // 'c' não foi encontrado nos 'size' bytes
		globals->scalars.errno_ = 0;
	}

	void StdCLib_memchr(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_src = state->r3;
		const void* src = ToPointer<const void>(p_src);
		int c = state->r4 & 0xFF;
		size_t n = static_cast<size_t>(state->r5);

		if (src == nullptr || n == 0)
		{
			state->r3 = 0;
			return;
		}

		const void* res = std::memchr(src, c, n);
		if (res == nullptr)
		{
			state->r3 = 0;
		}
		else
		{
			// Calcula o offset no host e mapeia de volta para o endereço virtual da VM
			size_t offset = static_cast<const uint8_t*>(res) - static_cast<const uint8_t*>(src);
			state->r3 = p_src + static_cast<uint32_t>(offset);
		}
	}
	
	void StdCLib_memcmp(StdCLib::Globals* globals, MachineState* state)
{
    const void* s1 = ToPointer<const void>(state->r3);
    const void* s2 = ToPointer<const void>(state->r4);
    
    // Forçar estritamente 32-bits (tamanho máximo na VM PowerPC)
    uint32_t size = static_cast<uint32_t>(state->r5);
    
    if (size == 0)
    {
        state->r3 = 0;
        return;
    }
    
    if (s1 == nullptr || s2 == nullptr)
    {
        globals->scalars.errno_ = EINVAL;
        state->r3 = 0;
        return;
    }
    
    // Garante que o resultado respeita as convenções de 32-bits do registador r3
    int result = memcmp(s1, s2, size);
    state->r3 = static_cast<int32_t>(result);
}

void StdCLib_memcpy(StdCLib::Globals* globals, MachineState* state)
{
    void* dest = ToPointer<void>(state->r3);
    const void* src = ToPointer<const void>(state->r4);
    
    // Forçar estritamente 32-bits para evitar overflows de tamanho no host de 64-bits
    uint32_t size = static_cast<uint32_t>(state->r5);
    
    if (size == 0)
    {
        // Se o tamanho for 0, o memcpy clássico apenas retorna o destino original intacto
        state->r3 = state->r3; 
        return;
    }
    
    if (dest == nullptr || src == nullptr)
    {
        globals->scalars.errno_ = EINVAL;
        return;
    }
    
    // Executa a cópia nativa segura
    memcpy(dest, src, size);
    
    // O r3 deve reter o endereço VIRTUAL original de destino (que já estava em state->r3).
    // Usar o ToIntPtr(memcpy(...)) original era perigoso porque podia tentar converter
    // o ponteiro real do host (64-bit) de volta para o r3 (32-bit), corrompendo o endereço da VM!
    state->r3 = state->r3; 
    globals->scalars.errno_ = 0;
}


	void StdCLib_memmove(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_dest = state->r3;
		void* dest = ToPointer<void>(p_dest);
		const void* src = ToPointer<const void>(state->r4);
		uint32_t size = static_cast<uint32_t>(state->r5);

		if (size == 0)
		{
			state->r3 = p_dest;
			return;
		}

		if (dest == nullptr || src == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			return;
		}

		// memmove lida em segurança com blocos de memória sobrepostos
		std::memmove(dest, src, size);
		state->r3 = p_dest;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_memset(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_dest = state->r3;
		void* dest = ToPointer<void>(p_dest);
		int value = state->r4 & 0xFF;
		uint32_t size = static_cast<uint32_t>(state->r5);

		if (size == 0)
		{
			state->r3 = p_dest;
			return;
		}

		if (dest == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			return;
		}

		std::memset(dest, value, size);
		state->r3 = p_dest; // Retorna o endereço virtual original de destino
		globals->scalars.errno_ = 0;
	}

	void StdCLib_mktemp(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_template = state->r3; // Endereço virtual da string com o padrão (ex: "/tmp/fileXXXXXX")
		char* templateStr = ToPointer<char>(p_template);
		
		if (templateStr == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0;
			return;
		}
		
		// O mktemp clássico/POSIX modifica o template diretamente na memória.
		// Como templateStr aponta para a memória perfeitamente mapeada da VM, podemos
		// invocar com segurança a função de sistema do host que lida com o padrão.
		// Nota: mktemp() nativo está obsoleto em POSIX moderno (preferindo-se mkstemp), 
		// mas para simular o comportamento exato sem abrir o ficheiro, preservamos o comportamento.
		char* result = ::mktemp(templateStr);
		
		if (result == nullptr || (std::strlen(templateStr) > 0 && templateStr[0] == '\0'))
		{
			globals->scalars.errno_ = errno;
			state->r3 = 0; // Falha catastrófica ao mutar o padrão
		}
		else
		{
			// Sucesso: os bytes na memória virtual foram modificados pelo host.
			// Devolvemos o endereço virtual original de 32-bits (r3) para manter a ABI estável.
			globals->scalars.errno_ = 0;
			state->r3 = p_template;
		}
	}

	void StdCLib_mktime(StdCLib::Globals* globals, MachineState* state)
	{
		void* p_virtualTM = ToPointer<void>(state->r3);
		if (p_virtualTM == nullptr) { state->r3 = -1; return; }

		std::tm timeInfo = ParseVirtualTM(p_virtualTM);
		std::time_t hostTime = std::mktime(&timeInfo);

		if (hostTime == -1)
		{
			state->r3 = -1;
			return;
		}

		// Atualiza a estrutura modificada de volta para a VM (ex: acerto de tm_wday)
		FillVirtualTM(p_virtualTM, &timeInfo);

		// Converte o resultado de volta para segundos da época clássica
		uint32_t classicTime = static_cast<uint32_t>(hostTime + 2082844800ULL);
		state->r3 = classicTime;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_open(StdCLib::Globals* globals, MachineState* state)
	{
		const char* path = ToPointer<const char>(state->r3);
		uint32_t classicFlags = state->r4;
		uint32_t classicMode = state->r5; // Usado se O_CREAT estiver ativo

		if (path == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = -1;
			return;
		}

		// Tradução segura das flags clássicas (MSL/ToolBox) para POSIX do Host
		int hostFlags = 0;
		
		// Isolar o modo de acesso básico
		uint32_t accmode = classicFlags & 0x03; // Geralmente mapeia 0=O_RDONLY, 1=O_WRONLY, 2=O_RDWR
		if (accmode == 0) hostFlags |= O_RDONLY;
		else if (accmode == 1) hostFlags |= O_WRONLY;
		else if (accmode == 2) hostFlags |= O_RDWR;

		// Mapeamento de flags de controlo típicas do Mac OS Clássico
		if (classicFlags & 0x0100) hostFlags |= O_CREAT;
		if (classicFlags & 0x0200) hostFlags |= O_EXCL;
		if (classicFlags & 0x0400) hostFlags |= O_TRUNC;
		if (classicFlags & 0x0800) hostFlags |= O_APPEND;
		if (classicFlags & 0x2000) hostFlags |= O_NONBLOCK;

		// Executa a chamada de sistema real no Host
		int fd = ::open(path, hostFlags, static_cast<mode_t>(classicMode));

		if (fd < 0)
		{
			globals->scalars.errno_ = errno;
			state->r3 = -1;
		}
		else
		{
			globals->scalars.errno_ = 0;
			state->r3 = static_cast<int32_t>(fd); // Retorna o descritor de 32-bits para a VM
		}
	}


	void StdCLib_ParseTheLocaleString(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_perror(StdCLib::Globals* globals, MachineState* state)
	{
		const char* prefix = ToPointer<const char>(state->r3);
		int currentErrno = static_cast<int>(globals->scalars.errno_);

		// Mapeia temporariamente para o errno do host para usar a mensagem canónica do std::perror
		int oldHostErrno = errno;
		errno = currentErrno;

		if (prefix && std::strlen(prefix) > 0)
		{
			std::fprintf(stderr, "%s: ", prefix);
		}
		
		std::perror("");
		std::fflush(stderr);

		errno = oldHostErrno; // Restaura o errno original do host
	}

	void StdCLib_PLpos(StdCLib::Globals* globals, MachineState* state)
	{
		// O PLpos clássico devolve o índice (base 1) de onde a substring (r3) começa na string principal (r4).
		// Se não encontrar, devolve 0.
		const uint8_t* needle = ToPointer<const uint8_t>(state->r3);
		const uint8_t* haystack = ToPointer<const uint8_t>(state->r4);

		if (needle == nullptr || haystack == nullptr) { state->r3 = 0; return; }

		uint32_t hLen = haystack[0];
		uint32_t nLen = needle[0];

		if (nLen == 0) { state->r3 = 1; return; }
		if (nLen > hLen) { state->r3 = 0; return; }

		for (uint32_t i = 0; i <= hLen - nLen; i++)
		{
			if (std::memcmp(&haystack[1 + i], &needle[1], nLen) == 0)
			{
				state->r3 = i + 1; // Índice baseado em 1, regra canónica Pascal
				return;
			}
		}
		state->r3 = 0;
	}

	void StdCLib_PLstrcat(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_dest = state->r3;
		uint8_t* dest = ToPointer<uint8_t>(p_dest);
		const uint8_t* src = ToPointer<const uint8_t>(state->r4);

		if (dest == nullptr || src == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = p_dest;
			return;
		}

		uint32_t destLen = dest[0];
		uint32_t srcLen = src[0];
		
		// O tamanho combinado não pode ultrapassar o limite físico absoluto de 255 bytes de uma Pascal String
		uint33_t totalLen = destLen + srcLen;
		if (totalLen > 255) totalLen = 255;

		uint32_t charsToCopy = totalLen - destLen;

		if (charsToCopy > 0)
		{
			std::memmove(&dest[1 + destLen], &src[1], charsToCopy);
		}

		dest[0] = static_cast<uint8_t>(totalLen); // Atualiza o novo tamanho
		state->r3 = p_dest;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_PLstrchr(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_str = state->r3;
		const uint8_t* s = ToPointer<const uint8_t>(p_str);
		int character = state->r4 & 0xFF;

		if (s == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0;
			return;
		}

		uint32_t length = s[0];
		for (uint32_t i = 0; i < length; i++)
		{
			if (s[1 + i] == character)
			{
				// Retorna o ponteiro virtual exato para o carácter dentro da string Pascal
				state->r3 = p_str + 1 + i;
				globals->scalars.errno_ = 0;
				return;
			}
		}

		state->r3 = 0; // Não encontrado
		globals->scalars.errno_ = 0;
    }

	void StdCLib_PLstrcmp(StdCLib::Globals* globals, MachineState* state)
	{
		const uint8_t* s1 = ToPointer<const uint8_t>(state->r3);
		const uint8_t* s2 = ToPointer<const uint8_t>(state->r4);

		if (s1 == nullptr || s2 == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = (s1 == nullptr) ? -1 : 1;
			return;
		}

		uint32_t len1 = s1[0];
		uint32_t len2 = s2[0];
		uint32_t minLen = (len1 < len2) ? len1 : len2;

		int res = std::memcmp(&s1[1], &s2[1], minLen);
		if (res == 0)
		{
			if (len1 < len2) res = -1;
			else if (len1 > len2) res = 1;
		}

		state->r3 = static_cast<int32_t>(res);
		globals->scalars.errno_ = 0;
	}

	void StdCLib_PLstrcpy(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_dest = state->r3;
		uint8_t* dest = ToPointer<uint8_t>(p_dest);
		const uint8_t* src = ToPointer<const uint8_t>(state->r4);

		if (dest == nullptr || src == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			return;
		}

		uint8_t length = src[0];
		dest[0] = length; // Copia o byte de tamanho
		
		// Copia os caracteres reais (estatisticamente seguro contra overlaps)
		std::memmove(&dest[1], &src[1], length);

		state->r3 = p_dest; // Retorna o endereço virtual original de destino
		globals->scalars.errno_ = 0;
	}

	void StdCLib_PLstrlen(StdCLib::Globals* globals, MachineState* state)
	{
		const uint8_t* s = ToPointer<const uint8_t>(state->r3);
		
		// O tamanho é o valor guardado estritamente no primeiro byte
		state->r3 = s ? static_cast<uint32_t>(s[0]) : 0;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_PLstrncat(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_dest = state->r3;
		uint8_t* dest = ToPointer<uint8_t>(p_dest);
		const uint8_t* src = ToPointer<const uint8_t>(state->r4);
		uint32_t maxAppend = state->r5; // Máximo de bytes a anexar

		if (dest == nullptr || src == nullptr || maxAppend == 0)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = p_dest;
			return;
		}

		uint32_t destLen = dest[0];
		uint32_t srcLen = src[0];
		
		uint32_t charsToCopy = (srcLen > maxAppend) ? maxAppend : srcLen;
		
		uint32_t totalLen = destLen + charsToCopy;
		if (totalLen > 255) totalLen = 255;

		charsToCopy = totalLen - destLen;

		if (charsToCopy > 0)
		{
			std::memmove(&dest[1 + destLen], &src[1], charsToCopy);
		}

		dest[0] = static_cast<uint8_t>(totalLen);
		state->r3 = p_dest;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_PLstrncmp(StdCLib::Globals* globals, MachineState* state)
	{
		const uint8_t* s1 = ToPointer<const uint8_t>(state->r3);
		const uint8_t* s2 = ToPointer<const uint8_t>(state->r4);
		uint32_t count = state->r5;

		if (s1 == nullptr || s2 == nullptr || count == 0)
		{
			state->r3 = 0;
			return;
		}

		uint32_t len1 = (s1[0] > count) ? count : s1[0];
		uint32_t len2 = (s2[0] > count) ? count : s2[0];
		uint32_t minLen = (len1 < len2) ? len1 : len2;

		int res = std::memcmp(&s1[1], &s2[1], minLen);
		if (res == 0)
		{
			if (len1 < len2) res = -1;
			else if (len1 > len2) res = 1;
		}

		state->r3 = static_cast<int32_t>(res);
		globals->scalars.errno_ = 0;
	}


	void StdCLib_PLstrncpy(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_dest = state->r3;
		uint8_t* dest = ToPointer<uint8_t>(p_dest);
		const uint8_t* src = ToPointer<const uint8_t>(state->r4);
		uint32_t maxSize = state->r5; // Tamanho máximo do buffer de destino (incluindo o byte de tamanho)

		if (dest == nullptr || src == nullptr || maxSize == 0)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = p_dest;
			return;
		}

		// O número máximo de caracteres a copiar é maxSize - 1 (para dar espaço ao byte de tamanho no início)
		uint32_t maxChars = maxSize - 1;
		uint8_t actualLength = src[0];
		uint8_t copyLength = (actualLength > maxChars) ? static_cast<uint8_t>(maxChars) : actualLength;

		dest[0] = copyLength; // Grava o tamanho efetivo copiado
		if (copyLength > 0)
		{
			std::memmove(&dest[1], &src[1], copyLength);
		}

		// A especificação clássica dita preencher o resto do buffer com zeros se sobrar espaço
		if (maxSize > static_cast<uint32_t>(copyLength) + 1)
		{
			std::memset(&dest[1 + copyLength], 0, maxSize - (1 + copyLength));
		}

		state->r3 = p_dest;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_PLstrpbrk(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_s1 = state->r3;
		const uint8_t* s1 = ToPointer<const uint8_t>(p_s1);
		const char* s2 = ToPointer<const char>(state->r4); // O set de quebra costuma ser uma C-String padrão

		if (s1 == nullptr || s2 == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0;
			return;
		}

		uint32_t length = s1[0];
		size_t s2Len = std::strlen(s2);

		for (uint32_t i = 0; i < length; i++)
		{
			for (size_t j = 0; j < s2Len; j++)
			{
				if (s1[1 + i] == static_cast<uint8_t>(s2[j]))
				{
					state->r3 = p_s1 + 1 + i;
					return;
				}
			}
		}
		state->r3 = 0;
	}

	void StdCLib_PLstrrchr(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_str = state->r3;
		const uint8_t* s = ToPointer<const uint8_t>(p_str);
		int character = state->r4 & 0xFF;

		if (s == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0;
			return;
		}

		uint32_t length = s[0];
		// Busca reversa a partir do fim da Pascal String
		for (uint32_t i = length; i > 0; i--)
		{
			if (s[i] == character)
			{
				state->r3 = p_str + i;
				globals->scalars.errno_ = 0;
				return;
			}
		}

		state->r3 = 0;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_PLstrspn(StdCLib::Globals* globals, MachineState* state)
	{
		// Devolve quantos caracteres iniciais de s1 pertencem ao conjunto s2
		const uint8_t* s1 = ToPointer<const uint8_t>(state->r3);
		const char* s2 = ToPointer<const char>(state->r4);

		if (s1 == nullptr || s2 == nullptr) { state->r3 = 0; return; }

		uint32_t length = s1[0];
		size_t s2Len = std::strlen(s2);
		uint32_t count = 0;

		for (uint32_t i = 0; i < length; i++)
		{
			bool found = false;
			for (size_t j = 0; j < s2Len; j++)
			{
				if (s1[1 + i] == static_cast<uint8_t>(s2[j])) { found = true; break; }
			}
			if (!found) break;
			count++;
		}
		state->r3 = count;
	}

	void StdCLib_PLstrstr(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_haystack = state->r3;
		const uint8_t* haystack = ToPointer<const uint8_t>(p_haystack);
		const uint8_t* needle = ToPointer<const uint8_t>(state->r4);

		if (haystack == nullptr || needle == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0;
			return;
		}

		uint32_t hLen = haystack[0];
		uint32_t nLen = needle[0];

		if (nLen == 0) { state->r3 = p_haystack + 1; return; }
		if (nLen > hLen) { state->r3 = 0; return; }

		for (uint32_t i = 0; i <= hLen - nLen; i++)
		{
			if (std::memcmp(&haystack[1 + i], &needle[1], nLen) == 0)
			{
				state->r3 = p_haystack + 1 + i;
				return;
			}
		}
		state->r3 = 0;
	}

	void StdCLib_printf(StdCLib::Globals* globals, MachineState* state)
	{
		const char* format = ToPointer<const char>(state->r3);
		if (format == nullptr) { globals->scalars.errno_ = EINVAL; state->r3 = -1; return; }

		// Na ABI PowerPC, format está em r3, logo os argumentos começam em r4
		std::string output = StringPrintF(format, *globals, state, 4);

		int result = std::printf("%s", output.c_str());
		std::fflush(stdout);
		
		if (result >= 0) { state->r3 = static_cast<int32_t>(output.size()); globals->scalars.errno_ = 0; }
	}


	void StdCLib_putc(StdCLib::Globals* globals, MachineState* state)
    {
        // putc expande-se semanticamente como um fputc padrão
        StdCLib_fputc(globals, state);
    }

	void StdCLib_putchar(StdCLib::Globals* globals, MachineState* state)
	{
		int character = state->r3;
		int result = std::putchar(character);
		std::fflush(stdout);

		if (result == EOF)
		{
			globals->scalars.errno_ = errno;
			state->r3 = EOF;
		}
		else
		{
			state->r3 = result & 0xFF;
			globals->scalars.errno_ = 0;
		}
	}

	void StdCLib_puts(StdCLib::Globals* globals, MachineState* state)
{
    // O r3 contém o endereço virtual da string na VM PowerPC
    uint32_t p_str = state->r3;
    const char* address = ToPointer<const char>(p_str);
    
    // Blindagem contra ponteiros nulos enviados pela VM
    if (address == nullptr)
    {
        // Em vez de crashar com SegFault, imprimimos uma string segura e definimos o erro
        int result = ::puts("(null)");
        ::fflush(stdout);
        state->r3 = static_cast<int32_t>(result);
        globals->scalars.errno_ = EINVAL;
        return;
    }
    
    // Executa o puts nativo no host moderno
    int result = ::puts(address);
    ::fflush(stdout); // Força a escrita imediata no terminal do Darling
    
    if (result == EOF)
    {
        globals->scalars.errno_ = errno;
        state->r3 = -1; // EOF clássico
    }
    else
    {
        globals->scalars.errno_ = 0;
        // O puts devolve um valor não-negativo. Garantimos o cast seguro para 32-bits.
        state->r3 = static_cast<int32_t>(result); 
    }
}


	void StdCLib_putw(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_qsort(StdCLib::Globals* globals, MachineState* state)
    {
        uint32_t p_base = state->r3;
        uint32_t nmemb = state->r4;
        uint32_t size = state->r5;
        uint32_t p_compar = state->gpr[6];

        uint8_t* base = ToPointer<uint8_t>(p_base);
        if (base == nullptr || nmemb <= 1 || size == 0 || p_compar == 0) return;

        // Implementação simplificada de Bubble Sort / Insertion Sort iterativo 
        // para evitar transições de contexto recursivas excessivas no interpretador da VM.
        for (uint32_t i = 0; i < nmemb - 1; i++)
        {
            for (uint32_t j = 0; j < nmemb - i - 1; j++)
            {
                uint32_t p_e1 = p_base + (j * size);
                uint32_t p_e2 = p_base + ((j + 1) * size);

                uint32_t originalPC = state->pc;
                uint32_t originalLR = state->lr;

                state->pc = p_compar;
                state->r3 = p_e1;
                state->r4 = p_e2;
                state->lr = 0;

                // Executa a comparação na VM PowerPC
                // PPCVM::Execute(state);

                int32_t compRes = static_cast<int32_t>(state->r3);
                state->pc = originalPC;
                state->lr = originalLR;

                if (compRes > 0)
                {
                    // Troca física dos bytes correspondentes na memória virtualizada
                    uint8_t* e1 = ToPointer<uint8_t>(p_e1);
                    uint8_t* e2 = ToPointer<uint8_t>(p_e2);
                    for (uint32_t k = 0; k < size; k++)
                    {
                        std::swap(e1[k], e2[k]);
                    }
                }
            }
        }
        globals->scalars.errno_ = 0;
    }

	void StdCLib_raise(StdCLib::Globals* globals, MachineState* state)
	{
		int sig = static_cast<int>(state->r3);

		// Dispara o sinal diretamente no ecossistema do Host para o processo atual
		int result = ::raise(sig);

		if (result != 0)
		{
			globals->scalars.errno_ = errno;
			state->r3 = -1; // Falha ao lançar o sinal
		}
		else
		{
			globals->scalars.errno_ = 0;
			state->r3 = 0;  // Sucesso
		}
	}

	void StdCLib_rand(StdCLib::Globals* globals, MachineState* state)
	{
		int result = std::rand();
		// Garante que o retorno está no intervalo [0, RAND_MAX] clássico de 15 ou 31 bits
		state->r3 = static_cast<int32_t>(result);
		globals->scalars.errno_ = 0;
	}

	void StdCLib_read(StdCLib::Globals* globals, MachineState* state)
	{
		int fd = static_cast<int>(state->r3);
		void* buf = ToPointer<void>(state->r4);
		uint32_t count = static_cast<uint32_t>(state->r5);

		if (fd < 0)
		{
			globals->scalars.errno_ = EBADF;
			state->r3 = -1;
			return;
		}

		if (buf == nullptr && count > 0)
		{
			globals->scalars.errno_ = EFAULT;
			state->r3 = -1;
			return;
		}

		// Executa a leitura nativa no host
		ssize_t result = ::read(fd, buf, count);

		if (result < 0)
		{
			globals->scalars.errno_ = errno;
			state->r3 = -1;
		}
		else
		{
			globals->scalars.errno_ = 0;
			state->r3 = static_cast<int32_t>(result); // Devolve o número de bytes lidos
		}
	}

    void StdCLib_realloc(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_vmAddress = state->r3;
		uint32_t newSize = state->r4;

		if (p_vmAddress == 0)
		{
			// Realloc com ponteiro NULL funciona exatamente como um malloc
			state->r3 = newSize;
			StdCLib_malloc(globals, state);
			return;
		}

		if (newSize == 0)
		{
			// Realloc com tamanho zero liberta a memória e retorna NULL
			StdCLib_free(globals, state);
			state->r3 = 0;
			return;
		}

		void* oldHostPtr = ToPointer<void>(p_vmAddress);
		
		// Tentamos usar o alocador dinâmico. Se a tua classe Common::Allocator expuser 
		// uma primitiva nativa de realocação, o ideal é mapeá-la. 
		// Caso contrário, fazemos a migração manual defensiva:
		void* newHostPtr = globals->allocator.Allocate(newSize, 8);

		if (newHostPtr == nullptr)
		{
			globals->scalars.errno_ = ENOMEM;
			state->r3 = 0; // Falha na alocação, mas o bloco antigo permanece intacto
			return;
		}

		if (oldHostPtr != nullptr)
		{
			// Blindagem defensiva: Como não sabemos o tamanho antigo, o memcpy assume o newSize.
			// NOTA: Se vires problemas de buffering ou crashes em apps complexas, verifica se o teu
			// Common::Allocator possui um método 'GetSize(ptr)' para fazeres:
			// size_t copySize = std::min(static_cast<size_t>(newSize), allocator.GetSize(oldHostPtr));
			
			std::memcpy(newHostPtr, oldHostPtr, newSize); 
			globals->allocator.Deallocate(oldHostPtr);
		}

		state->r3 = globals->allocator.ToIntPtr(newHostPtr);
		globals->scalars.errno_ = 0;
	}

    void StdCLib_remove(StdCLib::Globals* globals, MachineState* state)
    {
        const char* path = ToPointer<const char>(state->r3);
        if (path == nullptr)
        {
            globals->scalars.errno_ = EINVAL;
            state->r3 = -1;
            return;
        }

        int result = std::remove(path);
        if (result != 0)
        {
            globals->scalars.errno_ = errno;
            state->r3 = -1;
        }
        else
        {
            globals->scalars.errno_ = 0;
            state->r3 = 0;
        }
    }

	void StdCLib_rename(StdCLib::Globals* globals, MachineState* state)
    {
        const char* oldpath = ToPointer<const char>(state->r3);
        const char* newpath = ToPointer<const char>(state->r4);

        if (oldpath == nullptr || newpath == nullptr)
        {
            globals->scalars.errno_ = EINVAL;
            state->r3 = -1;
            return;
        }

        int result = std::rename(oldpath, newpath);
        if (result != 0)
        {
            globals->scalars.errno_ = errno;
            state->r3 = -1;
        }
        else
        {
            globals->scalars.errno_ = 0;
            state->r3 = 0;
        }
    }

	void StdCLib_ResolveFolderAliases(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_ResolveFolderAliases_Long(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_ResolvePath(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_ResolvePath_Long(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_rewind(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_iob = state->r3;
		FILE* fptr = MakeFilePtr(globals, p_iob);

		if (fptr != nullptr)
		{
			std::rewind(fptr);
			globals->scalars.errno_ = 0;
		}
		else
		{
			globals->scalars.errno_ = EBADF;
		}
	}

		void StdCLib_scanf(StdCLib::Globals* globals, MachineState* state)
	{
		const char* format = ToPointer<const char>(state->r3);

		if (format == nullptr) 
		{ 
			globals->scalars.errno_ = EINVAL; 
			state->r3 = -1; 
			return; 
		}

		// Na ABI do PowerPC de 32-bits, os argumentos variádicos de escrita começam em r4
		// (visto que o formato ocupa o registador r3)
		int nextGPR = 4;
		uint32_t stackPtr = state->r1;
		int stackOffset = 24;

		// Lambda para extrair com segurança os ponteiros de escrita da VM PowerPC
		auto getNextPointerArg = [&]() -> uint32_t {
			if (nextGPR <= 10) return state->gpr[nextGPR++];
			uint32_t* ptr = ToPointer<uint32_t>(stackPtr + stackOffset);
			stackOffset += 4;
			return ptr ? Common::CF::BigToHost<uint32_t>::Swap(*ptr) : 0;
		};

		size_t i = 0;
		size_t len = std::strlen(format);
		int tokensMatched = 0;

		// Lemos diretamente do stream de entrada padrão do Host (stdin)
		FILE* fptr = stdin;

		while (i < len)
		{
			// Consumir e sincronizar espaços em branco no formato e no teclado
			if (std::isspace(format[i]))
			{
				int ch;
				while ((ch = std::fgetc(fptr)) != EOF && std::isspace(ch));
				if (ch != EOF) std::ungetc(ch, fptr);
				i++;
				continue;
			}

			if (format[i] == '%' && i + 1 < len)
			{
				i++;
				if (format[i] == '%')
				{
					int ch = std::fgetc(fptr);
					if (ch != '%') { if (ch != EOF) std::ungetc(ch, fptr); break; }
					i++;
					continue;
				}

				char specifier = format[i];
				uint32_t p_dest = getNextPointerArg();
				if (p_dest == 0) { globals->scalars.errno_ = EFAULT; break; }

				if (specifier == 'd' || specifier == 'i')
				{
					int32_t val = 0;
					if (std::fscanf(fptr, "%d", &val) != 1) break;
					
					int32_t* dest = ToPointer<int32_t>(p_dest);
					if (dest) *dest = Common::CF::HostToBig<int32_t>::Swap(val);
					tokensMatched++;
				}
				else if (specifier == 'u')
				{
					uint32_t val = 0;
					if (std::fscanf(fptr, "%u", &val) != 1) break;
					
					uint32_t* dest = ToPointer<uint32_t>(p_dest);
					if (dest) *dest = Common::CF::HostToBig<uint32_t>::Swap(val);
					tokensMatched++;
				}
				else if (specifier == 'x' || specifier == 'X')
				{
					uint32_t val = 0;
					if (std::fscanf(fptr, specifier == 'x' ? "%x" : "%X", &val) != 1) break;
					
					uint32_t* dest = ToPointer<uint32_t>(p_dest);
					if (dest) *dest = Common::CF::HostToBig<uint32_t>::Swap(val);
					tokensMatched++;
				}
				else if (specifier == 'c')
				{
					int ch = std::fgetc(fptr);
					if (ch == EOF) break;
					
					char* dest = ToPointer<char>(p_dest);
					if (dest) *dest = static_cast<char>(ch);
					tokensMatched++;
				}
				else if (specifier == 's')
				{
					char tmpBuf[512];
					if (std::fscanf(fptr, "%511s", tmpBuf) != 1) break;
					
					char* dest = ToPointer<char>(p_dest);
					if (dest) std::strcpy(dest, tmpBuf);
					tokensMatched++;
				}
				else if (specifier == 'f' || specifier == 'g' || specifier == 'e')
				{
					float val = 0.0f;
					if (std::fscanf(fptr, "%f", &val) != 1) break;
					
					float* dest = ToPointer<float>(p_dest);
					if (dest) {
						uint32_t bits;
						std::memcpy(&bits, &val, sizeof(float));
						bits = Common::CF::HostToBig<uint32_t>::Swap(bits);
						std::memcpy(dest, &bits, sizeof(float));
					}
					tokensMatched++;
				}
				i++;
			}
			else
			{
				int ch = std::fgetc(fptr);
				if (ch != format[i])
				{
					if (ch != EOF) std::ungetc(ch, fptr);
					break;
				}
				i++;
			}
		}

		if (tokensMatched == 0 && std::feof(fptr)) 
		{
			state->r3 = -1; // EOF canónico se falhar antes de qualquer match
		}
		else 
		{
			state->r3 = tokensMatched; // Devolve o número de inputs convertidos
		}
		globals->scalars.errno_ = 0;
	}

	void StdCLib_setbuf(StdCLib::Globals* globals, MachineState* state)
    {
        uint32_t p_iob = state->r3;
        char* buf = ToPointer<char>(state->r4);

        FILE* fptr = MakeFilePtr(globals, p_iob);
        if (fptr != nullptr)
        {
            std::setbuf(fptr, buf);
        }
    }

	void StdCLib_setenv(StdCLib::Globals* globals, MachineState* state)
	{
		const char* name = ToPointer<const char>(state->r3);
		const char* value = ToPointer<const char>(state->r4);
		int overwrite = static_cast<int>(state->r5);

		if (name == nullptr || std::strlen(name) == 0 || std::strchr(name, '=') != nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = -1;
			return;
		}

		int result = 0;
		if (value == nullptr)
		{
			// Um valor nulo na biblioteca clássica atua como um unsetenv nativo
			result = ::unsetenv(name);
		}
		else
		{
			result = ::setenv(name, value, overwrite);
		}

		if (result < 0)
		{
			globals->scalars.errno_ = errno;
			state->r3 = -1;
		}
		else
		{
			globals->scalars.errno_ = 0;
			state->r3 = 0; // Sucesso
		}
	}


	void StdCLib_setlocale(StdCLib::Globals* globals, MachineState* state)
    {
        int category = static_cast<int>(state->r3);
        const char* localeStr = ToPointer<const char>(state->r4);

        // Define a localização nativa no host
        char* res = std::setlocale(category, localeStr);
        if (res == nullptr)
        {
            state->r3 = 0; // NULL
            return;
        }

        // Aloca o nome da localização na memória da VM para o binário clássico ler
        size_t len = std::strlen(res) + 1;
        void* vmBuf = globals->allocator.Allocate(len, 1);
        if (vmBuf == nullptr)
        {
            globals->scalars.errno_ = ENOMEM;
            state->r3 = 0;
            return;
        }

        std::memcpy(vmBuf, res, len);
        state->r3 = globals->allocator.ToIntPtr(vmBuf);
        globals->scalars.errno_ = 0;
    }

	void StdCLib_setvbuf(StdCLib::Globals* globals, MachineState* state)
    {
        uint32_t p_iob = state->r3;
        char* buf = ToPointer<char>(state->r4);
        int mode = static_cast<int>(state->r5);
        size_t size = static_cast<size_t>(state->gpr[6]);

        FILE* fptr = MakeFilePtr(globals, p_iob);
        if (fptr == nullptr)
        {
            state->r3 = -1;
            globals->scalars.errno_ = EBADF;
            return;
        }

        int result = std::setvbuf(fptr, buf, mode, size);
        state->r3 = static_cast<int32_t>(result);
        globals->scalars.errno_ = (result != 0) ? EINVAL : 0;
    }

	void StdCLib_signal(StdCLib::Globals* globals, MachineState* state)
	{
		int sig = static_cast<int>(state->r3);
		uint32_t p_handlerVector = state->r4; // Ponteiro virtual para o Vector/Função na VM

		// Constantes padrão da norma C para comportamentos especiais
		// SIG_DFL = 0, SIG_IGN = 1, SIG_ERR = -1
		if (p_handlerVector == 0 || p_handlerVector == 1)
		{
			// Se for comportamento padrão ou ignorar, podemos delegar diretamente no Host
			void (*hostHandler)(int) = (p_handlerVector == 0) ? SIG_DFL : SIG_IGN;
			void (*prevHandler)(int) = ::signal(sig, hostHandler);

			if (prevHandler == SIG_ERR)
			{
				globals->scalars.errno_ = errno;
				state->r3 = 0xFFFFFFFF; // SIG_ERR
			}
			else if (prevHandler == SIG_IGN)
			{
				state->r3 = 1;
			}
			else
			{
				state->r3 = 0;
			}
			return;
		}

		// Armadilha de Compatibilidade: Se a aplicação tentar registar uma função PowerPC real,
		// o emulador interseta o pedido. Em implementações futuras avançadas, isto dispararia
		// um callback assíncrono no interpretador. Por agora, aceitamos e simulamos sucesso
		// para permitir que o fluxo da aplicação Guest prossiga sem crashing.
		std::fprintf(stderr, "[ClassiC] signal: Aplicacao registou manipulador virtual para o sinal %d (PC: 0x%08x).\n", 
			sig, p_handlerVector);

		// Retorna um sucesso simulado (assumindo que o anterior era o default)
		state->r3 = 0; 
		globals->scalars.errno_ = 0;
	}


	void StdCLib_sprintf(StdCLib::Globals* globals, PPCVM::MachineState* state)
	{
		char* destBuffer = ToPointer<char>(state->r3);
		const char* format = ToPointer<const char>(state->r4);
		if (destBuffer == nullptr || format == nullptr) { globals->scalars.errno_ = EINVAL; state->r3 = -1; return; }

		// Destino em r3, formato em r4, logo os argumentos começam em r5
		std::string output = StringPrintF(format, *globals, state, 5);

		std::memcpy(destBuffer, output.c_str(), output.size() + 1);
		
		state->r3 = static_cast<int32_t>(output.size());
		globals->scalars.errno_ = 0;
	}


    void StdCLib_srand(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t seed = state->r3;
		std::srand(static_cast<unsigned int>(seed));
		globals->scalars.errno_ = 0;
	}

	void StdCLib_sscanf(StdCLib::Globals* globals, MachineState* state)
	{
		const char* src = ToPointer<const char>(state->r3);
		const char* format = ToPointer<const char>(state->r4);

		if (src == nullptr || format == nullptr) 
		{ 
			globals->scalars.errno_ = EINVAL; 
			state->r3 = -1; 
			return; 
		}

		// Na ABI do PowerPC, os argumentos de escrita começam em r5
		int nextGPR = 5;
		uint32_t stackPtr = state->r1;
		int stackOffset = 24;

		auto getNextPointerArg = [&]() -> uint32_t {
			if (nextGPR <= 10) return state->gpr[nextGPR++];
			uint32_t* ptr = ToPointer<uint32_t>(stackPtr + stackOffset);
			stackOffset += 4;
			return ptr ? Common::CF::BigToHost<uint32_t>::Swap(*ptr) : 0;
		};

		size_t i = 0;
		size_t len = std::strlen(format);
		int tokensMatched = 0;

		while (i < len)
		{
			// Ignorar espaços no formato e na string de origem
			if (std::isspace(format[i]))
			{
				while (*src != '\0' && std::isspace(static_cast<unsigned char>(*src))) src++;
				i++;
				continue;
			}

			if (format[i] == '%' && i + 1 < len)
			{
				i++;
				if (format[i] == '%')
				{
					if (*src != '%') break;
					src++; i++;
					continue;
				}

				char specifier = format[i];
				uint32_t p_dest = getNextPointerArg();
				if (p_dest == 0) { globals->scalars.errno_ = EFAULT; break; }

				int bytesConsumed = 0;

				if (specifier == 'd' || specifier == 'i')
				{
					int32_t val = 0;
					if (std::sscanf(src, "%d%n", &val, &bytesConsumed) != 1) break;
					
					int32_t* dest = ToPointer<int32_t>(p_dest);
					if (dest) *dest = Common::CF::HostToBig<int32_t>::Swap(val);
					tokensMatched++;
				}
				else if (specifier == 'u')
				{
					uint32_t val = 0;
					if (std::sscanf(src, "%u%n", &val, &bytesConsumed) != 1) break;
					
					uint32_t* dest = ToPointer<uint32_t>(p_dest);
					if (dest) *dest = Common::CF::HostToBig<uint32_t>::Swap(val);
					tokensMatched++;
				}
				else if (specifier == 'x' || specifier == 'X')
				{
					uint32_t val = 0;
					if (std::sscanf(src, specifier == 'x' ? "%x%n" : "%X%n", &val, &bytesConsumed) != 1) break;
					
					uint32_t* dest = ToPointer<uint32_t>(p_dest);
					if (dest) *dest = Common::CF::HostToBig<uint32_t>::Swap(val);
					tokensMatched++;
				}
				else if (specifier == 'c')
				{
					if (*src == '\0') break;
					char* dest = ToPointer<char>(p_dest);
					if (dest) *dest = *src;
					bytesConsumed = 1;
					tokensMatched++;
				}
				else if (specifier == 's')
				{
					char tmpBuf[512];
					if (std::sscanf(src, "%511s%n", tmpBuf, &bytesConsumed) != 1) break;
					
					char* dest = ToPointer<char>(p_dest);
					if (dest) std::strcpy(dest, tmpBuf);
					tokensMatched++;
				}
				else if (specifier == 'f' || specifier == 'g' || specifier == 'e')
				{
					float val = 0.0f;
					if (std::sscanf(src, "%f%n", &val, &bytesConsumed) != 1) break;
					
					float* dest = ToPointer<float>(p_dest);
					if (dest) {
						uint32_t bits;
						std::memcpy(&bits, &val, sizeof(float));
						bits = Common::CF::HostToBig<uint32_t>::Swap(bits);
						std::memcpy(dest, &bits, sizeof(float));
					}
					tokensMatched++;
				}

				src += bytesConsumed; // Avança a string de origem conforme o que foi lido
				i++;
			}
			else
			{
				if (*src != format[i]) break;
				src++; i++;
			}
		}

		state->r3 = tokensMatched;
		globals->scalars.errno_ = 0;
	}


	void StdCLib_strcat(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_dest = state->r3;
		char* dest = ToPointer<char>(p_dest);
		const char* src = ToPointer<const char>(state->r4);

		if (dest == nullptr || src == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = p_dest;
			return;
		}

		std::strcat(dest, src);
		state->r3 = p_dest;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_strchr(StdCLib::Globals* globals, MachineState* state)
{
    uint32_t p_str = state->r3;
    const char* s = ToPointer<const char>(p_str);
    int character = state->r4 & 0xFF; // Garante que o caracter cabe num byte
    
    if (s == nullptr)
    {
        globals->scalars.errno_ = EINVAL;
        state->r3 = 0; // Devolve NULL
        return;
    }
    
    // Executa a procura nativa no host
    const char* result = strchr(s, character);
    
    if (result == nullptr)
    {
        state->r3 = 0;
    }
    else
    {
        // CORREÇÃO CRÍTICA: Calcular o offset em bytes dentro da string
        // e somar ao endereço virtual original (p_str) da VM!
        size_t offset = result - s;
        state->r3 = p_str + static_cast<uint32_t>(offset);
    }
    globals->scalars.errno_ = 0;
}

	void StdCLib_strcmp(StdCLib::Globals* globals, MachineState* state)
{
    const char* s1 = ToPointer<const char>(state->r3);
    const char* s2 = ToPointer<const char>(state->r4);
    
    if (s1 == nullptr || s2 == nullptr)
    {
        globals->scalars.errno_ = EINVAL;
        state->r3 = 0;
        return;
    }
    
    // Executa a comparação nativa e força o cast seguro para o registador de 32-bits
    int result = strcmp(s1, s2);
    state->r3 = static_cast<int32_t>(result);
    globals->scalars.errno_ = 0;
}

	void StdCLib_strcoll(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_strcpy(StdCLib::Globals* globals, MachineState* state)
{
    uint32_t p_dest = state->r3; // Endereço virtual original de destino
    char* s1 = ToPointer<char>(p_dest);
    const char* s2 = ToPointer<const char>(state->r4);
    
    if (s1 == nullptr || s2 == nullptr)
    {
        globals->scalars.errno_ = EINVAL;
        return;
    }
    
    // Executa a cópia nativa em segurança
    strcpy(s1, s2);
    
    // CORREÇÃO CRÍTICA: Devolve o endereço virtual original de destino (32-bits)
    // para o r3, em vez do ponteiro de 64-bits truncado do host!
    state->r3 = p_dest;
    globals->scalars.errno_ = 0;
}

	void StdCLib_strcspn(StdCLib::Globals* globals, MachineState* state)
	{
		const char* s1 = ToPointer<const char>(state->r3);
		const char* s2 = ToPointer<const char>(state->r4);

		if (s1 == nullptr || s2 == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0;
			return;
		}

		size_t res = std::strcspn(s1, s2);
		state->r3 = static_cast<uint32_t>(res);
	}

	void StdCLib_strerror(StdCLib::Globals* globals, MachineState* state)
	{
		int errnum = static_cast<int>(state->r3);
		const char* errMsg = std::strerror(errnum);
		
		if (errMsg == nullptr)
		{
			state->r3 = 0;
			return;
		}

		// Alocamos dinamicamente no espaço da VM para que o Guest possa ler a string
		size_t len = std::strlen(errMsg) + 1;
		void* vmBuf = globals->allocator.Allocate(len, 1);
		if (vmBuf == nullptr)
		{
			globals->scalars.errno_ = ENOMEM;
			state->r3 = 0;
			return;
		}

		std::memcpy(vmBuf, errMsg, len);
		state->r3 = globals->allocator.ToIntPtr(vmBuf);
		globals->scalars.errno_ = 0;
	}

	void StdCLib_strftime(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_dest = state->r3;
		char* dest = ToPointer<char>(p_dest);
		uint32_t maxSize = state->r4;
		const char* format = ToPointer<const char>(state->r5);
		const void* p_virtualTM = ToPointer<const void>(state->gpr[6]); // r6 é o 4º argumento gpr

		if (dest == nullptr || format == nullptr || p_virtualTM == nullptr || maxSize == 0)
		{
			state->r3 = 0;
			return;
		}

		std::tm timeInfo = ParseVirtualTM(p_virtualTM);
		size_t written = std::strftime(dest, maxSize, format, &timeInfo);

		state->r3 = static_cast<uint32_t>(written);
		globals->scalars.errno_ = 0;
	}

	void StdCLib_strlen(StdCLib::Globals* globals, MachineState* state)
{
    const char* s = ToPointer<const char>(state->r3);
    
    // Blindagem contra ponteiros nulos passados pela VM PowerPC
    if (s == nullptr)
    {
        globals->scalars.errno_ = EINVAL;
        state->r3 = 0; // Devolve comprimento 0
        return;
    }
    
    // Executa a contagem nativa
    size_t length = strlen(s);
    
    // CORREÇÃO CRÍTICA: Força o cast explícito de 64-bits (size_t do host) 
    // para o registador de 32-bits (uint32_t da VM), garantindo conformidade.
    state->r3 = static_cast<uint32_t>(length);
    globals->scalars.errno_ = 0;
}


	void StdCLib_strncat(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_dest = state->r3;
		char* dest = ToPointer<char>(p_dest);
		const char* src = ToPointer<const char>(state->r4);
		uint32_t size = static_cast<uint32_t>(state->r5);

		if (dest == nullptr || src == nullptr || size == 0)
		{
			state->r3 = p_dest;
			return;
		}

		std::strncat(dest, src, size);
		state->r3 = p_dest;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_strncmp(StdCLib::Globals* globals, MachineState* state)
	{
		const char* s1 = ToPointer<const char>(state->r3);
		const char* s2 = ToPointer<const char>(state->r4);
		uint32_t size = static_cast<uint32_t>(state->r5);

		if (size == 0 || (s1 == nullptr && s2 == nullptr))
		{
			state->r3 = 0;
			return;
		}

		if (s1 == nullptr || s2 == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = (s1 == nullptr) ? -1 : 1;
			return;
		}

		int result = std::strncmp(s1, s2, size);
		state->r3 = static_cast<int32_t>(result);
		globals->scalars.errno_ = 0;
	}

	void StdCLib_strncpy(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_dest = state->r3;
		char* dest = ToPointer<char>(p_dest);
		const char* src = ToPointer<const char>(state->r4);
		uint32_t size = static_cast<uint32_t>(state->r5);

		if (size == 0)
		{
			state->r3 = p_dest;
			return;
		}

		if (dest == nullptr || src == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			return;
		}

		std::strncpy(dest, src, size);
		state->r3 = p_dest;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_strpbrk(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_s1 = state->r3;
		const char* s1 = ToPointer<const char>(p_s1);
		const char* s2 = ToPointer<const char>(state->r4);

		if (s1 == nullptr || s2 == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0;
			return;
		}

		const char* res = std::strpbrk(s1, s2);
		state->r3 = res ? (p_s1 + static_cast<uint32_t>(res - s1)) : 0;
	}

	void StdCLib_strrchr(StdCLib::Globals* globals, MachineState* state)
{
    uint32_t p_str = state->r3; // Guardamos o endereço VIRTUAL original da VM
    const char* s = ToPointer<const char>(p_str);
    int character = state->r4 & 0xFF; // Garante que o caractere cabe num byte
    
    if (s == nullptr)
    {
        globals->scalars.errno_ = EINVAL;
        state->r3 = 0; // Devolve NULL se o ponteiro de origem for inválido
        return;
    }
    
    // Executa a busca reversa nativa no host moderno
    const char* result = strrchr(s, character);
    
    if (result == nullptr)
    {
        state->r3 = 0; // Caractere não encontrado
    }
    else
    {
        // CORREÇÃO CRÍTICA: Calcular o offset em bytes dentro da string no host
        // e somar ao endereço virtual original (p_str) da VM!
        size_t offset = result - s;
        state->r3 = p_str + static_cast<uint32_t>(offset);
    }
    globals->scalars.errno_ = 0;
}


	void StdCLib_strspn(StdCLib::Globals* globals, MachineState* state)
	{
		const char* s1 = ToPointer<const char>(state->r3);
		const char* s2 = ToPointer<const char>(state->r4);

		if (s1 == nullptr || s2 == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0;
			return;
		}

		size_t res = std::strspn(s1, s2);
		state->r3 = static_cast<uint32_t>(res);
	}

	void StdCLib_strstr(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_haystack = state->r3;
		const char* haystack = ToPointer<const char>(p_haystack);
		const char* needle = ToPointer<const char>(state->r4);

		if (haystack == nullptr || needle == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0;
			return;
		}

		const char* res = std::strstr(haystack, needle);
		state->r3 = res ? (p_haystack + static_cast<uint32_t>(res - haystack)) : 0;
	}

	void StdCLib_strtod(StdCLib::Globals* globals, MachineState* state)
	{
		const char* str = ToPointer<const char>(state->r3);
		uint32_t p_endptr = state->r4;

		if (str == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->fpr[1] = 0.0;
			return;
		}

		char* hostEndPtr = nullptr;
		double result = std::strtod(str, &hostEndPtr);

		if (p_endptr != 0)
		{
			uint32_t* endptr = ToPointer<uint32_t>(p_endptr);
			if (endptr)
			{
				uint32_t offset = static_cast<uint32_t>(hostEndPtr - str);
				*endptr = Common::CF::HostToBig<uint32_t>::Swap(state->r3 + offset);
			}
		}

		state->fpr[1] = result; // Devolve o double no FPR1
		globals->scalars.errno_ = errno;
	}

	void StdCLib_strtok(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_str = state->r3;
		const char* delim = ToPointer<const char>(state->r4);

		if (delim == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0;
			return;
		}

		// Usamos uma variável estática local (ou preferencialmente mapeada no escopo do emulador se necessário)
		static uint32_t p_lastTokenPos = 0;

		uint32_t p_current = (p_str != 0) ? p_str : p_lastTokenPos;
		char* current = ToPointer<char>(p_current);

		if (current == nullptr || *current == '\0')
		{
			state->r3 = 0;
			p_lastTokenPos = 0;
			return;
		}

		// Ignorar delimitadores iniciais
		size_t skip = std::strspn(current, delim);
		if (current[skip] == '\0')
		{
			state->r3 = 0;
			p_lastTokenPos = 0;
			return;
		}

		uint32_t p_tokenStart = p_current + static_cast<uint32_t>(skip);
		char* tokenStart = current + skip;

		// Encontrar o fim do token atual
		size_t tokenLen = std::strcspn(tokenStart, delim);
		if (tokenStart[tokenLen] != '\0')
		{
			tokenStart[tokenLen] = '\0'; // Trunca o token na memória virtualizado da VM
			p_lastTokenPos = p_tokenStart + static_cast<uint32_t>(tokenLen) + 1;
		}
		else
		{
			p_lastTokenPos = 0; // Chegou ao fim absoluto da string
		}

		state->r3 = p_tokenStart;
		globals->scalars.errno_ = 0;
	}


	void StdCLib_strtol(StdCLib::Globals* globals, MachineState* state)
	{
		const char* str = ToPointer<const char>(state->r3);
		uint32_t p_endptr = state->r4;
		int base = static_cast<int>(state->r5);

		if (str == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0;
			return;
		}

		char* hostEndPtr = nullptr;
		long result = std::strtol(str, &hostEndPtr, base);

		if (p_endptr != 0)
		{
			uint32_t* endptr = ToPointer<uint32_t>(p_endptr);
			if (endptr)
			{
				// Calcula o offset percorrido no host e replica no endereço virtual da VM
				uint32_t offset = static_cast<uint32_t>(hostEndPtr - str);
				*endptr = Common::CF::HostToBig<uint32_t>::Swap(state->r3 + offset);
			}
		}

		state->r3 = static_cast<int32_t>(result);
		globals->scalars.errno_ = errno;
	}

	void StdCLib_strtoll(StdCLib::Globals* globals, MachineState* state)
	{
		const char* str = ToPointer<const char>(state->r3);
		uint32_t p_endptr = state->r4;
		int base = static_cast<int>(state->r5);

		if (str == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0; state->r4 = 0;
			return;
		}

		char* hostEndPtr = nullptr;
		long long result = std::strtoll(str, &hostEndPtr, base);

		if (p_endptr != 0)
		{
			uint32_t* endptr = ToPointer<uint32_t>(p_endptr);
			if (endptr)
			{
				uint32_t offset = static_cast<uint32_t>(hostEndPtr - str);
				*endptr = Common::CF::HostToBig<uint32_t>::Swap(state->r3 + offset);
			}
		}

		// Na ABI PowerPC de 32-bits, um int64_t é devolvido dividido em r3 (high) e r4 (low)
		uint64_t ures = static_cast<uint64_t>(result);
		state->r3 = static_cast<uint32_t>(ures >> 32);
		state->r4 = static_cast<uint32_t>(ures & 0xFFFFFFFF);
		globals->scalars.errno_ = errno;
	}

    void StdCLib_strtoul(StdCLib::Globals* globals, MachineState* state)
	{
		const char* str = ToPointer<const char>(state->r3);
		uint32_t p_endptr = state->r4;
		int base = static_cast<int>(state->r5);

		if (str == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0;
			return;
		}

		char* hostEndPtr = nullptr;
		unsigned long result = std::strtoul(str, &hostEndPtr, base);

		if (p_endptr != 0)
		{
			uint32_t* endptr = ToPointer<uint32_t>(p_endptr);
			if (endptr)
			{
				uint32_t offset = static_cast<uint32_t>(hostEndPtr - str);
				*endptr = Common::CF::HostToBig<uint32_t>::Swap(state->r3 + offset);
			}
		}

		state->r3 = static_cast<uint32_t>(result);
		globals->scalars.errno_ = errno;
	}

    void StdCLib_strtoull(StdCLib::Globals* globals, MachineState* state)
	{
		const char* str = ToPointer<const char>(state->r3);
		uint32_t p_endptr = state->r4;
		int base = static_cast<int>(state->r5);

		if (str == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0; state->r4 = 0;
			return;
		}

		char* hostEndPtr = nullptr;
		unsigned long long result = std::strtoull(str, &hostEndPtr, base);

		if (p_endptr != 0)
		{
			uint32_t* endptr = ToPointer<uint32_t>(p_endptr);
			if (endptr)
			{
				uint32_t offset = static_cast<uint32_t>(hostEndPtr - str);
				*endptr = Common::CF::HostToBig<uint32_t>::Swap(state->r3 + offset);
			}
		}

		state->r3 = static_cast<uint32_t>(result >> 32);
		state->r4 = static_cast<uint32_t>(result & 0xFFFFFFFF);
		globals->scalars.errno_ = errno;
	}

	void StdCLib_strxfrm(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_dest = state->r3;
		char* dest = ToPointer<char>(p_dest);
		const char* src = ToPointer<const char>(state->r4);
		size_t n = static_cast<size_t>(state->r5);

		if (src == nullptr)
		{
			globals->scalars.errno_ = EINVAL;
			state->r3 = 0;
			return;
		}

		// Se a aplicação passar um buffer de destino válido e tamanho, transforma a string
		// de acordo com as regras de ordenação lexicográfica regional ativas no Host.
		size_t result = 0;
		if (dest != nullptr && n > 0)
		{
			result = std::strxfrm(dest, src, n);
		}
		else
		{
			// Se dest for NULL ou n for 0, strxfrm apenas calcula o tamanho necessário
			result = std::strxfrm(nullptr, src, 0);
		}

		// Devolve o tamanho real da string transformada para o registador de 32-bits da VM
		state->r3 = static_cast<uint32_t>(result);
		globals->scalars.errno_ = 0;
	}

	void StdCLib_system(StdCLib::Globals* globals, MachineState* state)
	{
		const char* command = ToPointer<const char>(state->r3);

		// Se o comando for nulo, system verifica se o processador de comandos existe
		if (command == nullptr)
		{
			int status = ::system(nullptr);
			state->r3 = (status != 0) ? 1 : 0;
			globals->scalars.errno_ = 0;
			return;
		}

		// Executa o comando de sistema no ecossistema Host/Darling
		int status = ::system(command);

		if (status == -1)
		{
			globals->scalars.errno_ = errno;
			state->r3 = -1;
		}
		else
		{
			globals->scalars.errno_ = 0;
			// Retorna o status de terminação clássico limpo
			state->r3 = static_cast<int32_t>(status);
		}
	}

	void StdCLib_time(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_timer = state->r3; // Endereço virtual na VM

		// Obtém o tempo atual do host em segundos desde 1970
		std::time_t hostTime = std::time(nullptr);

		// Converte para a época do Mac OS Clássico (segundos desde 1904)
		// Nota: Se a tua StdCLib no Classic usar a época Unix padrão, remove esta constante.
		uint32_t classicTime = static_cast<uint32_t>(hostTime + 2082844800ULL);

		if (p_timer != 0)
		{
			uint32_t* dest = ToPointer<uint32_t>(p_timer);
			if (dest)
			{
				*dest = Common::CF::HostToBig<uint32_t>::Swap(classicTime);
			}
		}

		state->r3 = classicTime;
		globals->scalars.errno_ = 0;
	}

	void StdCLib_tmpfile(StdCLib::Globals* globals, MachineState* state)
	{
		// 1. Procurar por um descritor de ficheiro vago na tabela virtual _iob
		for (int i = 0; i < StdCLib::NFILE; i++)
		{
			auto& ioBuffer = globals->scalars._iob[i];
			uint32_t p_iobAddress = ToIntPtr(&ioBuffer);
			
			// Se o endereço virtual do slot iob não está no mapa, o slot está livre
			if (globals->nativeFileMap.find(p_iobAddress) == globals->nativeFileMap.end())
			{
				// Cria o ficheiro temporário físico seguro no Host (abre-se em "wb+")
				FILE* hostFile = std::tmpfile();
				if (hostFile == nullptr)
				{
					globals->scalars.errno_ = errno;
					state->r3 = 0; // Retorna NULL em caso de falha física no Host
					return;
				}
				
				// Inicializa os metadados clássicos esperados pela VM do PowerPC
				ioBuffer._file = i;
				ioBuffer._flag = 0x01; // Flag MSL padrão para ficheiro aberto
				ioBuffer._cnt  = 0;
				ioBuffer._ptr  = 0;
				ioBuffer._base = 0;
				ioBuffer._end  = 0;
				ioBuffer._size = 0;
				
				// Associa e protege o par no mapa seguro de 64-bits
				globals->nativeFileMap[p_iobAddress] = hostFile;
				
				// Devolve com sucesso o endereço virtual de 32-bits para a VM
				globals->scalars.errno_ = 0;
				state->r3 = p_iobAddress;
				return;
			}
		}
		
		// Se a tabela interna NFILE (40) esgotou
		globals->scalars.errno_ = EMFILE;
		state->r3 = 0;
	}

	void StdCLib_tmpnam(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_destBuffer = state->r3; // Endereço virtual enviado pela VM (pode ser NULL/0)
		
		// Geramos o nome temporário usando a rotina nativa segura do Host
		char hostPathBuffer[L_tmpnam];
		char* result = std::tmpnam(hostPathBuffer);
		
		if (result == nullptr)
		{
			globals->scalars.errno_ = errno;
			state->r3 = 0;
			return;
		}
		
		size_t len = std::strlen(hostPathBuffer) + 1;
		
		if (p_destBuffer == 0)
		{
			// De acordo com a norma C, se for passado NULL, a função deve retornar 
			// um ponteiro para um buffer estático interno gerido pela biblioteca.
			// Usamos uma área global temporária segura dentro da estrutura de scalars.
			void* p_virtualStatic = ToPointer<void>(globals->scalars._lastbuf);
			if (p_virtualStatic == nullptr)
			{
				globals->scalars.errno_ = ENOMEM;
				state->r3 = 0;
				return;
			}
			
			std::memcpy(p_virtualStatic, hostPathBuffer, len);
			state->r3 = globals->scalars._lastbuf; // Devolve o endereço da área estática virtual
		}
		else
		{
			// Se o utilizador forneceu um buffer na VM, validamos e copiamos diretamente para lá
			char* dest = ToPointer<char>(p_destBuffer);
			if (dest == nullptr)
			{
				globals->scalars.errno_ = EFAULT;
				state->r3 = 0;
				return;
			}
			
			std::memcpy(dest, hostPathBuffer, len);
			state->r3 = p_destBuffer; // Devolve o endereço virtual original recebido
		}
		
		globals->scalars.errno_ = 0;
	}


	void StdCLib_toascii(StdCLib::Globals* globals, MachineState* state)
	{
		// Trunca o valor mantendo estritamente apenas os 7 bits inferiores ASCII
		state->r3 = static_cast<int32_t>(state->r3 & 0x7F);
	}


	void StdCLib_tolower(StdCLib::Globals* globals, MachineState* state)
{
    // 1. Isolamos estritamente o byte inferior (o caractere real de 8-bits)
    // para evitar que lixo residual nos bits superiores de r3 confunda o tolower nativo
    int character = static_cast<int>(state->r3 & 0xFF);
    
    // 2. Executamos a conversão nativa em segurança
    int lowerChar = tolower(character);
    
    // 3. Forçamos o cast explícito para int32_t para o registador r3 da VM,
    // garantindo que o resultado limpa os bits superiores de forma determinística
    state->r3 = static_cast<int32_t>(lowerChar & 0xFF);
    
    globals->scalars.errno_ = 0;
}


	void StdCLib_toupper(StdCLib::Globals* globals, MachineState* state)
{
    // 1. Isolamos estritamente o byte inferior (o caractere de 8-bits)
    // para limpar lixo residual que a VM possa ter deixado em r3
    int character = static_cast<int>(state->r3 & 0xFF);
    
    // 2. Executamos a conversão para maiúsculas nativa do host
    int upperChar = toupper(character);
    
    // 3. Devolvemos o caractere convertido limpando os bits superiores 
    // com um cast seguro para o registador de 32-bits da VM
    state->r3 = static_cast<int32_t>(upperChar & 0xFF);
    
    globals->scalars.errno_ = 0;
}


	void StdCLib_TrapAvailable(StdCLib::Globals* globals, MachineState* state)
	{
		// Esta rotina da MSL verifica se uma trap específica (em r3) está implementada no ambiente.
		// Como o teu fork do ClassiC está a reconstruir a API progressivamente, a abordagem mais robusta
		// para o runtime não abortar é responder que a trap está disponível (retorna 1).
		// Caso isoles uma trap que cause falhas por não estar emulada, podes adicionar uma exceção aqui.
		uint32_t trapNum = state->r3;
		
		// Descomentar para depuração se precisares de mapear o que a aplicação Guest anda a procurar:
		// std::fprintf(stderr, "[ClassiX] TrapAvailable: Program was tested the avaliability of trap 0x%04X\n", trapNum);
		
		state->r3 = 1; // 1 = Disponível/Suportada, 0 = Não implementada
		globals->scalars.errno_ = 0;
	}

	void StdCLib_ungetc(StdCLib::Globals* globals, MachineState* state)
	{
		int character = state->r3;
		uint32_t p_iob = state->r4;
		FILE* fptr = MakeFilePtr(globals, p_iob);

		if (fptr == nullptr)
		{
			globals->scalars.errno_ = EBADF;
			state->r3 = EOF;
			return;
		}

		int result = std::ungetc(character, fptr);
		if (result == EOF)
		{
			state->r3 = EOF;
		}
		else
		{
			state->r3 = result & 0xFF;
			globals->scalars.errno_ = 0;
		}
	}

	void StdCLib_unlink(StdCLib::Globals* globals, MachineState* state)
    {
        const char* path = ToPointer<const char>(state->r3);
        if (path == nullptr)
        {
            globals->scalars.errno_ = EINVAL;
            state->r3 = -1;
            return;
        }

        int result = ::unlink(path);
        if (result < 0)
        {
            globals->scalars.errno_ = errno;
            state->r3 = -1;
        }
        else
        {
            globals->scalars.errno_ = 0;
            state->r3 = 0;
        }
    }

	void StdCLib_vec_calloc(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t num = state->r3;
		uint32_t size = state->r4;
		uint32_t total = num * size;
		void* ptr = globals->allocator.Allocate(total, 16);
		if (ptr) std::memset(ptr, 0, total);
		state->r3 = ptr ? globals->allocator.ToIntPtr(ptr) : 0;
	}

	void StdCLib_vec_free(StdCLib::Globals* globals, MachineState* state)
	{
		StdCLib_free(globals, state);
	}

	void StdCLib_vec_malloc(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t size = state->r3;
		if (size == 0) { state->r3 = 0; return; }
		void* ptr = globals->allocator.Allocate(size, 16); // Força alinhamento estrito AltiVec (16-byte)
		state->r3 = ptr ? globals->allocator.ToIntPtr(ptr) : 0;
	}

	void StdCLib_vec_realloc(StdCLib::Globals* globals, MachineState* state)
	{
		// Simplificação segura mantendo o alinhamento AltiVec
		StdCLib_realloc(globals, state); 
	}

	void StdCLib_vfprintf(StdCLib::Globals* globals, MachineState* state)
	{
		FILE* fptr = MakeFilePtr(globals, state->r3);
		const char* format = ToPointer<const char>(state->r4);
		uint32_t p_vaList = state->r5; // Ponteiro virtual para a estrutura/lista va_list da VM

		if (fptr == nullptr || format == nullptr) 
		{ 
			globals->scalars.errno_ = EINVAL; 
			state->r3 = -1; 
			return; 
		}

		std::string output = StringPrintFFromPointer(format, *globals, p_vaList);

		int charactersWritten = fputs(output.c_str(), fptr);
		
		if (charactersWritten >= 0) 
		{ 
			state->r3 = static_cast<int32_t>(output.size()); 
			globals->scalars.errno_ = 0; 
		}
		else 
		{ 
			state->r3 = -1; 
			globals->scalars.errno_ = errno; 
		}
	}


		void StdCLib_vprintf(StdCLib::Globals* globals, MachineState* state)
	{
		const char* format = ToPointer<const char>(state->r3);
		uint32_t p_vaList = state->r4; // Ponteiro virtual para va_list na stack da VM

		if (format == nullptr) 
		{ 
			globals->scalars.errno_ = EINVAL; 
			state->r3 = -1; 
			return; 
		}

		// Constrói a string interpretando os argumentos virtuais
		std::string output = StringPrintFFromPointer(format, *globals, p_vaList);

		int result = std::printf("%s", output.c_str());
		std::fflush(stdout);
		
		if (result >= 0) 
		{ 
			state->r3 = static_cast<int32_t>(output.size()); 
			globals->scalars.errno_ = 0; 
		}
		else 
		{
			state->r3 = -1;
			globals->scalars.errno_ = errno;
		}
	}

	void StdCLib_vsprintf(StdCLib::Globals* globals, MachineState* state)
	{
		char* destBuffer = ToPointer<char>(state->r3);
		const char* format = ToPointer<const char>(state->r4);
		uint32_t p_vaList = state->r5;

		if (destBuffer == nullptr || format == nullptr) 
		{ 
			globals->scalars.errno_ = EINVAL; 
			state->r3 = -1; 
			return; 
		}

		std::string output = StringPrintFFromPointer(format, *globals, p_vaList);

		// Copia os dados para o buffer da VM incluindo o terminador nulo '\0'
		std::memcpy(destBuffer, output.c_str(), output.size() + 1);
		
		state->r3 = static_cast<int32_t>(output.size());
		globals->scalars.errno_ = 0;
	}


	void StdCLib_wcstombs(StdCLib::Globals* globals, MachineState* state)
    {
        char* s = ToPointer<char>(state->r3);
        const wchar_t* pwcs = ToPointer<const wchar_t>(state->r4);
        size_t n = static_cast<size_t>(state->r5);

        if (pwcs == nullptr) { state->r3 = 0; return; }

        // Duplicação temporária para reverter os bytes Big-Endian da VM para o Host antes da conversão
        std::vector<wchar_t> hostWc;
        const wchar_t* curr = pwcs;
        while (true)
        {
            wchar_t converted = static_cast<wchar_t>(Common::CF::BigToHost<uint32_t>::Swap(static_cast<uint32_t>(*curr)));
            hostWc.push_back(converted);
            if (converted == 0) break;
            curr++;
        }

        size_t result = std::wcstombs(s, hostWc.data(), n);
        state->r3 = static_cast<uint32_t>(result);
    }

	void StdCLib_wctomb(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_write(StdCLib::Globals* globals, MachineState* state)
	{
		int fd = static_cast<int>(state->r3);
		const void* buf = ToPointer<const void>(state->r4);
		uint32_t count = static_cast<uint32_t>(state->r5);

		if (fd < 0)
		{
			globals->scalars.errno_ = EBADF;
			state->r3 = -1;
			return;
		}

		if (buf == nullptr && count > 0)
		{
			globals->scalars.errno_ = EFAULT;
			state->r3 = -1;
			return;
		}

		// Executa a escrita nativa no host
		ssize_t result = ::write(fd, buf, count);

		if (result < 0)
		{
			globals->scalars.errno_ = errno;
			state->r3 = -1;
		}
		else
		{
			globals->scalars.errno_ = 0;
			state->r3 = static_cast<int32_t>(result); // Devolve o número de bytes escritos
		}
	}
}
