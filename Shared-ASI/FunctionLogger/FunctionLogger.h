#pragma once

#include <fstream>
#include <iostream>
#include <cstdio>

#include "../../Shared-ASI/Common.h"
#include "../../Shared-ASI/Interface.h"
#include "../../Shared-ASI/ME3Tweaks/ME3TweaksHeader.h"

#define MYHOOK "FunctionLogger_"
#if defined GAMELE1
#define SPI_GAME SPI_GAME_LE1
#define GAMETAG "LE1"
#elif defined GAMELE2
#define SPI_GAME SPI_GAME_LE2
#define GAMETAG "LE2"
#elif defined GAMELE3
#define SPI_GAME SPI_GAME_LE3
#define GAMETAG "LE3"
#endif

#define ASINAME L"FunctionLogger"
#define ASIVERSION "3"
#define ASIDEV L"Mgamerz"

SPI_PLUGINSIDE_SUPPORT(ASINAME, ASIVERSION L".0.0", ASIDEV, SPI_GAME, SPI_VERSION_ANY);
SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;

ME3TweaksASILogger logger("Function Logger v" ASIVERSION, GAMETAG "FunctionLog.log");

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
