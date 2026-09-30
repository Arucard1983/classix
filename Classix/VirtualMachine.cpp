//
// VirtualMachine.cpp
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

#include "VirtualMachine.h"
#include <unordered_set>

// --- VARIÁVEIS GLOBAIS DE SUPORTE AO AMBIENTE DE THREADS MACH-O ---
Common::Allocator* g_macho_allocator = nullptr;
OSEnvironment::Managers* g_macho_managers = nullptr;
PPCVM::MachineState* g_macho_main_state = nullptr;

namespace Classix
{
	uint32_t ProgramControlHandle::RunSymbol(CFM::ResolvedSymbol& symbol)
	{
		auto threadMarker = vm.managers.ThreadManager().CreateExecutionMarker();
		
		auto vector = vm.allocator.ToPointer<const PEF::TransitionVector>(symbol.Address);
		BeginTransition(*vector);
		vm.state.lr = vm.allocator.ToIntPtr(vm.interpreter.GetEndAddress());
		vm.interpreter.Execute(vm.allocator.ToPointer<Common::UInt32>(pc));
		return vm.state.r3;
	}
	
	void ProgramControlHandle::BeginTransition(const PEF::TransitionVector &vector)
	{
		vm.state.r0 = 0;
		vm.state.r1 = vm.allocator.ToIntPtr(stackInfo.sp - 8);
		vm.state.r3 = vm.state.r27 = stackInfo.argc;
		vm.state.r4 = vm.state.r28 = vm.allocator.ToIntPtr(stackInfo.argv);
		vm.state.r5 = vm.state.r29 = vm.allocator.ToIntPtr(stackInfo.envp);
		
		vm.state.r2 = vector.TableOfContents;
		pc = vector.EntryPoint;
	}
	
	void ProgramControlHandle::StepInto()
	{
		auto threadMarker = vm.managers.ThreadManager().CreateExecutionMarker();
		const Common::UInt32* newPC = vm.interpreter.ExecuteOne(vm.allocator.ToPointer<Common::UInt32>(pc));
		pc = vm.allocator.ToIntPtr(newPC);
	}
	
	void ProgramControlHandle::StepOver()
	{
		auto threadMarker = vm.managers.ThreadManager().CreateExecutionMarker();
		
		Common::UInt32 word = *vm.allocator.ToPointer<Common::UInt32>(pc);
		PPCVM::Instruction inst = word.Get();
		if (inst.OPCD == 18 && inst.LK == 1)
		{
			uint32_t sp = vm.state.r1;
			uint32_t desiredPC = pc + 4;
			do
			{
				RunTo(desiredPC);
			} while (vm.state.r1 != sp);
		}
		else
			StepInto();
	}
	
	void ProgramControlHandle::RunTo(uint32_t address)
	{
		auto threadMarker = vm.managers.ThreadManager().CreateExecutionMarker();
		
		std::unordered_set<const void*> until = {vm.allocator.ToPointer<void>(address)};
		const Common::UInt32* eip = vm.allocator.ToPointer<Common::UInt32>(pc);
		eip = vm.interpreter.ExecuteUntil(eip, until);
		pc = vm.allocator.ToIntPtr(eip);
	}

    uint32_t MachOProgramControlHandle::RunFromAddress(uint32_t entryAddress)
    {
        auto threadMarker = vm.managers.ThreadManager().CreateExecutionMarker();
        
        // Inicializa o estado dos registadores da máquina para a ABI do Mach-O
        vm.state.r1 = vm.allocator.ToIntPtr(stackInfo.sp - 16); // Stack alignment
        vm.state.r3 = stackInfo.argc;
        vm.state.r4 = vm.allocator.ToIntPtr(stackInfo.argv);
        vm.state.r5 = vm.allocator.ToIntPtr(stackInfo.envp);
        
        // Define o endereço de retorno (LR) para o endereço de terminação do interpretador
        vm.state.lr = vm.allocator.ToIntPtr(vm.interpreter.GetEndAddress());
        
        // Executa o interpretador PPC a partir do Entry Point nativo do Mach-O
        uint32_t pc = entryAddress;
        vm.interpreter.Execute(vm.allocator.ToPointer<Common::UInt32>(pc));
        
        return vm.state.r3; // Retorno padrão em r3
    }

	
	MainStub::MainStub(VirtualMachine& vm, CFM::ResolvedSymbol mainSymbol)
	: vm(vm), mainSymbol(mainSymbol)
	{
		StackSize = Common::StackPreparator::DefaultStackSize;
	}
	
	uint32_t MainStub::operator()(const std::string& argv0)
	{
		return this->operator()(&argv0, (&argv0) + 1);
	}
	
	uint32_t MainStub::operator()(int argc, const char** argv)
	{
		return this->operator()(argv, argv + argc);
	}
	
	uint32_t MainStub::operator()(int argc, const char** argv, const char** envp)
	{
		const char** envpEnd = envp;
		while (*envpEnd != nullptr)
			envpEnd++;
		
		return this->operator()(argv, argv + argc, envp, envpEnd);
	}
	
	uint32_t MachOMainStub::operator()(int argc, const char** argv, const char** envp) 
	{
		// 1. Instanciar o gestor de controlo e preparar a stack virtual do Mach-O
		auto handle = Instantiate(argv, argv + argc, envp, envp ? envp : nullptr);

		// 2. CAPTURA DE CONTEXTO: Inicializa as referências globais que o MachOThreadHelper vai herdar
		g_macho_allocator  = &vm.allocator;
		g_macho_managers   = &vm.managers;
		g_macho_main_state = &vm.state;

		// 3. Despoleta a execução concorrente através do Entry Point detetado pelo Loader
		return handle.RunFromAddress(machOEntryPoint);
	}
	
	VirtualMachine::VirtualMachine(Common::Allocator& allocator, OSEnvironment::Managers& managers)
	: allocator(allocator), managers(managers), interpreter(allocator, state), pefResolver(allocator, fragmentManager)
	{
		AddLibraryResolver(pefResolver);
	}
	
	void VirtualMachine::AddLibraryResolver(CFM::LibraryResolver &resolver)
	{
		fragmentManager.LibraryResolvers.push_back(&resolver);
	}
	
	MainStub VirtualMachine::LoadMainContainer(const std::string &path)
	{
		if (!fragmentManager.LoadContainer(path))
			throw std::logic_error("Could not load specified container");
		
		auto resolver = fragmentManager.GetSymbolResolver(path);
		auto entryPoints = resolver->GetEntryPoints();
		for (const auto& entryPoint : entryPoints)
		{
			if (entryPoint.Name == CFM::SymbolResolver::MainSymbolName)
			{
				if (entryPoint.Universe == CFM::SymbolUniverse::PowerPC)
					return MainStub(*this, entryPoint);
				else
					throw std::logic_error("Container successfully resolved, but main symbol is not a PPC symbol");
			}
		}
		throw std::logic_error("Container does not contain a main symbol");
	}

    MachOMainStub VirtualMachine::LoadMachOContainer(const std::string& path)
    {
    // 1. Mapear o ficheiro binário Mach-O para a memória do host
    // Nota: Usamos a infraestrutura nativa do ClassiX para mapeamento de ficheiros
    auto mapping = std::make_shared<Common::FileMapping>(path);
    
    // 2. Instanciar o teu parser Mach-O passando o alocador e os limites do ficheiro
    // Isto vai validar o header PPC e processar os Load Commands (Segmentos, Dylibs, etc.)
    auto container = std::make_unique<MachO::MachOContainer>(allocator, mapping->begin(), mapping->end());
    
    // 3. Mapear os segmentos parsed pelo container para a memória virtual do PPCVM
    for (const auto& segment : container->GetSegments())
    {
        // Ignora segmentos que não têm dados ou tamanho virtual mapeável
        if (segment.vmSize == 0) continue;

        // Aloca espaço na memória virtual do Guest (PPC) respeitando o endereço desejado (vmAddr)
        // No ecossistema Mach-O, os segmentos pedem endereços virtuais fixos específicos
        void* guestTargetMem = allocator.MapVirtual(segment.vmAddr, segment.vmSize, segment.initProt);
        
        if (segment.dataPtr && segment.fileSize > 0)
        {
            // Copia os dados reais do ficheiro mapeado para o espaço de endereçamento do Guest PPC
            std::memcpy(guestTargetMem, segment.dataPtr, segment.fileSize);
        }
        
        // Se o tamanho na memória (vmSize) for maior que o tamanho no ficheiro (fileSize),
        // o restante deve ser preenchido com zeros (comportamento padrão de seções .bss)
        if (segment.vmSize > segment.fileSize)
        {
            size_t bssSize = segment.vmSize - segment.fileSize;
            uint8_t* bssPtr = static_cast<uint8_t*>(guestTargetMem) + segment.fileSize;
            std::memset(bssPtr, 0, bssSize);
        }
    }

    // 4. Extrair o Entry Point que o teu parser obteve através do LC_UNIXTHREAD
    uint32_t entryPoint = container->GetEntryPoint();
    if (entryPoint == 0)
    {
        throw std::runtime_error("[MachO Loader] Falha crítica: Nenhum Entry Point válido foi encontrado no binário.");
    }

    // 5. Retornar o Stub inicializador configurado com o endereço de entrada
    // O operador () deste stub irá despoletar a execução no interpretador PPCVM
    return MachOMainStub(*this, entryPoint);
  }
}
