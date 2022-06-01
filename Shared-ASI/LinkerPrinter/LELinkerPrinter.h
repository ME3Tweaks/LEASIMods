#include <stdio.h>
#include <io.h>
#include <string>
#include <fstream>
#include <iostream>
#include <map>
#include <ostream>
#include <streambuf>
#include <sstream>


#define MYHOOK "LinkerPrinter_"
#define VERSION L"2.0.0"


#ifdef GAMELE1
#include "../../LE1-ASI-Plugins/LE1-SDK/Interface.h"
SPI_PLUGINSIDE_SUPPORT(L"LinkerPrinter", VERSION, L"ME3Tweaks", SPI_GAME_LE1, SPI_VERSION_ANY);
#endif
#ifdef GAMELE2
#include "../../LE2-ASI-Plugins/LE2-SDK/Interface.h"
SPI_PLUGINSIDE_SUPPORT(L"LinkerPrinter", VERSION, L"ME3Tweaks", SPI_GAME_LE2, SPI_VERSION_ANY);
#endif
#ifdef GAMELE3
#include "../../LE3-ASI-Plugins/LE3-SDK/Interface.h"
SPI_PLUGINSIDE_SUPPORT(L"LinkerPrinter", VERSION, L"ME3Tweaks", SPI_GAME_LE3, SPI_VERSION_ANY);
#endif

#include "../../Shared-ASI/Common.h"
#include "../../Shared-ASI/ME3Tweaks/ME3TweaksHeader.h"


SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;

ME3TweaksASILogger logger("Linker Printer v2", "LinkerPrinter.log");

bool CanPrint = true;

// ======================================================================
// SetLinker hook
// ======================================================================
map<std::string, std::wstring> NodePathToFileNameMap;


typedef void (*tSetLinker)(UObject* Context, ULinkerLoad* Linker, int LinkerIndex);
tSetLinker SetLinker = nullptr;
tSetLinker SetLinker_orig = nullptr;
void SetLinker_hook(UObject* Context, ULinkerLoad* Linker, int LinkerIndex)
{
	if (Context->Linker)
	{
		NodePathToFileNameMap.insert_or_assign(std::string(Context->GetFullPath()), std::wstring(Context->Linker->Filename.Data));
	}
	SetLinker_orig(Context, Linker, LinkerIndex);
}



// ProcessEvent hook (for non-native .Activated())
// ======================================================================

typedef void (*tProcessEvent)(UObject* Context, UFunction* Function, void* Parms, void* Result);
tProcessEvent ProcessEvent = nullptr;

tProcessEvent ProcessEvent_orig = nullptr;

void PrintLinkers()
{
	CanPrint = false;
	writeln("Dumping %llu linkers, this may take a while (game will be unresponsive)...", NodePathToFileNameMap.size());

	// Create a map of Objects to their names (in reverse), otherwise we take a huge performance hit when objects got dumped. 
	// Then we have N build time and then O(1) lookup time instead of N^2

	map<std::string, UObject*> ObjectNameToObjectMap;

	const auto objCount = UObject::GObjObjects()->Count;
	const auto objArray = UObject::GObjObjects()->Data;
	for (auto j = 0; j < objCount; j++)
	{
		auto obj = objArray[j];
		if (obj) {
			ObjectNameToObjectMap.insert_or_assign(std::string(obj->GetFullPath()), obj);
		}
	}

	cout << "0 done";
	long numDone = 0;
	wchar_t buffer[1024];
	for (auto const& [key, val] : NodePathToFileNameMap)
	{
		if (numDone % 1000 == 0)
		{
			cout << "\r" << numDone << " done";
		}

		if (ObjectNameToObjectMap.find(key) == ObjectNameToObjectMap.end())
		{
			// Not found
			swprintf(buffer, 1024, L"%hs -> (Dumped from memory)\n", key.c_str());
		}
		else
		{
			swprintf(buffer, 1024, L"%hs -> %s\n", key.c_str(), val.c_str());
		}

		logger.writeWideToLog(buffer, false);
		numDone++;
	}

	cout << "\rPrinted linker source of " << numDone << " objects\n";
	logger.writeWideLineToLog(L"------------------------------");
	CanPrint = true;
}


void ProcessEvent_hook(UObject* Context, UFunction* Function, void* Parms, void* Result)
{
	if (CanPrint && !strcmp(Function->GetFullName(), "Function SFXGame.BioHUD.PostRender"))
	{
		// Toggle drawing/not drawing
		if ((GetKeyState('O') & 0x8000) && (GetKeyState(VK_CONTROL) & 0x8000)) {
			if (CanPrint) {
				PrintLinkers();
				writeln("Printed linkers to log");
			}
		}
	}
	ProcessEvent_orig(Context, Function, Parms, Result);
}


SPI_IMPLEMENT_ATTACH
{
	Common::OpenConsole();

	auto _ = SDKInitializer::Instance();

	INIT_FIND_PATTERN_POSTHOOK(SetLinker, /*"4c 8b 51 2c 4c"*/ "8b c9 4d 85 d2 74 39 48 85 d2 74 1c 48 8b c1");
	if (const auto rc = InterfacePtr->InstallHook(MYHOOK "SetLinker", SetLinker, SetLinker_hook, reinterpret_cast<void**>(&SetLinker_orig));
		rc != SPIReturn::Success)
	{
		writeln(L"Attach - failed to hook SetLinker: %d / %s", rc, SPIReturnToString(rc));
		return false;
	}

	// Hook ProcessEvent for input combo
	INIT_FIND_PATTERN_POSTHOOK(ProcessEvent, LE_PATTERN_POSTHOOK_PROCESSEVENT);
	if (auto rc = InterfacePtr->InstallHook(MYHOOK "ProcessEvent", ProcessEvent, ProcessEvent_hook, (void**)&ProcessEvent_orig);
		rc != SPIReturn::Success)
	{
		writeln(L"Attach - failed to hook ProcessEvent: %d / %s", rc, SPIReturnToString(rc));
		return false;
	}

	writeln("LinkerPriner: Press CTRL + O to dump all objects that have loaded and their source.");
	return true;
}

SPI_IMPLEMENT_DETACH
{
	Common::CloseConsole();
	logger.flush();
	return true;
}
