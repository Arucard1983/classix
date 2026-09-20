//
// NotSupportedException.h
// Classix
//

#ifndef __Classix__NotSupportedException__
#define __Classix__NotSupportedException__

#include "PPCRuntimeException.h"
#include <string>

namespace PPCVM
{
	class NotSupportedException : public Common::PPCRuntimeException
	{
		std::string featureName;
		std::string description;
		std::string fullMessage;
		
	public:
		explicit NotSupportedException(const std::string& feature);
		NotSupportedException(const std::string& feature, const std::string& description);
		
		virtual Common::PPCRuntimeException* ToHeapAlloc() const override;
		virtual const char* what() const noexcept override;
		virtual ~NotSupportedException() override;
	};
}

#endif /* defined(__Classix__NotSupportedException__) */
