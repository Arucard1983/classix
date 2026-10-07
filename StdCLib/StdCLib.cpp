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
    int stackOffset = 24; 

    // Lambdas auxiliares para extração segura de tipos da ABI da VM
    auto getNextArg32 = [&]() -> uint32_t {
        if (nextGPR <= 10) return state->gpr[nextGPR++];
        uint32_t* ptr = ToPointer<uint32_t>(stackPtr + stackOffset);
        stackOffset += 4;
        return ptr ? Common::CF::BigToHost<uint32_t>::Swap(*ptr) : 0;
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
        if (format[i] == '%' && i + 1 < len)
        {
            i++; 
            if (format[i] == '%') { output += '%'; i++; continue; }

            // Ignorar flags de largura/precisão básicas para o parser simples
            while (i < len && (format[i] == '.' || (format[i] >= '0' && format[i] <= '9') || format[i] == '-')) i++;
            if (i >= len) break;

            char specifier = format[i];
            char buffer[256];

            switch (specifier)
            {
                case 'd': case 'i':
                    std::snprintf(buffer, sizeof(buffer), "%d", static_cast<int32_t>(getNextArg32()));
                    output += buffer;
                    break;
                case 'u':
                    std::snprintf(buffer, sizeof(buffer), "%u", getNextArg32());
                    output += buffer;
                    break;
                case 'x': case 'X':
                    std::snprintf(buffer, sizeof(buffer), specifier == 'x' ? "%x" : "%X", getNextArg32());
                    output += buffer;
                    break;
                case 'c':
                    output += static_cast<char>(getNextArg32() & 0xFF);
                    break;
                case 's': {
                    const char* str = ToPointer<const char>(getNextArg32());
                    output += str ? str : "(null)";
                    break;
                }
                case 'p':
                    std::snprintf(buffer, sizeof(buffer), "0x%08x", getNextArg32());
                    output += buffer;
                    break;
                case 'f': case 'F': case 'g': case 'G': case 'e': case 'E': {
                    double val = 0.0;
                    if (nextFPR <= 13) {
                        val = state->fpr[nextFPR++];
                        advanceGPRSpace(2);
                    } else {
                        uint32_t* slotLow = ToPointer<uint32_t>(stackPtr + stackOffset);
                        uint32_t* slotHigh = ToPointer<uint32_t>(stackPtr + stackOffset + 4);
                        if (slotLow && slotHigh) {
                            uint64_t rawDouble = (static_cast<uint64_t>(Common::CF::BigToHost<uint32_t>::Swap(*slotLow)) << 32) | 
                                                 Common::CF::BigToHost<uint32_t>::Swap(*slotHigh);
                            std::memcpy(&val, &rawDouble, sizeof(double));
                        }
                        stackOffset += 8;
                        advanceGPRSpace(2);
                    }
                    std::snprintf(buffer, sizeof(buffer), specifier == 'f' ? "%f" : "%g", val);
                    output += buffer;
                    break;
                }
                default:
                    output += '%';
                    output += specifier;
                    break;
            }
            i++;
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
        currentArgPtr += 4; // Avança 4 bytes na stack da VM
        return ptr ? Common::CF::BigToHost<uint32_t>::Swap(*ptr) : 0;
    };

    std::string output;
    size_t i = 0;
    size_t len = std::strlen(format);

    while (i < len)
    {
        if (format[i] == '%' && i + 1 < len)
        {
            i++; 
            if (format[i] == '%') { output += '%'; i++; continue; }

            while (i < len && (format[i] == '.' || (format[i] >= '0' && format[i] <= '9') || format[i] == '-')) i++;
            if (i >= len) break;

            char specifier = format[i];
            char buffer[256];

            switch (specifier)
            {
                case 'd': case 'i':
                    std::snprintf(buffer, sizeof(buffer), "%d", static_cast<int32_t>(getNextArg32()));
                    output += buffer;
                    break;
                case 'u':
                    std::snprintf(buffer, sizeof(buffer), "%u", getNextArg32());
                    output += buffer;
                    break;
                case 'x': case 'X':
                    std::snprintf(buffer, sizeof(buffer), specifier == 'x' ? "%x" : "%X", getNextArg32());
                    output += buffer;
                    break;
                case 'c':
                    output += static_cast<char>(getNextArg32() & 0xFF);
                    break;
                case 's': {
                    const char* str = ToPointer<const char>(getNextArg32());
                    output += str ? str : "(null)";
                    break;
                }
                case 'p':
                    std::snprintf(buffer, sizeof(buffer), "0x%08x", getNextArg32());
                    output += buffer;
                    break;
                case 'f': case 'F': case 'g': case 'G': case 'e': case 'E': {
                    // Na stack de varargs crua de 32-bit do PowerPC, os doubles ocupam 8 bytes alinhados
                    double val = 0.0;
                    uint32_t low = getNextArg32();
                    uint32_t high = getNextArg32();
                    uint64_t rawDouble = (static_cast<uint64_t>(low) << 32) | high;
                    std::memcpy(&val, &rawDouble, sizeof(double));
                    
                    std::snprintf(buffer, sizeof(buffer), specifier == 'f' ? "%f" : "%g", val);
                    output += buffer;
                    break;
                }
                default:
                    output += '%';
                    output += specifier;
                    break;
            }
            i++;
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib___assertprint(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib___DebugMallocHeap(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib___GetTrapType(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib___growFileTable(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib___NumToolboxTraps(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib___RestoreInitialCFragWorld(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib___RevertCFragWorld(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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


	void StdCLib___vec_longjmp(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib___vec_setjmp(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__addDevHandler(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__badPtr(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__Bogus(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__BreakPoint(StdCLib::Globals* globals, MachineState* state)
	{
		const char* reason = ToPointer<char>(state->r3);
                printf("[ClassiX] Interrupted by %s\n", reason ? reason : "unknown");
		std::raise(SIGTRAP);
	}

	void StdCLib__bufsync(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__c2pstrcpy(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__coClose(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__coExit(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__coFAccess(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__coIoctl(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__coWrite(StdCLib::Globals* globals, MachineState* state)
	{
		// Força a escrita para o descritor 1 (stdout)
		state->r3 = 1; 
		StdCLib_write(globals, state);
	}

	void StdCLib__cvt(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__DoExitProcs(StdCLib::Globals* globals, MachineState* state)
{
    // Executa as rotinas de terminação por ordem inversa ao registo (LIFO)
    while (!globals->atExit.empty())
    {
        // Obtém a última rotina de transição registada (com byteswap já tratado pelo atexit)
        PEF::TransitionVector targetRoutine = globals->atExit.back();
        globals->atExit.pop_back();

        if (targetRoutine.pc != 0)
        {
            // Salva o estado atual crítico de fluxo para onde o interpretador deve regressar
            uint32_t originalPC = state->pc;
            uint32_t originalLR = state->lr;
            uint32_t originalR2 = state->r2; // O TOC original

            // Prepara a ABI da VM para executar a rotina clássica de limpeza
            state->pc = targetRoutine.pc;
            state->r2 = targetRoutine.toc;  // Atualiza o Table of Contents da biblioteca alvo
            
            // Definimos o Link Register (LR) para uma armadilha ou endereço de retorno nulo.
            // Isto força o interpretador virtual a parar a execução assim que a subrotina
            // clássica fizer o desvio de retorno ('blr').
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__doscan(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__exit(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__faccess(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__filbuf(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__findiop(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__fsFAccess(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__fsIoctl(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__fsRead(StdCLib::Globals* globals, MachineState* state)
	{
		// Encaminha diretamente para a implementação POSIX nativa segura
		StdCLib_read(globals, state);
	}

	void StdCLib__FSSpec2Path(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__fsWrite(StdCLib::Globals* globals, MachineState* state)
	{
		// Encaminha diretamente para a implementação POSIX nativa segura
		StdCLib_write(globals, state);
	}

	void StdCLib__GetAliasInfo(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__getDevHandler(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__getIOPort(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__memchr(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__memcpy(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__ResolveFileAlias(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__rmemcpy(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__RTExit(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__RTInit(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__SA_DeletePtr(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__SA_GetPID(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__SA_SetPtrSize(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__syClose(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__syFAccess(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__syIoctl(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__wrtchk(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib__xflsbuf(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_abort(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_abs(StdCLib::Globals* globals, MachineState* state)
	{
		int32_t val = static_cast<int32_t>(state->r3);
		state->r3 = static_cast<int32_t>(std::abs(val));
	}

	void StdCLib_access(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_bsearch(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_creat(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_dup(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_ecvt(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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

	#include <unistd.h> // Garante que está incluído para a função access()

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
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_ferror(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_fflush(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_fgetc(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_fputs(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_fread(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_fsetpos(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_FSMakeFSSpec_Long(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_FSp_creat(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_FSp_faccess(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_FSp_fopen(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_FSp_freopen(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_FSp_fsetfileinfo(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_FSp_open(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_FSp_remove(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_FSp_rename(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_FSp_unlink(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_FSSpec2Path_Long(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_ftell(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_fwrite(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_getc(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_getchar(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_gets(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
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


	oid StdCLib_labs(StdCLib::Globals* globals, MachineState* state)
	{
		int32_t val = static_cast<int32_t>(state->r3);
		state->r3 = static_cast<int32_t>(std::labs(val));
	}

	void StdCLib_ldiv(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_localeconv(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_mbstowcs(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_mbtowc(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_PLpos(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_PLstrcat(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_PLstrchr(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_PLstrcmp(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_PLstrcpy(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_PLstrlen(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_PLstrncat(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_PLstrncmp(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_PLstrncpy(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_PLstrpbrk(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_PLstrrchr(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_PLstrspn(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_PLstrstr(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_putchar(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_raise(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_rand(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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

	oid StdCLib_realloc(StdCLib::Globals* globals, MachineState* state)
	{
		uint32_t p_vmAddress = state->r3;
		uint32_t newSize = state->r4;

		if (p_vmAddress == 0)
		{
			// Se o ponteiro original for NULL, realloc funciona exatamente como um malloc
			state->r3 = p_vmAddress; // r3 temporário para o argumento
			state->r3 = newSize;     // ajusta r3 para o tamanho
			StdCLib_malloc(globals, state);
			return;
		}

		if (newSize == 0)
		{
			// Se o tamanho for zero, funciona como um free e retorna NULL
			StdCLib_free(globals, state);
			state->r3 = 0;
			return;
		}

		void* oldHostPtr = ToPointer<void>(p_vmAddress);
		void* newHostPtr = globals->allocator.Allocate(newSize, 8);

		if (newHostPtr == nullptr)
		{
			globals->scalars.errno_ = ENOMEM;
			state->r3 = 0; // Falha, mas o bloco original em oldHostPtr continua válido
			return;
		}

		if (oldHostPtr != nullptr)
		{
			// Nota: Como o allocator básico pode não expor o tamanho antigo facilmente,
			// uma abordagem conservadora segura é copiar o menor valor entre o novo tamanho 
			// ou assumir o limite seguro que não cause segfault. Se o teu allocator 
			// tiver uma função GetSize(oldHostPtr), usa-a aqui.
			std::memcpy(newHostPtr, oldHostPtr, newSize); 
			globals->allocator.Deallocate(oldHostPtr);
		}

		state->r3 = globals->allocator.ToIntPtr(newHostPtr);
		globals->scalars.errno_ = 0;
	}

	void StdCLib_remove(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_rename(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_scanf(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_setbuf(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_setenv(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_setlocale(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_setvbuf(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_signal(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
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
		state->r3 = = static_cast<uint32_t>(res);
	}

	void StdCLib_strerror(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_system(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_tmpnam(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_ungetc(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_unlink(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
		state->r3 = ptr ? globals->allocator.ToIntPtr(ptr) : 

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
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_vsprintf(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
	}

	void StdCLib_wcstombs(StdCLib::Globals* globals, MachineState* state)
	{
		throw PPCVM::NotImplementedException(__func__);
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
