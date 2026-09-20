//
// NotSupportedException.cpp
// 2026 (C) Arucard
//

#include "NotSupportedException.h"

namespace PPCVM
{
	NotSupportedException::NotSupportedException(const std::string& feature)
	: featureName(feature), fullMessage(feature + " is not supported and will not be implemented")
	{ }
	
	NotSupportedException::NotSupportedException(const std::string& feature, const std::string& description)
	: featureName(feature), description(description)
	{
		fullMessage = featureName + " is not supported: " + description;
	}
	
	Common::PPCRuntimeException* NotSupportedException::ToHeapAlloc() const
	{
		return new NotSupportedException(featureName, description);
	}
	
	const char* NotSupportedException::what() const noexcept
	{
		return fullMessage.c_str();
	}
	
	NotSupportedException::~NotSupportedException()
	{ }
}
