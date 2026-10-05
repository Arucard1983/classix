//
// MacErrors.cpp
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

#include <iostream>
#include <iomanip>
#include <cstdlib>
#include "Prototypes.h"

void CarbonLib_SysError(CarbonLib::Globals* globals, MachineState* state)
{
	int16_t errorID = static_cast<int16_t>(state->r3);
	std::string errorDesc;

	// Tradução rápida dos códigos de bomba clássicos
	switch (errorID) {
		case 1:  errorDesc = "Bus Error (Acesso inválido à memória)"; break;
		case 2:  errorDesc = "Address Error (Alinhamento de endereço inválido)"; break;
		case 3:  errorDesc = "Illegal Instruction (Instrução PPC inválida)"; break;
		case 4:  errorDesc = "Zero Divide (Exceção numérica)"; break;
		case 9:  errorDesc = "Line 1010 Trap (Tentativa ilegal de usar A-Trap 68k)"; break;
		case 25: errorDesc = "Memory Manager Error (Heap de memória corrompido)"; break;
		case 28: errorDesc = "Stack Overflow (Estouro da pilha de execução)"; break;
		default: errorDesc = "Erro de Sistema Desconhecido."; break;
	}

	// Criar a mensagem de texto estruturada que vai ser enviada para a UI Cocoa
	std::stringstream uiMessage;
	uiMessage << "O emulador ClassiC detetou um System Panic (Bomba Mac OS)\n\n"
	          << "ID do Erro: " << errorID << "\n"
	          << "Descrição: " << errorDesc << "\n\n"
	          << "A aplicação será encerrada para proteger o estado do host.";

	// Fazer o dump detalhado no terminal para o programador (Darling debug)
	std::cerr << "\n*** MAC OS SYSTEM PANIC (SysError " << errorID << ") ***" << std::endl;
	std::cerr << errorDesc << std::endl;
	// Nota: Substitui o 'state->pc' pelo registo de programa correto do teu emulador
	// state->lr costuma ser útil aqui para saber quem chamou a função com falha
	std::cerr << "LR (Link Register): 0x" << std::hex << state->lr << std::endl; 

	// ENGENHARIA SURREAL: Disparar uma mensagem IPC para o Cocoa/Darling abrir um popup nativo!
	// Assumindo que adicionas uma mensagem 'IPCMessage::DisplayFatalAlert' ao teu enum
	globals->uiChannel->PerformAction<void>(CarbonLib::IPCMessage::DisplayFatalAlert, uiMessage.str());

	// Terminar a aplicação de forma limpa pelo canal IPC do Félix
	globals->uiChannel->PerformAction<void>(CarbonLib::IPCMessage::TerminateApplication);
	std::exit(EXIT_FAILURE);
}

