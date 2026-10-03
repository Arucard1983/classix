//
// FSM.cpp
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

#include <cstdint>
#include <cstring>
#include "Prototypes.h"
#include "CarbonLib.h"

// Constantes históricas do File Manager e FSM
constexpr int16_t noErr    = 0;
constexpr int16_t paramErr = -3;
constexpr int16_t nsvErr   = -35; // No Such Volume

// Estruturas de controle simplificadas para simulação em Big-Endian no host
struct MacVCB {
	Common::UInt32 vcbLink;
	Common::Int16  vcbFlags;
	Common::UInt16 vcbMacVersion;
	Common::Int16  vcbRefNum;    // Identificador numérico do Volume (ex: -1)
	Common::UInt32 vcbDrvNum;
	Common::Int16  vcbDRefNum;   // Identificador do Driver (.AppleCD seria negativo)
	uint8_t        vcbVN[28];    // Nome do Volume em Pascal String (ex: "\x0fDiablo II Disc")
};

// Identificador fictício para o nosso volume de CD camuflado
constexpr int16_t kFakeCDVolRefNum = -1;

void CarbonLib_GetFSInfo(CarbonLib::Globals* globals, MachineState* state)
{
	// r3: Número de índice do sistema de arquivos ou seletor
	// r4: vRefNum ou Drive Number
	// r5: Ponteiro para a estrutura de informações a preencher
	
	uint32_t infoPtrAddr = state->r5;
	if (infoPtrAddr == 0) {
		state->r3 = static_cast<uint32_t>(paramErr);
		return;
	}

	// Preenchemos com noErr (0) e dizemos ao jogo que o sistema de arquivos 
	// suporta atributos de CD-ROM (como leitura estrita ISO9660/HFS).
	state->r3 = static_cast<uint32_t>(noErr);
}

void CarbonLib_InformFFS(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = static_cast<uint32_t>(noErr); // No-Op de handshake bem-sucedido
}

void CarbonLib_InformFSM(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = static_cast<uint32_t>(noErr); // No-Op de handshake bem-sucedido
}

void CarbonLib_InstallFS(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = static_cast<uint32_t>(paramErr); // Recusa instalação de drivers brutos de FS clássicos
}

void CarbonLib_RemoveFS(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = static_cast<uint32_t>(paramErr);
}

void CarbonLib_SetFSInfo(CarbonLib::Globals* globals, MachineState* state)
{
	// Bloqueia tentativas de injetar modificações de escrita nas tabelas do kernel
	state->r3 = static_cast<uint32_t>(paramErr);
}

void CarbonLib_UTAddFCBToSearchList(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTAddNewVCB(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTAdjustEOF(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTAllocateFCB(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTAllocateVCB(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTAllocateWDCB(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTBlockInFQHashP(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTCacheReadIP(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTCacheWriteIP(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTCheckDirBusy(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTCheckFCB(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTCheckFileModifiable(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = static_cast<uint32_t>(noErr); // Permite o fluxo normal de leitura
}

void CarbonLib_UTCheckFileRefNum(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = static_cast<uint32_t>(noErr);
}

void CarbonLib_UTCheckForkPermissions(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = static_cast<uint32_t>(noErr);
}

void CarbonLib_UTCheckPermission(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = static_cast<uint32_t>(noErr);
}

void CarbonLib_UTCheckVolModifiable(CarbonLib::Globals* globals, MachineState* state)
{
	// Devolve um erro de escrita porque leitores de CD/DVD originais são estritamente Read-Only.
	// É exatamente este comportamento que muitas proteções validam!
	state->r3 = static_cast<uint32_t>(-1ULL);
}

void CarbonLib_UTCheckVolOffline(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0; // Devolve 0 (O volume está ONLINE e pronto a ler)
}

void CarbonLib_UTCheckVolRefNum(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = static_cast<uint32_t>(noErr);
}

void CarbonLib_UTCheckWDRefNum(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = static_cast<uint32_t>(noErr);
}

void CarbonLib_UTDetermineVol(CarbonLib::Globals* globals, MachineState* state)
{
	// r3: vRefNum ou ponteiro fornecido pela aplicação para identificar o volume
	// Devolve o ponteiro para o VCB correspondente em r3.
	
	// Jogos com DRM varrem esta rotina para obter o ponteiro da tabela VCB interna.
	// Simulamos e devolvemos um endereço de sucesso estável.
	try {
		static uint32_t fakeVCBAddr = 0;
		if (fakeVCBAddr == 0) {
			MacVCB* vcb = globals->allocator.Allocate<MacVCB>("Fake CD-ROM VCB Table");
			vcb->vcbRefNum = Common::Int16(kFakeCDVolRefNum);
			vcb->vcbDRefNum = Common::Int16(-5); // Histórico do .AppleCD driver
			
			// Configura o nome do volume simulado estilo Pascal String ("Diablo II Play Disc")
			const char* discName = "Diablo II Play Disc";
			size_t len = std::strlen(discName);
			vcb->vcbVN[0] = static_cast<uint8_t>(len);
			std::memcpy(&vcb->vcbVN[1], discName, len);
			
			fakeVCBAddr = globals->allocator.ToIntPtr(vcb);
		}
		state->r3 = fakeVCBAddr;
	} catch (...) {
		state->r3 = 0;
	}
}

void CarbonLib_UTDisposeVCB(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTEjectVol(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTFindDrive(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTFlushCache(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = static_cast<uint32_t>(noErr); // O Linux já lida com o flush de ficheiros nativamente.
}

void CarbonLib_UTGetBlock(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTGetDefaultVol(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTGetForkControlBlockSize(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 128; // Tamanho padrão histórico de um FCB na RAM
}

void CarbonLib_UTGetPathComponentName(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTIndexFCB(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTLocateFCB(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTLocateFCBInSearchList(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTLocateNextFCB(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTLocateNextVCB(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTLocateVCBByName(CarbonLib::Globals* globals, MachineState* state)
{
	// r3: Ponteiro para a Pascal String com o nome do volume
	// Devolve o VCB se o nome bater certo.
	uint32_t nameAddr = state->r3;
	if (nameAddr == 0) {
		state->r3 = 0;
		return;
	}

	const uint8_t* pStr = globals->allocator.ToPointer<uint8_t>(nameAddr);
	uint8_t len = pStr[0];
	
	// Se o jogo procurar explicitamente por "Diablo" ou qualquer string válida, aceitamos
	if (len > 0) {
		CarbonLib_UTDetermineVol(globals, state);
	} else {
		state->r3 = 0;
	}
}

void CarbonLib_UTLocateVCBByRefNum(CarbonLib::Globals* globals, MachineState* state)
{
	// r3: vRefNum procurado. Mapeia diretamente para a nossa rotina de camuflagem acima.
	CarbonLib_UTDetermineVol(globals, state);
}

void CarbonLib_UTMarkDirty(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTParsePathname(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTReleaseBlock(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTReleaseFCB(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTReleaseWDCB(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTRemoveFCBFromSearchList(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTResolveFCB(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTResolveFileRefNum(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTResolveWDCB(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTSetDefaultVol(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTTrashBlocks(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTTrashFileBlocks(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTTrashVolBlocks(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTVolCacheReadIP(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

void CarbonLib_UTVolCacheWriteIP(CarbonLib::Globals* globals, MachineState* state)
{
	state->r3 = 0;
}

