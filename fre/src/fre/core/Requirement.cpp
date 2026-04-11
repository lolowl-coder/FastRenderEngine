#include "fre/core/Requirement.hpp"
#include "fre/core/Log.hpp"

namespace fre
{
	bool evaluateRequirement(bool supported, RequirementRequest& requirement)
	{
		bool result = false;

		if (supported)
		{
			result = true;
			requirement.enabled = true;
			LOG_INFO("Feature \033[36m{}\033[0m is supported and enabled", requirement.name);
		}
		else
		{
			if (requirement.requirement == Requirement::Required)
			{
				result = false;
			}
			else
			{
				result = true;
				LOG_WARNING("Feature \033[36m{}\033[0m not supported, but it's optional", requirement.name);
				requirement.enabled = false;
			}
		}

		return result;
	}
}