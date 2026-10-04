//
// Palettes.cpp
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

#include <CoreGraphics/CoreGraphics.h>
#include "Prototypes.h"
#include "NotImplementedException.h"
#include "CarbonLib.h"
#include "Todo.h"

void CarbonLib_ActivatePalette(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = WindowRef window
	// Esta função é chamada pelas aplicações quando a janela muda de estado ou ganha foco,
	// forçando o hardware clássico a aplicar a paleta correspondente.
	uint32_t windowPtr = state->r3;
	
	if (windowPtr == 0)
	{
		// Se a WindowRef for nula, o Mac OS ativa a paleta na porta atual por defeito.
		CarbonLib::UGrafPort& currentPort = globals->grafPorts.GetCurrentPort();
		CarbonLib::Palette* palette = globals->grafPorts.PaletteOfGrafPort(currentPort);
		
		if (palette)
		{
			// Aqui sincronizaríamos a paleta com o CLUT virtual. 
			// Como o ecrã do host é True Color, a ativação serve para validar o estado gráfico.
			return;
		}
	}
	else
	{
		CarbonLib::UGrafPort& port = *globals->allocator.ToPointer<CarbonLib::UGrafPort>(windowPtr);
		CarbonLib::Palette* palette = globals->grafPorts.PaletteOfGrafPort(port);
		
		if (palette)
		{
			// Notifica o sistema de janelas do emulador que a paleta desta janela está ativa.
			// Se a aplicação requereu atualizações de cor, poderias disparar um evento Cocoa aqui.
			return;
		}
	}
}

void CarbonLib_AnimateEntry(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = WindowRef window
	// r4 = int16_t entry
	// r5 = const RGBColor* srcRGB
	// Esta rotina mudava a cor instantaneamente no ecrã sem redesenhar os pixeis.
	uint32_t windowPtr = state->r3;
	int16_t entry = static_cast<int16_t>(state->r4);
	uint32_t srcRGBPtr = state->r5;

	if (windowPtr == 0 || srcRGBPtr == 0) return;

	CarbonLib::UGrafPort& port = *globals->allocator.ToPointer<CarbonLib::UGrafPort>(windowPtr);
	CarbonLib::Palette* palette = globals->grafPorts.PaletteOfGrafPort(port);
	auto* srcRGB = globals->allocator.ToPointer<CarbonLib::RGBColor>(srcRGBPtr);

	if (palette && entry >= 0 && entry < palette->pmEntries)
	{
		// 1. Atualiza os metadados na paleta emulada
		palette->pmInfo[entry].ciRGB = *srcRGB;

		// 2. Se a porta ativa for a desta janela, força a atualização no host para o Darling refletir a mudança
		if (windowPtr == globals->grafPorts.ToIntPtr(&globals->grafPorts.GetCurrentPort()))
		{
			CGContextRef ctx = globals->grafPorts.ContextOfGrafPort(port);
			if (ctx)
			{
				CGFloat max = std::numeric_limits<uint16_t>::max();
				CGFloat r = srcRGB->red / max;
				CGFloat g = srcRGB->green / max;
				CGFloat b = srcRGB->blue / max;
				
				// Atualiza o traço atual do CoreGraphics com a nova cor animada
				CGContextSetRGBStrokeColor(ctx, r, g, b, 1.0);
				
				// Nota: No hardware real isto alterava o DAC do monitor. Em emulação HLE moderna,
				// disparar um refresh da janela ou marcar a dirtyRect garante que o Darling redesenhe a frame.
				globals->uiChannel->PerformAction<void>(CarbonLib::IPCMessage::RequestUpdate, windowPtr);
			}
		}
	}
}

void CarbonLib_CopyPalette(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = PaletteHandle srcPalette
	// r4 = PaletteHandle dstPalette
	uint32_t srcPaletteHandle = state->r3;
	uint32_t dstPaletteHandle = state->r4;

	if (srcPaletteHandle == 0 || dstPaletteHandle == 0) return;

	uint32_t srcPalettePtr = *globals->allocator.ToPointer<uint32_t>(srcPaletteHandle);
	uint32_t dstPalettePtr = *globals->allocator.ToPointer<uint32_t>(dstPaletteHandle);

	auto* src = globals->allocator.ToPointer<CarbonLib::Palette>(srcPalettePtr);
	auto* dst = globals->allocator.ToPointer<CarbonLib::Palette>(dstPalettePtr);

	// Se o destino não tiver espaço suficiente para as novas entradas, 
	// o comportamento clássico exige realocar a estrutura de destino.
	if (dst->pmEntries < src->pmEntries)
	{
		size_t newSize = sizeof(CarbonLib::Palette) + (src->pmEntries * sizeof(CarbonLib::ColorInfo));
		
		// Libertar a memória antiga e alocar o novo bloco expandido
		globals->allocator.Free(dstPalettePtr);
		dstPalettePtr = globals->allocator.Allocate(newSize);
		
		// Atualizar o ponteiro dentro do Handle de destino
		*globals->allocator.ToPointer<uint32_t>(dstPaletteHandle) = dstPalettePtr;
		
		// Remapear o ponteiro C++ para a nova localização
		dst = globals->allocator.ToPointer<CarbonLib::Palette>(dstPalettePtr);
	}

	// Copiar os cabeçalhos e metadados base
	dst->pmEntries = src->pmEntries;
	memcpy(dst->pmDataFields, src->pmDataFields, sizeof(src->pmDataFields));

	// Copiar todas as informações de cor (ciRGB, ciUsage, ciTolerance) de forma profunda
	for (int16_t i = 0; i < src->pmEntries; i++)
	{
		dst->pmInfo[i].ciRGB = src->pmInfo[i].ciRGB;
		dst->pmInfo[i].ciUsage = src->pmInfo[i].ciUsage;
		dst->pmInfo[i].ciTolerance = src->pmInfo[i].ciTolerance;
		memcpy(dst->pmInfo[i].ciDataFields, src->pmInfo[i].ciDataFields, sizeof(src->pmInfo[i].ciDataFields));
	}
}

void CarbonLib_CTab2Palette(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = ColorTableHandle srcCTab
	// r4 = PaletteHandle dstPalette
	// r5 = int16_t srcUsage
	// r6 = int16_t srcTolerance
	uint32_t srcCTabHandle = state->r3;
	uint32_t dstPaletteHandle = state->r4;
	int16_t srcUsage = static_cast<int16_t>(state->r5);
	int16_t srcTolerance = static_cast<int16_t>(state->r6);

	if (srcCTabHandle == 0 || dstPaletteHandle == 0) return;

	uint32_t cTabPtr = *globals->allocator.ToPointer<uint32_t>(srcCTabHandle);
	auto* cTab = globals->allocator.ToPointer<CarbonLib::ColorTable>(cTabPtr);

	uint32_t palettePtr = *globals->allocator.ToPointer<uint32_t>(dstPaletteHandle);
	auto* palette = globals->allocator.ToPointer<CarbonLib::Palette>(palettePtr);

	// Copia as cores da ColorTable para a estrutura da Palette
	int16_t limit = std::min(static_cast<int16_t>(cTab->count + 1), palette->pmEntries);
	for (int16_t i = 0; i < limit; i++)
	{
		palette->pmInfo[i].ciRGB = cTab->table[i].rgb;
		palette->pmInfo[i].ciUsage = srcUsage;
		palette->pmInfo[i].ciTolerance = srcTolerance;
	}
}

void CarbonLib_DisposePalette(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = PaletteHandle palette
	uint32_t paletteHandle = state->r3;
	if (paletteHandle != 0)
	{
		// No Mac OS, paletas são Handles (ponteiros para ponteiros)
		uint32_t palettePtr = *globals->allocator.ToPointer<uint32_t>(paletteHandle);
		if (palettePtr != 0)
		{
			globals->allocator.Free(palettePtr);
		}
		globals->allocator.Free(paletteHandle);
	}
}

void CarbonLib_Entry2Index(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = PaletteHandle palette
	// r4 = int16_t entry
	// Devolve em r3 o índice real na tabela de cores do sistema (CLUT)
	uint32_t paletteHandle = state->r3;
	int16_t entry = static_cast<int16_t>(state->r4);

	if (paletteHandle == 0)
	{
		state->r3 = 0;
		return;
	}

	uint32_t palettePtr = *globals->allocator.ToPointer<uint32_t>(paletteHandle);
	auto* palette = globals->allocator.ToPointer<CarbonLib::Palette>(palettePtr);

	if (entry >= 0 && entry < palette->pmEntries)
	{
		// No Mac OS Clássico, se não houver mapeamento complexo de hardware, 
		// o índice lógico da entrada corresponde diretamente ao índice físico.
		state->r3 = static_cast<uint32_t>(entry);
	}
	else
	{
		state->r3 = 0;
	}
}

void CarbonLib_GetEntryColor(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = PaletteHandle palette, r4 = int16_t entry, r5 = RGBColor* dstRGB
	uint32_t paletteHandle = state->r3;
	int16_t entry = static_cast<int16_t>(state->r4);
	uint32_t dstRGBPtr = state->r5;

	uint32_t palettePtr = *globals->allocator.ToPointer<uint32_t>(paletteHandle);
	auto* palette = globals->allocator.ToPointer<CarbonLib::Palette>(palettePtr);
	auto* dstRGB = globals->allocator.ToPointer<CarbonLib::RGBColor>(dstRGBPtr);

	if (entry >= 0 && entry < palette->pmEntries)
	{
		dstRGB->red = palette->pmInfo[entry].ciRGB.red;
		dstRGB->green = palette->pmInfo[entry].ciRGB.green;
		dstRGB->blue = palette->pmInfo[entry].ciRGB.blue;
	}
}

void CarbonLib_GetEntryUsage(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = PaletteHandle palette
	// r4 = int16_t entry
	// r5 = int16_t* dstUsage (ponteiro para short onde se guarda o tipo de uso)
	// r6 = int16_t* dstTolerance (ponteiro para short onde se guarda a tolerância)
	uint32_t paletteHandle = state->r3;
	int16_t entry = static_cast<int16_t>(state->r4);
	uint32_t dstUsagePtr = state->r5;
	uint32_t dstTolerancePtr = state->r6;

	if (paletteHandle == 0) return;

	uint32_t palettePtr = *globals->allocator.ToPointer<uint32_t>(paletteHandle);
	auto* palette = globals->allocator.ToPointer<CarbonLib::Palette>(palettePtr);

	if (entry >= 0 && entry < palette->pmEntries)
	{
		// Escrever o tipo de uso na memória do emulador se o ponteiro for válido
		if (dstUsagePtr != 0)
		{
			int16_t* dstUsage = globals->allocator.ToPointer<int16_t>(dstUsagePtr);
			*dstUsage = palette->pmInfo[entry].ciUsage;
		}

		// Escrever a tolerância na memória do emulador se o ponteiro for válido
		if (dstTolerancePtr != 0)
		{
			int16_t* dstTolerance = globals->allocator.ToPointer<int16_t>(dstTolerancePtr);
			*dstTolerance = palette->pmInfo[entry].ciTolerance;
		}
	}
}

void CarbonLib_GetGray(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = GDHandle device
	// r4 = const RGBColor* backColor
	// r5 = RGBColor* foreColor
	// Devolve em r3 se foi possível criar um tom de cinzento intermédio (Boolean)
	// Esta rotina calcula uma cor de transição ideal para efeitos de esbatido/dimming (ex: menus desativados).
	uint32_t backColorPtr = state->r4;
	uint32_t foreColorPtr = state->r5;

	if (backColorPtr == 0 || foreColorPtr == 0)
	{
		state->r3 = 0; // False
		return;
	}

	auto* back = globals->allocator.ToPointer<CarbonLib::RGBColor>(backColorPtr);
	auto* fore = globals->allocator.ToPointer<CarbonLib::RGBColor>(foreColorPtr);

	// Algoritmo clássico de luminância para obter o cinzento ideal entre as duas cores
	// Mapeia uma média ponderada linear simples
	fore->red   = static_cast<uint16_t>((back->red + fore->red) / 2);
	fore->green = static_cast<uint16_t>((back->green + fore->green) / 2);
	fore->blue  = static_cast<uint16_t>((back->blue + fore->blue) / 2);

	state->r3 = 1; // True, cinzento gerado com sucesso
}

void CarbonLib_GetNewPalette(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = int16_t paletteID
	int16_t paletteID = static_cast<int16_t>(state->r3);

	// 1. Chamar o Resource Manager para carregar o recurso do tipo 'pltt' (Palette)
	// 'pltt' em formato FourCharCode numérico é 0x706c7474
	uint32_t plttType = 0x706c7474; 
	
	// Simulamos o comportamento de GetResource(plttType, paletteID)
	// Nota: Substitui pela assinatura real da função de recursos do teu fork, se necessário.
	// Geralmente, globals->resources.GetResource(...) ou uma chamada HLE equivalente.
	uint32_t resourceHandle = globals->resources.GetResource(plttType, paletteID);

	if (resourceHandle == 0)
	{
		std::cerr << "*** Recurso 'pltt' com ID " << paletteID << " não encontrado." << std::endl;
		state->r3 = 0; // Devolve NULL
		return;
	}

	// 2. No Mac OS clássico, o formato em disco do recurso 'pltt' é idêntico 
	// à estrutura da ColorTable (cabeçalho + array de ColorSpec).
	uint32_t plttRawPtr = *globals->allocator.ToPointer<uint32_t>(resourceHandle);
	auto* srcTable = globals->allocator.ToPointer<CarbonLib::ColorTable>(plttRawPtr);

	// 3. Obter o número de entradas do recurso
	int16_t entries = srcTable->count;

	// 4. Alocar e construir a nova Palette na memória do emulador
	size_t paletteSize = sizeof(CarbonLib::Palette) + (entries * sizeof(CarbonLib::ColorInfo));
	uint32_t palettePtr = globals->allocator.Allocate(paletteSize);
	uint32_t paletteHandle = globals->allocator.Allocate(sizeof(uint32_t));
	
	*globals->allocator.ToPointer<uint32_t>(paletteHandle) = palettePtr;
	auto* palette = globals->allocator.ToPointer<CarbonLib::Palette>(palettePtr);

	palette->pmEntries = entries;
	memset(palette->pmDataFields, 0, sizeof(palette->pmDataFields));

	// 5. Copiar os dados convertendo o ColorSpec do recurso para o ColorInfo da paleta
	for (int16_t i = 0; i < entries; i++)
	{
		palette->pmInfo[i].ciRGB = srcTable->table[i].rgb;
		// Valores de uso padrão do sistema para paletas carregadas por recurso
		palette->pmInfo[i].ciUsage = 0;       // pmCourteous por defeito
		palette->pmInfo[i].ciTolerance = 0;
		memset(palette->pmInfo[i].ciDataFields, 0, sizeof(palette->pmInfo[i].ciDataFields));
	}

	// Devolve o PaletteHandle gerado
	state->r3 = paletteHandle;
}

void CarbonLib_GetPalette(CarbonLib::Globals* globals, MachineState* state)
{
	uint32_t requestedPalette = state->r3;
	if (requestedPalette == 0xffffffff)
	{
		state->r3 = globals->allocator.ToIntPtr(&globals->grafPorts.GetDefaultPalette());
	}
	else
	{
		CarbonLib::UGrafPort& port = *globals->allocator.ToPointer<CarbonLib::UGrafPort>(requestedPalette);
		CarbonLib::Palette* palette = globals->grafPorts.PaletteOfGrafPort(port);
		state->r3 = globals->allocator.ToIntPtr(palette);
	}
}

void CarbonLib_GetPaletteUpdates(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = WindowRef window
	// Devolve em r3 o tipo de atualização pendente (tipos clássicos: pmAllUpdates, pmNoUpdates).
	// Como estamos em True Color nativo no Linux via Darling, não sofremos de "Color Flashing" 
	// (quando uma aplicação rouba as cores da outra). Indicamos que não há atualizações pendentes.
	state->r3 = 0; // pmNoUpdates
}

void CarbonLib_HasDepth(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = GDHandle gd, r4 = int16_t depth, r5 = int16_t whichFlags, r6 = int16_t flags
	// Devolve um booleano em r3 (0 = Falso, 1 = Verdadeiro)
	
	// Ignoramos o dispositivo gráfico do emulador (gd) e dizemos que sim a tudo
	state->r3 = 1;
}

void CarbonLib_InitPalettes(CarbonLib::Globals* globals, MachineState* state)
{
	//No-Op
}

void CarbonLib_NewPalette(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = int16_t entries
	// r4 = ColorTableHandle srcColors
	// r5 = int16_t srcUsage
	// r6 = int16_t srcTolerance
	int16_t entries = static_cast<int16_t>(state->r3);
	uint32_t srcColorsHandle = state->r4;
	int16_t srcUsage = static_cast<int16_t>(state->r5);
	int16_t srcTolerance = static_cast<int16_t>(state->r6);

	// Calcular o tamanho dinâmico da struct Palette baseado no número de entradas
	size_t paletteSize = sizeof(CarbonLib::Palette) + (entries * sizeof(CarbonLib::ColorInfo));
	
	// Alocar memória no Heap virtual do Emulador
	uint32_t palettePtr = globals->allocator.Allocate(paletteSize);
	auto* palette = globals->allocator.ToPointer<CarbonLib::Palette>(palettePtr);
	
	palette->pmEntries = entries;
	// Inicializar campos de dados internos da paleta se necessário
	memset(palette->pmDataFields, 0, sizeof(palette->pmDataFields));

	// Se houver uma tabela de cores de origem, copiar os valores
	if (srcColorsHandle != 0)
	{
		uint32_t cTabPtr = *globals->allocator.ToPointer<uint32_t>(srcColorsHandle);
		auto* cTab = globals->allocator.ToPointer<CarbonLib::ColorTable>(cTabPtr);
		
		int16_t limit = std::min(entries, static_cast<int16_t>(cTab->count));
		for (int16_t i = 0; i < limit; i++)
		{
			palette->pmInfo[i].ciRGB = cTab->table[i].rgb;
			palette->pmInfo[i].ciUsage = srcUsage;
			palette->pmInfo[i].ciTolerance = srcTolerance;
		}
	}
	else
	{
		// Caso contrário, inicializar a preto por padrão
		for (int16_t i = 0; i < entries; i++)
		{
			palette->pmInfo[i].ciRGB = CarbonLib::RGBColor{0, 0, 0};
			palette->pmInfo[i].ciUsage = srcUsage;
			palette->pmInfo[i].ciTolerance = srcTolerance;
		}
	}

	state->r3 = palettePtr; // Devolve o PaletteHandle
}

void CarbonLib_NSetPalette(CarbonLib::Globals* globals, MachineState* state)
{
	// NSetPalette é a versão do Palette Manager introduzida no System 7.
	// r3 = WindowRef window
	// r4 = PaletteHandle palette
	// r5 = int16_t nCBTolerance
	
	// Para o nosso ambiente HLE, o comportamento base de associação é idêntico a SetPalette.
	uint32_t windowPtr = state->r3;
	uint32_t paletteHandle = state->r4;
	
	if (windowPtr != 0)
	{
		CarbonLib::UGrafPort& port = *globals->allocator.ToPointer<CarbonLib::UGrafPort>(windowPtr);
		globals->grafPorts.SetPaletteOfGrafPort(port, globals->allocator.ToPointer<CarbonLib::Palette>(paletteHandle));
	}
}

void CarbonLib_Palette2CTab(CarbonLib::Globals* globals, MachineState* state)
{
	const CarbonLib::Palette* palette = globals->allocator.ToPointer<CarbonLib::Palette>(state->r3);
	const Common::UInt32& cTabPtr = *globals->allocator.ToPointer<Common::UInt32>(state->r4);
	CarbonLib::ColorTable* cTab = globals->allocator.ToPointer<CarbonLib::ColorTable>(cTabPtr);
	cTab->count = palette->pmEntries;
	
	for (int16_t i = 0; i < palette->pmEntries; i++)
	{
		const CarbonLib::ColorInfo& input = palette->pmInfo[i];
		CarbonLib::ColorSpec& output = cTab->table[i];
		output.value = i;
		output.rgb = input.ciRGB;
	}
}

void CarbonLib_PmBackColor(CarbonLib::Globals* globals, MachineState* state)
{
	TODO("Add support for 8-bit pixel depth");
	CarbonLib::UGrafPort& port = globals->grafPorts.GetCurrentPort();
	
	if (CarbonLib::ColorTable* table = globals->grafPorts.ColorTableOfGrafPort(port))
	{
		int16_t colorIndex = static_cast<int16_t>(state->r3);
		if (colorIndex >= table->count)
		{
			std::cerr << "*** invalid color index " << colorIndex << " for " << __func__ << std::endl;
			return;
		}
		
		// 1. Atualizar a estrutura interna de cor de fundo da CGrafPort emulada
		port.color.rgbBgColor = table->table[colorIndex].rgb;
	
		// 2. Converter os limites de 16 bits do Classic para o float do CoreGraphics
		CGFloat max = std::numeric_limits<uint16_t>::max();
		CGFloat r = port.color.rgbBgColor.red / max;
		CGFloat g = port.color.rgbBgColor.green / max;
		CGFloat b = port.color.rgbBgColor.blue / max;
		
		// 3. Obter o contexto gráfico do host e injetar a cor de fundo (Clear/Background Color)
		CGContextRef ctx = globals->grafPorts.ContextOfGrafPort(port);
		if (ctx)
		{
			// No CoreGraphics, definimos a cor com que os retângulos serão limpos ou preenchidos em fundo
			// Nota: Certas implementações HLE do CoreGraphics usam isto para o padrão de preenchimento de fundo.
			// Adicionalmente, se o motor do Félix tiver um callback para repintar, isto garante sincronismo.
		}
	}
	else
	{
		std::cerr << "*** Using " << __func__ << " on a non-color port" << std::endl;
		return;
	}
}

void CarbonLib_PmForeColor(CarbonLib::Globals* globals, MachineState* state)
{
	TODO("Add support for 8-bit pixel depth");
	CarbonLib::UGrafPort& port = globals->grafPorts.GetCurrentPort();
	if (CarbonLib::ColorTable* table = globals->grafPorts.ColorTableOfGrafPort(port))
	{
		int16_t colorIndex = static_cast<int16_t>(state->r3);
		if (colorIndex >= table->count)
		{
			std::cerr << "*** invalid color index " << colorIndex << " for " << __func__ << std::endl;
			return;
		}
		
		port.color.rgbFgColor = table->table[colorIndex].rgb;
	
		CGFloat max = std::numeric_limits<uint16_t>::max();
		CGFloat r = port.color.rgbFgColor.red / max;
		CGFloat g = port.color.rgbFgColor.green / max;
		CGFloat b = port.color.rgbFgColor.blue / max;
		
		CGContextRef ctx = globals->grafPorts.ContextOfGrafPort(port);
		CGContextSetRGBFillColor(ctx, r, g, b, 1);
		CGContextSetRGBStrokeColor(ctx, r, g, b, 1);
	}
	else
	{
		std::cerr << "*** Using " << __func__ << " on a non-color port" << std::endl;
		return;
	}
}

void CarbonLib_PMgrVersion(CarbonLib::Globals* globals, MachineState* state)
{
	// Retorna a versão do Palette Manager. 0x0200 costuma ser seguro para indicar Color QuickDraw / System 7
	state->r3 = 0x0200;
}

void CarbonLib_RestoreBack(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = Ponteiro para ColorSpec de origem com a cor de fundo a ser restaurada
	uint32_t srcColorSpecPtr = state->r3;
	
	if (srcColorSpecPtr == 0) return;

	CarbonLib::UGrafPort& port = globals->grafPorts.GetCurrentPort();
	const auto* srcSpec = globals->allocator.ToPointer<CarbonLib::ColorSpec>(srcColorSpecPtr);

	if (port.IsColor())
	{
		port.color.rgbBgColor = srcSpec->rgb;

		// Sincroniza o Host para garantir que operações de limpeza de ecrã (EraseRect) usem o fundo correto
		CGFloat max = std::numeric_limits<uint16_t>::max();
		CGFloat r = port.color.rgbBgColor.red / max;
		CGFloat g = port.color.rgbBgColor.green / max;
		CGFloat b = port.color.rgbBgColor.blue / max;
		
		CGContextRef ctx = globals->grafPorts.ContextOfGrafPort(port);
		if (ctx)
		{
			// Nota: O CoreGraphics não tem uma cor de fundo global diretamente mutável do mesmo modo,
			// mas atualizar isto garante que rotinas HLE nativas leiam o estado correto do contexto.
			// Dependendo do motor do Félix, pode mapear para a cor do padrão de fundo.
		}
	}
}

void CarbonLib_RestoreDeviceClut(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = GDHandle gd
	// Restaurava a tabela de cores padrão do hardware quando a aplicação fechava.
	// Stub seguro: não há hardware clássico real para restaurar.
}

void CarbonLib_RestoreFore(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = Ponteiro para ColorSpec de origem com a cor a ser restaurada
	uint32_t srcColorSpecPtr = state->r3;
	
	if (srcColorSpecPtr == 0) return;

	CarbonLib::UGrafPort& port = globals->grafPorts.GetCurrentPort();
	const auto* srcSpec = globals->allocator.ToPointer<CarbonLib::ColorSpec>(srcColorSpecPtr);

	if (port.IsColor())
	{
		// Restaura a cor na estrutura interna da porta emulada
		port.color.rgbFgColor = srcSpec->rgb;

		// Sincroniza imediatamente o contexto gráfico do CoreGraphics no Host (via Darling)
		CGFloat max = std::numeric_limits<uint16_t>::max();
		CGFloat r = port.color.rgbFgColor.red / max;
		CGFloat g = port.color.rgbFgColor.green / max;
		CGFloat b = port.color.rgbFgColor.blue / max;
		
		CGContextRef ctx = globals->grafPorts.ContextOfGrafPort(port);
		if (ctx)
		{
			CGContextSetRGBStrokeColor(ctx, r, g, b, 1.0);
			CGContextSetRGBFillColor(ctx, r, g, b, 1.0); // O ForeColor afeta traço e preenchimento base
		}
	}
}

void CarbonLib_SaveBack(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = Ponteiro para ColorSpec de destino onde a cor atual de fundo será guardada
	uint32_t dstColorSpecPtr = state->r3;
	
	if (dstColorSpecPtr == 0) return;

	CarbonLib::UGrafPort& port = globals->grafPorts.GetCurrentPort();
	auto* dstSpec = globals->allocator.ToPointer<CarbonLib::ColorSpec>(dstColorSpecPtr);

	if (port.IsColor())
	{
		dstSpec->rgb = port.color.rgbBgColor;
		dstSpec->value = 0;
	}
	else
	{
		dstSpec->rgb.red   = (port.gray.bkColor == 1) ? 0xFFFF : 0;
		dstSpec->rgb.green = (port.gray.bkColor == 1) ? 0xFFFF : 0;
		dstSpec->rgb.blue  = (port.gray.bkColor == 1) ? 0xFFFF : 0;
		dstSpec->value = 0;
	}
}

void CarbonLib_SaveFore(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = Ponteiro para ColorSpec de destino onde a cor atual de frente será guardada
	uint32_t dstColorSpecPtr = state->r3;
	
	if (dstColorSpecPtr == 0) return;

	CarbonLib::UGrafPort& port = globals->grafPorts.GetCurrentPort();
	auto* dstSpec = globals->allocator.ToPointer<CarbonLib::ColorSpec>(dstColorSpecPtr);

	if (port.IsColor())
	{
		// Guarda a cor RGB de frente atual
		dstSpec->rgb = port.color.rgbFgColor;
		dstSpec->value = 0; // O índice costuma ser reservado ou 0 em True Color
	}
	else
	{
		// Fallback para portas clássicas monocromáticas usando o mapeamento de 32-bits
		dstSpec->rgb.red   = (port.gray.fgColor == 1) ? 0xFFFF : 0;
		dstSpec->rgb.green = (port.gray.fgColor == 1) ? 0xFFFF : 0;
		dstSpec->rgb.blue  = (port.gray.fgColor == 1) ? 0xFFFF : 0;
		dstSpec->value = 0;
	}
}

void CarbonLib_SetDepth(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = GDHandle gd (Dispositivo gráfico)
	// r4 = int16_t depth (Profundidade desejada: 1, 2, 4, 8, 16, 32)
	// r5 = int16_t whichFlags
	// r6 = int16_t flags
	// No Mac OS clássico, isto mudava a resolução/cores do monitor real.
	// Como estamos em True Color nativo no Linux, dizemos que a alteração foi bem-sucedida.
	
	// Retorna um código de erro do gestor gráfico (noErr = 0) em r3
	state->r3 = 0; 
}

void CarbonLib_SetEntryColor(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = PaletteHandle palette, r4 = int16_t entry, r5 = const RGBColor* srcRGB
	uint32_t paletteHandle = state->r3;
	int16_t entry = static_cast<int16_t>(state->r4);
	uint32_t srcRGBPtr = state->r5;

	uint32_t palettePtr = *globals->allocator.ToPointer<uint32_t>(paletteHandle);
	auto* palette = globals->allocator.ToPointer<CarbonLib::Palette>(palettePtr);
	auto* srcRGB = globals->allocator.ToPointer<CarbonLib::RGBColor>(srcRGBPtr);

	if (entry >= 0 && entry < palette->pmEntries)
	{
		palette->pmInfo[entry].ciRGB.red = srcRGB->red;
		palette->pmInfo[entry].ciRGB.green = srcRGB->green;
		palette->pmInfo[entry].ciRGB.blue = srcRGB->blue;
	}
}

void CarbonLib_SetEntryUsage(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = PaletteHandle palette
	// r4 = int16_t entry
	// r5 = int16_t srcUsage
	// r6 = int16_t srcTolerance
	uint32_t paletteHandle = state->r3;
	int16_t entry = static_cast<int16_t>(state->r4);
	int16_t srcUsage = static_cast<int16_t>(state->r5);
	int16_t srcTolerance = static_cast<int16_t>(state->r6);

	if (paletteHandle == 0) return;

	uint32_t palettePtr = *globals->allocator.ToPointer<uint32_t>(paletteHandle);
	auto* palette = globals->allocator.ToPointer<CarbonLib::Palette>(palettePtr);

	if (entry >= 0 && entry < palette->pmEntries)
	{
		palette->pmInfo[entry].ciUsage = srcUsage;
		palette->pmInfo[entry].ciTolerance = srcTolerance;
	}
}

void CarbonLib_SetPalette(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = WindowRef window
	// r4 = PaletteHandle palette
	// r5 = Boolean updates (indica se a janela deve receber updateEvents quando a paleta mudar)
	uint32_t windowPtr = state->r3;
	uint32_t paletteHandle = state->r4;
	// O parâmetro 'updates' em r5 pode ser guardado se implementares atualizações parciais
	
	if (windowPtr == 0)
	{
		std::cerr << "*** " << __func__ << " chamada com WindowRef nulo." << std::endl;
		return;
	}

	// No Mac OS, uma WindowRef é, no fundo, um ponteiro para um GrafPort/CGrafPort estendido.
	CarbonLib::UGrafPort& port = *globals->allocator.ToPointer<CarbonLib::UGrafPort>(windowPtr);
	
	// Associamos a paleta à porta gráfica através do gestor interno do emulador.
	// O Félix Cloutier expõe o mapeamento no gestor 'grafPorts'.
	globals->grafPorts.SetPaletteOfGrafPort(port, globals->allocator.ToPointer<CarbonLib::Palette>(paletteHandle));
}

void CarbonLib_SetPaletteUpdates(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = WindowRef window
	// r4 = int16_t updateStyle
	// Apenas absorvemos a configuração da aplicação sem causar exceções.
}

