//
// GrafPortManager.h
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

#ifndef __Classix__GrafPortManager__
#define __Classix__GrafPortManager__

#include <unordered_map>
#include "Allocator.h"
#include "CommonDefinitions.h"

// I don't want to #include IOSurface or CoreGraphics here because it brings
// tons of ambiguous QuickDraw definitions
typedef uint32_t IOSurfaceID;
typedef struct CGContext* CGContextRef;
typedef struct CGRect CGRect;

namespace CarbonLib
{
	class GrafPortData;
	
	class GrafPortManager
	{
		Common::Allocator& allocator;
		std::unordered_map<uint32_t, GrafPortData> ports;
		std::unordered_map<uint32_t, CGRect> updatedRegions;
		GrafPortData* currentPort;
		Palette* defaultPalette;
		
	public:
		GrafPortManager(Common::Allocator& allocator);
		
		CarbonLib::Palette& GetDefaultPalette();
		
		CarbonLib::UGrafPort& AllocateGrayGrafPort(const CarbonLib::Rect& bounds, const std::string& allocationName = "");
		CarbonLib::UGrafPort& AllocateColorGrafPort(const CarbonLib::Rect& bounds, const CarbonLib::Palette* palette = nullptr, const std::string& allocationName = "");
		
		void InitializeGrayGrafPort(CarbonLib::UGrafPort& port, const CarbonLib::Rect& bounds);
		void InitializeColorGrafPort(CarbonLib::UGrafPort& port, const CarbonLib::Rect& bounds, const CarbonLib::Palette* palette = nullptr);
		
		void SetCurrentPort(CarbonLib::UGrafPort& port);
		CarbonLib::UGrafPort& GetCurrentPort();
		
		CarbonLib::Palette* PaletteOfGrafPort(CarbonLib::UGrafPort& port);
		CarbonLib::ColorTable* ColorTableOfGrafPort(CarbonLib::UGrafPort& port);
		CGContextRef ContextOfGrafPort(CarbonLib::UGrafPort& port);
		IOSurfaceID SurfaceOfGrafPort(CarbonLib::UGrafPort& port);
		
		void BeginUpdate(CarbonLib::UGrafPort& port);
		bool UpdateRegion(CarbonLib::UGrafPort& port, CGRect region);
		CGRect EndUpdate(CarbonLib::UGrafPort& port);
		
		// this does not deallocate 'port', but it gets rid of the IOSurface and the graphics context
		void DestroyGrafPort(CarbonLib::UGrafPort& port);
		
		~GrafPortManager();
	};
}

#endif /* defined(__Classix__GrafPortManager__) */
