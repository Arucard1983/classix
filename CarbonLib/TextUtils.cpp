//
// TextUtils.cpp
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

#include <cstring>
#include <algorithm>
#include <cctype>
#include <CoreFoundation/CoreFoundation.h>
#include "Prototypes.h"
#include "CarbonLib.h"

void CarbonLib_C2PStr(CarbonLib::Globals* globals, MachineState* state)
{
	// Converte uma C-String (r3) para Pascal String (no próprio buffer r3 ou destino)
	// No Carbon PPC, r3 aponta para a C-String de origem/destino.
	uint32_t cStrAddr = state->r3;
	if (cStrAddr == 0) return;

	// Como a conversão é in-place ou assume espaço, lemos a string C
	const char* src = globals->allocator.ToPointer<char>(cStrAddr);
	size_t len = std::strlen(src);
	if (len > 255) len = 255; // Limite estrito de Str255

	// Para evitar sobreposição ao mover dados para a direita, 
	// copiamos de trás para a frente ou usamos um buffer temporário.
	char temp[256];
	std::memcpy(temp, src, len);

	// Atualiza o buffer na memória da VM
	uint8_t* dst = globals->allocator.ToPointer<uint8_t>(cStrAddr);
	dst[0] = static_cast<uint8_t>(len); // Comprimento no primeiro byte
	std::memcpy(dst + 1, temp, len);

	// Devolve o mesmo ponteiro em r3
	state->r3 = cStrAddr;
}

void CarbonLib_FindScriptRun(CarbonLib::Globals* globals, MachineState* state)
{
	// r3: Ponteiro para o início do texto
	// r4: Tamanho do texto (long)
	// r5: Ponteiro para o tamanho consumido (retorno)
	
	// No Mac OS clássico, isto discriminava blocos de texto que mudavam de alfabeto (ex: Romano para Kanji).
	// No Darling/Carbon moderno, assumimos tudo como um único bloco contínuo (script padrão).
	uint32_t textLength = state->r4;
	uint32_t consumedPtrAddr = state->r5;

	if (consumedPtrAddr && globals->allocator.IsValidAddress(consumedPtrAddr, sizeof(uint32_t))) {
		uint32_t* consumed = globals->allocator.ToPointer<uint32_t>(consumedPtrAddr);
		*consumed = textLength; // Consumiu o texto todo como um único script run
	}

	state->r3 = 0; // Devolve o ScriptCode base (0 = smRoman)
}

void CarbonLib_FindWord(CarbonLib::Globals* globals, MachineState* state)
{
	// Versão mais antiga/alternativa de deteção de palavras.
	// r3: Ponteiro para o texto
	// r4: Tamanho do texto (int16_t)
	// r5: Offset atual (int16_t)
	// r6: Booleano (fwdDirection)
	// r7: Ponteiro para rotina de quebra (ou nulo)
	// r8: Ponteiro para a OffsetTable de destino
	
	int16_t textLength = static_cast<int16_t>(state->r4);
	uint32_t offsetTableAddr = state->r8;

	if (offsetTableAddr && globals->allocator.IsValidAddress(offsetTableAddr, 4)) {
		int16_t* offsetTable = globals->allocator.ToPointer<int16_t>(offsetTableAddr);
		offsetTable[0] = 0;
		offsetTable[1] = textLength;
	}
}

void CarbonLib_FindWordBreaks(CarbonLib::Globals* globals, MachineState* state)
{
	// r3: Ponteiro para o início do texto
	// r4: Posição/Offset inicial do caractere (int16_t)
	// r5: Tamanho total do texto (int16_t)
	// r6: Booleano (leadingEdge)
	// r7: Ponteiro para a estrutura OffsetTable (onde devolve os limites da palavra)
	// r8: Código do script (ScriptCode)
	
	// Esta função procurava os limites de uma palavra para quebras de linha ou seleção (duplo clique).
	// Como stub nativo seguro para evitar loops em caixas de texto (TE/MLTE):
	// Simulamos que o texto inteiro é uma única palavra ou que não há quebra intermédia.
	uint32_t offsetTableAddr = state->r7;
	int16_t textLength = static_cast<int16_t>(state->r5);

	if (offsetTableAddr && globals->allocator.IsValidAddress(offsetTableAddr, 4)) {
		int16_t* offsetTable = globals->allocator.ToPointer<int16_t>(offsetTableAddr);
		// Mapeamento Big-Endian simplificado para o host:
		offsetTable[0] = 0;           // Início da palavra (offset 0)
		offsetTable[1] = textLength;  // Fim da palavra (fim do bloco)
	}
}

void CarbonLib_GetIndString(CarbonLib::Globals* globals, MachineState* state)
{
	// r3: ID do recurso de lista de strings 'STR#' (int16_t)
	// r4: Índice da string dentro da lista (int16_t, baseado em 1!)
	// r5: Ponteiro para o buffer Str255 de destino (Pascal String)
	int16_t strListID = static_cast<int16_t>(state->r3);
	int16_t index     = static_cast<int16_t>(state->r4);
	uint32_t dstAddr  = state->r5;

	if (dstAddr == 0) return;

	uint8_t* outPascalStr = globals->allocator.ToPointer<uint8_t>(dstAddr);
	
	// Quando o Resource Manager estiver pronto, lerá o recurso 'STR#' e extrairá a string[index].
	// Como stub seguro, inicializamos o buffer como uma Pascal String vazia (tamanho 0)
	outPascalStr[0] = 0; 
}

void CarbonLib_GetString(CarbonLib::Globals* globals, MachineState* state)
{
	/ r3 contém o ID do recurso 'STR ' (int16_t)
	// Devolve um Handle (StringHandle) em r3 para a Pascal String correspondente.
	int16_t stringResID = static_cast<int16_t>(state->r3);

	// Quando o seu Resource Manager estiver pronto, a lógica será:
	// state->r3 = CarbonLib_GetResource(globals, 'STR ', stringResID);
	
	// Como stub seguro e transparente (evita crashes no arranque de apps):
	// Retornamos um Handle vazio legítimo (Master Pointer a apontar para 0)
	try {
		uint32_t* masterPointer = globals->allocator.Allocate<uint32_t>("CarbonLib GetString Stub Handle");
		*masterPointer = 0; // Handle vazio
		state->r3 = globals->allocator.ToIntPtr(masterPointer);
	} catch (...) {
		state->r3 = 0;
	}
}

void CarbonLib_LowercaseText(CarbonLib::Globals* globals, MachineState* state)
{
	// Converte um bloco de texto corrido (r3 = ponteiro, r4 = tamanho) para minúsculas
	uint32_t textAddr = state->r3;
	size_t length = state->r4;
	if (textAddr == 0 || length == 0) return;

	char* text = globals->allocator.ToPointer<char>(textAddr);
	for (size_t i = 0; i < length; ++i) {
		text[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
	}
}

void CarbonLib_LowerText(CarbonLib::Globals* globals, MachineState* state)
{
	CarbonLib_LowercaseText(globals, state);
}

void CarbonLib_LwrText(CarbonLib::Globals* globals, MachineState* state)
{
	CarbonLib_LowercaseText(globals, state);
}

void CarbonLib_Munger(CarbonLib::Globals* globals, MachineState* state)
{
	// Parâmetros vindos dos registadores PPC:
	// state->r3: Handle da string/bloco de destino (h)
	// state->r4: Offset inicial no destino para começar a operação (offset)
	// state->r5: Ponteiro para a string de pesquisa (ptr1)
	// state->r6: Tamanho da string de pesquisa (len1)
	// state->r7: Ponteiro para a string de substituição (ptr2)
	// state->r8: Tamanho da string de substituição (len2)

	uint32_t handleAddr = state->r3;
	uint32_t offset     = state->r4;
	uint32_t ptr1Addr   = state->r5;
	uint32_t len1       = state->r6;
	uint32_t ptr2Addr   = state->r7;
	uint32_t len2       = state->r8;

	// 1. Validação do Handle de destino
	if (handleAddr == 0 || !globals->allocator.IsValidAddress(handleAddr, sizeof(uint32_t))) {
		state->r3 = static_cast<uint32_t>(-1); // Devolve erro em r3 se o Handle for inválido
		return;
	}

	uint32_t* masterPointer = globals->allocator.ToPointer<uint32_t>(handleAddr);
	uint32_t dataAddr = *masterPointer;

	size_t dstSize = 0;
	if (dataAddr != 0) {
		if (auto details = globals->allocator.GetDetails(dataAddr)) {
			dstSize = details->GetSize();
		}
	}

	// Proteção contra offsets fora dos limites do buffer
	if (offset > dstSize) {
		offset = dstSize;
	}

	// 2. Extração dos buffers da memória emulada para vetores locais do host
	uint8_t* dstData = (dataAddr != 0) ? globals->allocator.ToPointer<uint8_t>(dataAddr) : nullptr;
	std::vector<uint8_t> targetBuffer(dstData, dstData + dstSize);

	const uint8_t* ptr1 = (ptr1Addr != 0 && len1 > 0) ? globals->allocator.ToPointer<uint8_t>(ptr1Addr) : nullptr;
	const uint8_t* ptr2 = (ptr2Addr != 0 && len2 > 0) ? globals->allocator.ToPointer<uint8_t>(ptr2Addr) : nullptr;

	// 3. Execução da Lógica Matemática do Munger
	bool found = false;
	size_t matchIndex = offset;

	if (ptr1 != nullptr) {
		// Modo Pesquisa: Tenta localizar ptr1 dentro do targetBuffer a partir de 'offset'
		if (offset + len1 <= targetBuffer.size()) {
			auto it = std::search(
				targetBuffer.begin() + offset, targetBuffer.end(),
				ptr1, ptr1 + len1
			);
			if (it != targetBuffer.end()) {
				found = true;
				matchIndex = std::distance(targetBuffer.begin(), it);
			}
		}
		
		// Se a string de pesquisa foi fornecida mas não foi encontrada, a operação aborta aqui
		if (!found) {
			state->r3 = static_cast<uint32_t>(-1); // Retorna sinal de "não encontrado" (-1)
			return;
		}
	} else {
		// Se ptr1 for nulo, a operação acontece exatamente na posição ditada por 'offset'
		found = true;
		matchIndex = offset;
		len1 = 0; // Sem substituição de padrão, apenas inserção ou apagamento puro
	}

	// 4. Modificação do Buffer Local (Inserção, Remoção ou Substituição)
	if (ptr2 != nullptr) {
		// Temos dados para inserir ou substituir
		if (len1 > 0) {
			// Substituição: Remove a ocorrência antiga
			targetBuffer.erase(targetBuffer.begin() + matchIndex, targetBuffer.begin() + matchIndex + len1);
		}
		// Insere os novos bytes na posição correta
		targetBuffer.insert(targetBuffer.begin() + matchIndex, ptr2, ptr2 + len2);
		
		// Devolve o offset imediatamente após o texto inserido
		state->r3 = static_cast<uint32_t>(matchIndex + len2);
	} else {
		// ptr2 é nulo
		if (ptr1 != nullptr) {
			// Apagamento puro: Se ptr1 existia e ptr2 é nulo, removemos a string encontrada
			targetBuffer.erase(targetBuffer.begin() + matchIndex, targetBuffer.begin() + matchIndex + len1);
			state->r3 = static_cast<uint32_t>(matchIndex);
		} else {
			// Pesquisa pura: Ambos nulos, apenas devolvemos o offset onde a ação teria ocorrido
			state->r3 = static_cast<uint32_t>(matchIndex);
			return; 
		}
	}

	// 5. Atualização da Memória da VM e Realocação do Handle (Estilo Carbon)
	try {
		size_t newSize = targetBuffer.size();
		
		if (newSize == 0) {
			if (dataAddr != 0) {
				globals->allocator.Deallocate(globals->allocator.ToPointer<void>(dataAddr));
			}
			*masterPointer = 0;
		} else {
			// Aloca um novo bloco de memória com o tamanho atualizado
			uint8_t* newDataBlock = globals->allocator.Allocate("CarbonLib Munger Resized Data", newSize);
			if (newSize > 0) {
				std::memcpy(newDataBlock, targetBuffer.data(), newSize);
			}
			
			// Liberta o bloco antigo com segurança
			if (dataAddr != 0) {
				globals->allocator.Deallocate(globals->allocator.ToPointer<void>(dataAddr));
			}
			
			// Atualiza o Master Pointer para apontar estável para a nova área mapeada por mmap
			*masterPointer = globals->allocator.ToIntPtr(newDataBlock);
		}
	} catch (...) {
		state->r3 = static_cast<uint32_t>(-1); // Fallback em caso de falha catastrófica de alocação
	}
}

void CarbonLib_NewString(CarbonLib::Globals* globals, MachineState* state)
{
	// Cria uma nova Pascal String no Heap a partir de uma Pascal String de origem (r3)
	// Devolve o Handle (Master Pointer) em r3.
	uint32_t srcPStrAddr = state->r3;
	if (srcPStrAddr == 0) {
		state->r3 = 0;
		return;
	}

	const uint8_t* src = globals->allocator.ToPointer<uint8_t>(srcPStrAddr);
	uint8_t len = src[0];
	size_t totalSize = len + 1; // Dados + byte de tamanho

	try {
		// Aloca o bloco de dados da string
		uint8_t* dataBlock = globals->allocator.Allocate("CarbonLib String Data", totalSize);
		std::memcpy(dataBlock, src, totalSize);

		// Aloca o Master Pointer (Handle)
		uint32_t* masterPointer = globals->allocator.Allocate<uint32_t>("CarbonLib String Handle");
		*masterPointer = globals->allocator.ToIntPtr(dataBlock); // Nota: Converter para BigEndian se a VM o exigir

		state->r3 = globals->allocator.ToIntPtr(masterPointer);
	} catch (...) {
		state->r3 = 0; // memFullErr gerido pelo MemError se necessário
	}
}

void CarbonLib_NFindWord(CarbonLib::Globals* globals, MachineState* state)
{
	// Parâmetros do registador PPC:
	// state->r3: Ponteiro para o início do texto (textPtr)
	// state->r4: Tamanho total do bloco de texto (textLength - long)
	// state->r5: Posição/Offset atual do caractere (charPosition - long)
	// state->r6: Booleano (leadingEdge)
	// state->r7: Ponteiro para a estrutura BreakTable (ou NULL)
	// state->r8: Ponteiro para a OffsetTable de destino (2 inteiros de 16-bit)

	uint32_t textLength = state->r4;
	uint32_t offsetTableAddr = state->r8;

	// Validação básica do ponteiro da tabela de saída onde a app espera ler os offsets
	if (offsetTableAddr && globals->allocator.IsValidAddress(offsetTableAddr, 4)) {
		int16_t* offsetTable = globals->allocator.ToPointer<int16_t>(offsetTableAddr);
		
		// Como stub nativo estável e seguro:
		// Simulamos que a "palavra" selecionada engloba o bloco de texto inteiro.
		// Isto impede loops infinitos em motores de rendering como o TextEdit/MLTE.
		offsetTable[0] = 0;                                       // Início da palavra (Offset 0)
		offsetTable[1] = static_cast<int16_t>(textLength);        // Fim da palavra (Offset total)
	}

	// Devolve 0 (noErr) ou o ScriptCode correspondente (0 = smRoman por padrão)
	state->r3 = 0; 
}

void CarbonLib_P2CStr(CarbonLib::Globals* globals, MachineState* state)
{
	// Converte uma Pascal String (r3) para C-String in-place.
	uint32_t pStrAddr = state->r3;
	if (pStrAddr == 0) return;

	const uint8_t* src = globals->allocator.ToPointer<uint8_t>(pStrAddr);
	uint8_t len = src[0]; // Primeiro byte é o tamanho

	// Move os dados 1 byte para a esquerda para apagar o indicador de tamanho
	uint8_t* dst = globals->allocator.ToPointer<uint8_t>(pStrAddr);
	std::memmove(dst, src + 1, len);
	dst[len] = '\0'; // Adiciona o terminador nulo

	state->r3 = pStrAddr;
}

void CarbonLib_SetString(CarbonLib::Globals* globals, MachineState* state)
{
	// Altera o conteúdo de um Handle de string existente (r3) para uma nova Pascal String (r4)
	uint32_t handleAddr = state->r3;
	uint32_t srcPStrAddr = state->r4;
	if (handleAddr == 0 || srcPStrAddr == 0) return;

	uint32_t* masterPointer = globals->allocator.ToPointer<uint32_t>(handleAddr);
	uint32_t oldDataAddr = *masterPointer;

	const uint8_t* src = globals->allocator.ToPointer<uint8_t>(srcPStrAddr);
	uint8_t len = src[0];
	size_t newSize = len + 1;

	try {
		// Aloca novo bloco
		uint8_t* newDataBlock = globals->allocator.Allocate("CarbonLib SetString Data", newSize);
		std::memcpy(newDataBlock, src, newSize);

		// Liberta o antigo de forma segura
		if (oldDataAddr != 0) {
			globals->allocator.Deallocate(globals->allocator.ToPointer<void>(oldDataAddr));
		}

		*masterPointer = globals->allocator.ToIntPtr(newDataBlock);
	} catch (...) {
		// Falha silenciosa ou erro de memória
	}
}

void CarbonLib_StripDiacritics(CarbonLib::Globals* globals, MachineState* state)
{
	// state->r3: Ponteiro para o texto de origem/destino (modificação in-place)
	// state->r4: Tamanho do texto em bytes (int16_t)
	// state->r5: Código do script (ScriptCode - int16_t)
	uint32_t textAddr = state->r3;
	size_t length = static_cast<size_t>(static_cast<int16_t>(state->r4));

	if (textAddr == 0 || length == 0) return;

	char* text = globals->allocator.ToPointer<char>(textAddr);

	// Criamos uma CFString a partir do buffer emulada (usando MacRoman, o padrão da Toolbox)
	CFMutableStringRef cfStr = CFStringCreateMutable(kCFAllocatorDefault, length);
	CFStringAppendCString(cfStr, text, kCFStringEncodingMacRoman);

	// Aplica a transformação nativa do CoreFoundation para remover acentos e diacríticos
	CFStringTransform(cfStr, nullptr, kCFStringTransformStripDiacritics, false);

	// Devolve o texto transformado para o mesmo buffer da VM
	char cStr[512]; // Buffer temporário seguro para o host
	if (CFStringGetCString(cfStr, cStr, sizeof(cStr), kCFStringEncodingMacRoman)) {
		size_t newLen = std::strlen(cStr);
		size_t copySize = (newLen < length) ? newLen : length;
		std::memcpy(text, cStr, copySize);
		
		// Se a string resultante for menor, preenchemos o resto com espaços ou mantemos (estilo Classic)
		if (copySize < length) {
			std::memset(text + copySize, ' ', length - copySize);
		}
	}

	if (cfStr) CFRelease(cfStr);
}

void CarbonLib_StripText(CarbonLib::Globals* globals, MachineState* state)
{
	// No Carbon moderno, StripText é funcionalmente idêntica a StripDiacritics.
	CarbonLib_StripDiacritics(globals, state);
}

void CarbonLib_StripUpperText(CarbonLib::Globals* globals, MachineState* state)
{
	// Remove os diacríticos e, em seguida, converte o texto para maiúsculas.
	CarbonLib_StripDiacritics(globals, state);
	CarbonLib_UppercaseText(globals, state);
}

void CarbonLib_UppercaseStripDiacritics(CarbonLib::Globals* globals, MachineState* state)
{
	// Outra variação histórica mantida pela Apple que faz exatamente o mesmo: Maiúsculas + Strip.
	CarbonLib_StripDiacritics(globals, state);
	CarbonLib_UppercaseText(globals, state);
}

void CarbonLib_UppercaseText(CarbonLib::Globals* globals, MachineState* state)
{
	// Converte um bloco de texto corrido (r3 = ponteiro) com tamanho (r4 = bytes) para maiúsculas
	uint32_t textAddr = state->r3;
	size_t length = state->r4;
	if (textAddr == 0 || length == 0) return;

	char* text = globals->allocator.ToPointer<char>(textAddr);
	for (size_t i = 0; i < length; ++i) {
		text[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(text[i])));
	}
}

void CarbonLib_UpperString(CarbonLib::Globals* globals, MachineState* state)
{
	// Converte uma Pascal String (r3) para maiúsculas.
	// r4 contém um booleano (historicamente indicava se devia considerar diacríticos da ROM)
	uint32_t pStrAddr = state->r3;
	if (pStrAddr == 0) return;

	uint8_t* str = globals->allocator.ToPointer<uint8_t>(pStrAddr);
	uint8_t len = str[0];

	for (uint8_t i = 1; i <= len; ++i) {
		str[i] = static_cast<uint8_t>(std::toupper(str[i]));
	}
}

void CarbonLib_UpperText(CarbonLib::Globals* globals, MachineState* state)
{
	CarbonLib_UppercaseText(globals, state);
}

void CarbonLib_UprText(CarbonLib::Globals* globals, MachineState* state)
{
	CarbonLib_UppercaseText(globals, state);
}

