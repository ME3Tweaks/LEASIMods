#include <stdio.h>
#include <io.h>
#include <string>
#include <fstream>
#include <iostream>
#include <ostream>
#include <streambuf>
#include <sstream>

#define ASINAME L"KismetLogger"
#define MYHOOK "KismetLogger_"
#define VERSION L"3.0.0"

#ifdef GAMELE1
#include "../../LE1-ASI-Plugins/LE1-SDK/Interface.h"
SPI_PLUGINSIDE_SUPPORT(ASINAME, VERSION, L"ME3Tweaks", SPI_GAME_LE1, SPI_VERSION_ANY);
#endif
#ifdef GAMELE2
#include "../../LE2-ASI-Plugins/LE2-SDK/Interface.h"
SPI_PLUGINSIDE_SUPPORT(ASINAME, VERSION, L"ME3Tweaks", SPI_GAME_LE2, SPI_VERSION_ANY);
#endif
#ifdef GAMELE3
#include "../../LE3-ASI-Plugins/LE3-SDK/Interface.h"
SPI_PLUGINSIDE_SUPPORT(ASINAME, VERSION, L"ME3Tweaks", SPI_GAME_LE3, SPI_VERSION_ANY);
#endif

#include "../../Shared-ASI/Common.h"
#include "../../Shared-ASI/ME3Tweaks/ME3TweaksHeader.h"

SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;

ME3TweaksASILogger logger("KismetLogger v3", "KismetLog.txt");

// ProcessEvent hook (for non-native .Activated())
// ======================================================================

typedef void (*tProcessEvent)(UObject* Context, UFunction* Function, void* Parms, void* Result);
tProcessEvent ProcessEvent = nullptr;

tProcessEvent ProcessEvent_orig = nullptr;
void ProcessEvent_hook(UObject* Context, UFunction* Function, void* Parms, void* Result)
{
	if (!strcmp(Function->GetFullName(), "Function Engine.SequenceOp.Activated"))
	{
		const auto op = reinterpret_cast<USequenceOp*>(Context);
		char* fullInstancedPath = op->GetFullPath();
		char* className = op->Class->Name.GetName();
		wchar_t* inputPin = {};
		bool hasInputPin = false;
		for (int i = 0; i < op->InputLinks.Count; i++)
		{
			auto t = op->InputLinks.Data[i];
			if (t.bHasImpulse)
			{
				inputPin = t.LinkDesc.Data;
				hasInputPin = true;
				break;
			}
		}

		if (hasInputPin)
		{
			// C strings are literally the WORST
			char inputPinNonWide[256];
			sprintf(inputPinNonWide, "%ws", inputPin);
			logger.writeToLog(string_format("%s %s %s\n", className, fullInstancedPath, inputPinNonWide), true);
		}
		else
		{
			logger.writeToLog(string_format("%s %s\n", className, fullInstancedPath), true); // could not see input pin
		}

	}
	ProcessEvent_orig(Context, Function, Parms, Result);
}

// ======================================================================
// ProcessInternal hook (for native .Activated())
// ======================================================================

struct OutParmInfo
{
	UProperty* Prop;
	BYTE* PropAddr;
	OutParmInfo* Next;
};

struct FFrame
{
	void* vtable;
#if defined GAMELE1 || defined GAMELE2
	int unks[3];
#elif defined GAMELE3
	int unks[4];
#endif
	UStruct* Node;
	UObject* Object;
	BYTE* Code;
	BYTE* Locals;
	FFrame* PreviousFrame;
	OutParmInfo* OutParms;
};

typedef void (*tProcessInternal)(UObject* Context, FFrame* Stack, void* Result);
tProcessInternal ProcessInternal = nullptr;
tProcessInternal ProcessInternal_orig = nullptr;
void ProcessInternal_hook(UObject* Context, FFrame* Stack, void* Result)
{
	auto func = Stack->Node;
	auto obj = Stack->Object;
	if (obj->IsA(USequenceOp::StaticClass()) && !strcmp(func->GetName(), "Activated"))
	{
		const auto op = reinterpret_cast<USequenceOp*>(Context);
		char* fullInstancedPath = op->GetFullPath();
		char* className = op->Class->Name.GetName();
		logger.writeToLog(string_format("%s %s\n", className, fullInstancedPath), true);
	}
	ProcessInternal_orig(Context, Stack, Result);
}


SPI_IMPLEMENT_ATTACH
{
	Common::OpenConsole();

	auto _ = SDKInitializer::Instance();

	// Hook ProcessEvent for Non-Native
	INIT_FIND_PATTERN_POSTHOOK(ProcessEvent, LE_PATTERN_POSTHOOK_PROCESSEVENT);
	INIT_HOOK_PATTERN(ProcessEvent);

	// Hook ProcessInternal for native .Activated() calls
	INIT_FIND_PATTERN_POSTHOOK(ProcessInternal, LE_PATTERN_POSTHOOK_PROCESSINTERNAL);
	INIT_HOOK_PATTERN(ProcessInternal);


	return true;
}

SPI_IMPLEMENT_DETACH
{
	Common::CloseConsole();
	logger.flush();
	return true;
}
