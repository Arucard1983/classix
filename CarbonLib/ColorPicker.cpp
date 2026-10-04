//
// ColorPicker.cpp
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

#include "Prototypes.h"
#include "NotImplementedException.h"
#include <algorithm>
#include <cmath>

// Auxiliar para ler e converter uma estrutura RGBColor da memória em floats (0.0 a 1.0)
static void ReadRGB(CarbonLib::Globals* globals, uint32_t addr, double& r, double& g, double& b)
{
	auto* rgb = reinterpret_cast<CarbonLib::RGBColor*>(globals->TranslateAddress(addr));
	r = rgb->red / 65535.0;
	g = rgb->green / 65535.0;
	b = rgb->blue / 65535.0;
}

// Auxiliar para escrever floats (0.0 a 1.0) de volta numa estrutura RGBColor na memória
static void WriteRGB(CarbonLib::Globals* globals, uint32_t addr, double r, double g, double b)
{
	auto* rgb = reinterpret_cast<CarbonLib::RGBColor*>(globals->TranslateAddress(addr));
	rgb->red   = static_cast<uint16_t>(std::clamp(r, 0.0, 1.0) * 65535.0);
	rgb->green = static_cast<uint16_t>(std::clamp(g, 0.0, 1.0) * 65535.0);
	rgb->blue  = static_cast<uint16_t>(std::clamp(b, 0.0, 1.0) * 65535.0);
}

void CarbonLib_CMY2RGB(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = const CMYColor* srcColor, r4 = RGBColor* dstColor
	uint16_t* cmy = reinterpret_cast<uint16_t*>(globals->TranslateAddress(state->r3));
	double c = cmy[0] / 65535.0;
	double m = cmy[1] / 65535.0;
	double y = cmy[2] / 65535.0;

	WriteRGB(globals, state->r4, 1.0 - c, 1.0 - m, 1.0 - y);
}

void CarbonLib_Fix2SmallFract(CarbonLib::Globals* globals, MachineState* state)
{
	// Argumento: Fixed está em r3 (ou passado por valor)
    // Retorno: SmallFract deve ser colocado em r3
    uint32_t fixedValue = state->r3;
    
    // Converte ponto fixo 16.16 para SmallFract (0-65535)
    uint16_t smallFract = static_cast<uint16_t>((fixedValue >> 1) / 32768.0 * 65535.0); // Abordagem conceitual de mapeamento
    
    // Na prática do Classic, costuma ser uma divisão direta ou shift ajustado:
    // smallFract = (fixedValue >> 1) & 0xFFFF; (dependendo do comportamento exato da ROM)
    
    state->r3 = smallFract; 
}

void CarbonLib_GetColor(CarbonLib::Globals* globals, MachineState* state)
{
	// Convenção típica do Carbon:
    // r3 = Point where (passado por valor ou endereço)
    // r4 = Ponteiro para Str255 (Pascal String para o Prompt)
    // r5 = Ponteiro para a estrutura RGBColor original
    // r6 = Ponteiro para a estrutura RGBColor de saída
    
    // 1. Extrair os dados da memória PPC através do MachineState
    CarbonLib::Point where = CarbonLib::Point::FromWord(state->r3);
    
    // Obter o Prompt (convertendo a string Pascal da memória PPC em std::string)
    uint32_t promptPtr = state->r4;
    const char* pascalStr = reinterpret_cast<const char*>(globals->TranslateAddress(promptPtr)); // Ajusta conforme o teu método de leitura de memória
    std::string prompt = CarbonLib::PascalStringToCPPString(pascalStr);
    
    // Obter a cor inicial da memória
    uint32_t inColorPtr = state->r5;
    CarbonLib::RGBColor* inColor = reinterpret_cast<CarbonLib::RGBColor*>(globals->TranslateAddress(inColorPtr));
    
    // 2. Chamar o canal de UI (o processo Cocoa externo) através da infraestrutura do autor
    // Esperamos receber de volta um std::tuple com (bool accepted, uint16_t r, uint16_t g, uint16_t b)
    typedef std::tuple<uint32_t, uint16_t, uint16_t, uint16_t> ColorResponse;
    
    auto response = globals->uiChannel->PerformComplexAction<ColorResponse>(
        CarbonLib::IPCMessage::PromptColorPicker,
        prompt,
        inColor->red,
        inColor->green,
        inColor->blue
    );
    
    uint32_t accepted = std::get<0>(response);
    
    if (accepted)
    {
        uint32_t outColorPtr = state->r6;
        CarbonLib::RGBColor* outColor = reinterpret_cast<CarbonLib::RGBColor*>(globals->TranslateAddress(outColorPtr));
        
        // Atualiza a cor na memória do PPC (Lembra-te de gerir o Endianness se necessário!)
        outColor->red = std::get<1>(response);
        outColor->green = std::get<2>(response);
        outColor->blue = std::get<3>(response);
        
        state->r3 = 1; // Retorna TRUE (Utilizador clicou OK)
    }
    else
    {
        state->r3 = 0; // Retorna FALSE (Cancelado)
    }
}

void CarbonLib_HSL2RGB(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = const HSLColor* srcColor, r4 = RGBColor* dstColor
	uint16_t* hsl = reinterpret_cast<uint16_t*>(globals->TranslateAddress(state->r3));
	double h = hsl[0] / 65535.0;
	double s = hsl[1] / 65535.0;
	double l = hsl[2] / 65535.0;

	auto hue2rgb = [](double p, double q, double t) {
		if (t < 0.0) t += 1.0;
		if (t > 1.0) t -= 1.0;
		if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
		if (t < 1.0 / 2.0) return q;
		if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
		return p;
	};

	double r, g, b;
	if (s == 0.0)
	{
		r = g = b = l; // Acromático (cinzento)
	}
	else
	{
		double q = (l < 0.5) ? (l * (1.0 + s)) : (l + s - l * s);
		double p = 2.0 * l - q;
		r = hue2rgb(p, q, h + 1.0 / 3.0);
		g = hue2rgb(p, q, h);
		b = hue2rgb(p, q, h - 1.0 / 3.0);
	}

	WriteRGB(globals, state->r4, r, g, b);
}

void CarbonLib_HSV2RGB(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = const HSVColor* srcColor, r4 = RGBColor* dstColor
	uint16_t* hsv = reinterpret_cast<uint16_t*>(globals->TranslateAddress(state->r3));
	double h = hsv[0] / 65535.0;
	double s = hsv[1] / 65535.0;
	double v = hsv[2] / 65535.0;

	double r = 0.0, g = 0.0, b = 0.0;

	if (s == 0.0)
	{
		r = g = b = v; // Acromático (cinzento)
	}
	else
	{
		double sector = h * 6.0;
		int i = static_cast<int>(std::floor(sector));
		double f = sector - i;
		
		double p = v * (1.0 - s);
		double q = v * (1.0 - s * f);
		double t = v * (1.0 - s * (1.0 - f));

		switch (i % 6)
		{
			case 0: r = v; g = t; b = p; break;
			case 1: r = q; g = v; b = p; break;
			case 2: r = p; g = v; b = t; break;
			case 3: r = p; g = q; b = v; break;
			case 4: r = t; g = p; b = v; break;
			case 5: r = v; g = p; b = q; break;
		}
	}

	WriteRGB(globals, state->r4, r, g, b);
}

void CarbonLib_RGB2CMY(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = const RGBColor* srcColor, r4 = CMYColor* dstColor
	double r, g, b;
	ReadRGB(globals, state->r3, r, g, b);

	// No QuickDraw, CMY é o inverso linear direto de RGB
	uint16_t* cmy = reinterpret_cast<uint16_t*>(globals->TranslateAddress(state->r4));
	cmy[0] = static_cast<uint16_t>((1.0 - r) * 65535.0); // Cyan
	cmy[1] = static_cast<uint16_t>((1.0 - g) * 65535.0); // Magenta
	cmy[2] = static_cast<uint16_t>((1.0 - b) * 65535.0); // Yellow
}

void CarbonLib_RGB2HSL(CarbonLib::Globals* globals, MachineState* state)
{
	// r3 = const RGBColor* srcColor, r4 = HSLColor* dstColor
	double r, g, b;
	ReadRGB(globals, state->r3, r, g, b);

	double maxC = std::max({r, g, b});
	double minC = std::min({r, g, b});
	double delta = maxC - minC;

	double h = 0.0, s = 0.0, l = (maxC + minC) / 2.0;

	if (delta > 0.0)
	{
		s = (l > 0.5) ? (delta / (2.0 - maxC - minC)) : (delta / (maxC + minC));

		if (maxC == r)
		{
			h = (g - b) / delta + (g < b ? 6.0 : 0.0);
		}
		else if (maxC == g)
		{
			h = (b - r) / delta + 2.0;
		}
		else
		{
			h = (r - g) / delta + 4.0;
		}
		h /= 6.0; // Normaliza Hue para 0.0 - 1.0
	}

	uint16_t* hsl = reinterpret_cast<uint16_t*>(globals->TranslateAddress(state->r4));
	hsl[0] = static_cast<uint16_t>(h * 65535.0);
	hsl[1] = static_cast<uint16_t>(s * 65535.0);
	hsl[2] = static_cast<uint16_t>(l * 65535.0);
}

void CarbonLib_RGB2HSV(CarbonLib::Globals* globals, MachineState* state)
{
        // r3 = const RGBColor* srcColor, r4 = HSVColor* dstColor
	double r, g, b;
	ReadRGB(globals, state->r3, r, g, b);

	double maxC = std::max({r, g, b});
	double minC = std::min({r, g, b});
	double delta = maxC - minC;

	double h = 0.0, s = 0.0, v = maxC;

	if (delta > 0.0)
	{
		if (maxC == r)
		{
			h = (g - b) / delta;
			if (h < 0.0) h += 6.0;
		}
		else if (maxC == g)
		{
			h = (b - r) / delta + 2.0;
		}
		else // maxC == b
		{
			h = (r - g) / delta + 4.0;
		}
		h /= 6.0; // Normaliza o Hue de 0.0 a 1.0

		if (maxC > 0.0)
		{
			s = delta / maxC;
		}
	}

	uint16_t* hsv = reinterpret_cast<uint16_t*>(globals->TranslateAddress(state->r4));
	hsv[0] = static_cast<uint16_t>(h * 65535.0); // Hue
	hsv[1] = static_cast<uint16_t>(s * 65535.0); // Saturation
	hsv[2] = static_cast<uint16_t>(v * 65535.0); // Value
}

void CarbonLib_SmallFract2Fix(CarbonLib::Globals* globals, MachineState* state)
{
	uint16_t smallFract = static_cast<uint16_t>(state->r3);
    uint32_t fixedValue = (static_cast<uint32_t>(smallFract) << 1) + 1; // Equivalente à lógica de aproximação da Apple
    
    state->r3 = fixedValue;
}

