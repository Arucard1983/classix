//
// Gestalt.cpp
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

#include "Gestalt.h"

namespace OSEnvironment
{
	Gestalt::Gestalt(bool LegacyMode)
	{
		//Common values
		SetValue("cput", 11);         // PowerPC G3
        SetValue("proc", 5);          // PowerPC
        SetValue("carb", 1);          // Carbon is present
        SetValue("carv", 0x01608000); // CarbonLib 1.6+
        SetValue("otvr", 0x02768000); // OpenTransport v2.7.6
        SetValue("otau", 1);          // OpenTransport enabled
        SetValue("fs  ", 0x0000000F); // Modern FS (POSIX/Darling)
        SetValue("vol ", 0x00003BFF); // Unix Permissions
        SetValue("thng", 0x00010000); // Thread-Safe Component Manager
        SetValue("menu", 0x00000003); // Carbon Menu Manager
        SetValue("cntl", 0x0000000F); // Embedded Controls
        SetValue("appr", 0x01010000); // Appearance Manager OS X
		
		if(LegacyMode)
		{
          // Set OS 9.2.2 emulation
		  SetValue("sysv", 0x0922);
          SetValue("sys1", 9);
          SetValue("sys2", 2);
          SetValue("sys3", 2);

		  //Classic Environment enabled
		  SetValue("clsc", 1);          //Enable Classic Sandbox
		  SetValue("macv", 4);          //Set Mac version
          SetValue('sysa', 0x00000200); //Set system attributes
		}
		else
		{
          // Set OS 10.4.11 emulation
		  SetValue("sysv", 0x104B);
          SetValue("sys1", 10);
          SetValue("sys2", 4);
          SetValue("sys3", 11);

		  //Classic Environment disabled
		  SetValue("clsc", 0);          //Enable Classic Sandbox
		  SetValue("macv", 5);          //Set Mac version
          SetValue("sysa", 0x00000010); //Set system attributes
		}
	}
	
	void Gestalt::SetValue(uint32_t key, int32_t value)
	{
		auto iter = callbackValues.find(key);
		if (iter != callbackValues.end())
		{
			callbackValues.erase(iter);
		}
		
		fixedValues[key] = value;
	}
	
	void Gestalt::SetValue(const Common::FourCharCode &code, int32_t value)
	{
		SetValue(code.code, value);
	}

	bool Gestalt::GetValue(uint32_t key, int32_t& value)
	{
		auto fixedValueIter = fixedValues.find(key);
		if (fixedValueIter != fixedValues.end())
		{
			value = fixedValueIter->second;
			return true;
		}
		
		auto callbackValueIter = callbackValues.find(key);
		if (callbackValueIter != callbackValues.end())
		{
			value = (*callbackValueIter->second)();
			return true;
		}
		
		return false;
	}
	
	bool Gestalt::GetValue(const Common::FourCharCode& code, int32_t &value)
	{
		return GetValue(code.code, value);
	}
	
	int32_t Gestalt::GetValue(uint32_t key)
	{
		int32_t result;
		if (!GetValue(key, result))
			throw std::logic_error("Could not find gestalt key");
		return result;
	}
	
	int32_t Gestalt::GetValue(const Common::FourCharCode& code)
	{
		return GetValue(code.code);
	}
}
