#pragma once

#include <fstream>
#include <iostream>
#include <cstdio>

#include "../../Shared-ASI/Common.h"
#include "../../Shared-ASI/Interface.h"
#include "../../Shared-ASI/ME3Tweaks/ME3TweaksHeader.h"

#define MYHOOK "KismetLogger_"

#ifdef GAMELE1
SPI_PLUGINSIDE_SUPPORT(L"FunctionLogger", L"2.0.0", L"Mgamerz", SPI_GAME_LE1, SPI_VERSION_ANY);
#endif
#ifdef GAMELE2
SPI_PLUGINSIDE_SUPPORT(L"FunctionLogger", L"2.0.0", L"Mgamerz", SPI_GAME_LE2, SPI_VERSION_ANY);
#endif
#ifdef GAMELE3
SPI_PLUGINSIDE_SUPPORT(L"FunctionLogger", L"2.0.0", L"Mgamerz", SPI_GAME_LE3, SPI_VERSION_ANY);
#endif
SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;

#ifdef GAMELE1
ME3TweaksASILogger logger("Function Logger v2", "LE1FunctionLog.log");
#endif
#ifdef GAMELE2
ME3TweaksASILogger logger("Function Logger v2", "LE2FunctionLog.log");
#endif
#ifdef GAMELE3
ME3TweaksASILogger logger("Function Logger v2", "LE3FunctionLog.log");
#endif



// ProcessEvent hook
// ======================================================================

typedef void (*tProcessEvent)(UObject* Context, UFunction* Function, void* Parms, void* Result);
tProcessEvent ProcessEvent = nullptr;

tProcessEvent ProcessEvent_orig = nullptr;
void ProcessEvent_hook(UObject* Context, UFunction* Function, void* Parms, void* Result)
{
	logger.writeToLog(string_format("[U] %s->%s()\n", Context->GetFullPath(), Function->GetFullName(false)), true, false, false);
	logger.flush();
	ProcessEvent_orig(Context, Function, Parms, Result);
}

typedef void (*tProcessInternal)(UObject* Context, FFrame* Stack, void* Result);
tProcessInternal ProcessInternal = nullptr;
tProcessInternal ProcessInternal_orig = nullptr;
void ProcessInternal_hook(UObject* Context, FFrame* Stack, void* Result)
{
	logger.writeToLog(string_format("[N] %s->%s()\n", Context->GetFullPath(), Stack->Node->GetFullName(false)), true, false, false);
	logger.flush();
	ProcessInternal_orig(Context, Stack, Result);
}


SPI_IMPLEMENT_ATTACH
{
	Common::OpenConsole();

	INIT_CHECK_SDK()
	writeln(L"Crash game as soon as possible in order to keep log size down");

	// Hook ProcessEvent for Non-Native
	INIT_FIND_PATTERN_POSTHOOK(ProcessEvent, LE_PATTERN_POSTHOOK_PROCESSEVENT);
	INIT_HOOK_PATTERN(ProcessEvent);

	// Hook ProcessInternal for native calls
	INIT_FIND_PATTERN_POSTHOOK(ProcessInternal, LE_PATTERN_POSTHOOK_PROCESSINTERNAL);
	INIT_HOOK_PATTERN(ProcessInternal);

	return true;
}

SPI_IMPLEMENT_DETACH
{
	Common::CloseConsole();
	return true;
}
