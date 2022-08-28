#pragma once

#include "../ME3Tweaks/ME3TweaksHeader.h"
#include "UtilityMethods.h"

class LEXCommunications
{
public:
	// Return true if other features should also be able to handle this function call
	// Return false if other features shouldn't be able to also handle this function call
	static bool ProcessEvent(UObject* Context, UFunction* Function, void* Parms, void* Result)
	{
		const auto className = Context->Class->Name.GetName();
		if (strcmp(className, "SeqAct_SendMessageToLEX") == 0)
		{
			if (strcmp(Function->GetFullName(), "Function Engine.SequenceOp.Activated") == 0)
			{
				const auto op = static_cast<USequenceOp*>(Context);
				SendMessageToLEX(op);
				return false; // Other features will not handle this
			}
		}
		return true; // Other features can handle this call
	}
};