#pragma once

#include "../Common.h"
#include "../ME3Tweaks/ME3TweaksHeader.h"
#include "SharedData.h"
#include "UtilityMethods.h"



// typedefs
typedef bool (*tIsWindowFocused)();

#ifdef GAMELE1
#define IS_WINDOW_FOCUSED_PATTERN /*"48 83 ec 28 48*/ "8b 05 5d 3f c2 00 48 85 c0 74 2c 48 8b 88 c8 07 00 00 48 85 c9 74 20 48 8b 89 68 01 00 00"
#elif defined GAMELE2
#define IS_WINDOW_FOCUSED_PATTERN /*"48 83 ec 28 48*/ "8b 05 2d bf c5 00 48 85 c0 74 29 48 8b 88 a0 07 00 00 48 85 c9 74 1d 48 8b 49 78"
#elif defined GAMELE2
#define IS_WINDOW_FOCUSED_PATTERN /*"48 83 ec 28 48*/ "8b 05 5d 3f c2 00 48 85 c0 74 2c 48 8b 88 c8 07 00 00 48 85 c9 74 20 48 8b 89 68 01 00 00"
#endif

class LEAnimViewer
{
#if defined GAMELE1 || defined GAMELE2
	// Memory pre-allocated for use with passing to kismet, since FString doesn't copy it
	static wchar_t ActorFullMemoryPath[1024];
	static std::wstring ActorMemoryPath;
	static std::wstring ActorPackageFile;
#endif

	// BioWorldInfo internal native
	// HasFocus()
	static tIsWindowFocused IsWindowFocused;
	static tIsWindowFocused IsWindowFocused_orig;

	static bool IsWindowFocused_hook()
	{
		if (AllowPausing)
		{
			return IsWindowFocused_orig();
		}
		// We don't report that the window is not focused, ever.
		return true;
	}

	// Return value is not used; it's only to allow usage of macro
	static bool AllowWindowPausing(const bool allow)
	{
		const auto InterfacePtr = SharedData::SPIInterfacePtr;
		if (!IsWindowFocused)
		{
			INIT_POSTHOOK(IsWindowFocused, IS_WINDOW_FOCUSED_PATTERN);
		}
		AllowPausing = allow;
		return true;
	}

	static void Initialize(ISharedProxyInterface* InterfacePtr)
	{
		AllowWindowPausing(false);
	}

public:

	// Return true if handled.
	// Return false if not handled.
	static bool HandleCommand(std::string command)
	{
		// All AnimViewer commands start with ANIMV_
		if (!stringStartsWith("ANIMV_", command))
			return false;

		if (stringStartsWith("ANIMV_ALLOW_WINDOW_PAUSE", command))
		{
			AllowWindowPausing(true);
			return true;
		}
		if (stringStartsWith("ANIMV_DISALLOW_WINDOW_PAUSE", command))
		{
			AllowWindowPausing(false);
			return true;
		}

		if (stringStartsWith("ANIMV_CHANGE_PAWN ", command))
		{
			// Test code.
			auto subCommand = GetCommandParam(command);
			const auto actorFullPath = s2ws(GetCommandParam(command));

			// Copy the string to the buffer
			wcscpy(ActorFullMemoryPath, actorFullPath.c_str());

			// CauseEvent: ChangeActor
			//auto player = reinterpret_cast<ABioPlayerController*>(FindObjectOfType(ABioPlayerController::StaticClass()));
			//if (player)
			//{
			//	FName foundName;
			//	StaticVariables::CreateName(L"ChangeActor", 0, & foundName);
			//	player->CauseEvent(foundName);
			//}
			//return true;

			//ActorPackageFile = s2ws(GetCommandParam(command));
			//ActorMemoryPath = s2ws(GetCommandParam(command));

			return true;
		}

		return false;
	}

	// Return true if other features should also be able to handle this function call
	// Return false if other features shouldn't be able to also handle this function call
	static bool ProcessEvent(UObject* Context, UFunction* Function, void* Parms, void* Result)
	{
		if (!strcmp(Context->Class->GetName(), "LEXSeqAct_SpawnPawn") && !strcmp(Function->GetName(), "Activated"))
		{
			// Make sure the variables are set
			//SetSequenceString(static_cast<USequenceOp*>(Context), L"Type", LEAnimViewer::ActorFullMemoryPath);

			const auto spGame = static_cast<ABioSPGame*>(FindObjectOfType(ABioSPGame::StaticClass()));
			spGame->ServerOptions = FString(ActorFullMemoryPath);
			auto t = "";
			//FVector v = FVector();
			//v.X = -4013;
			//v.Y = -6046;
			//v.Z = -26559;
			//FRotator r = FRotator();

			//auto pf = FString(ActorPackageFile.data());
			//spGame->PreloadPackage(pf); // Load into memory

			//// How do we know when this is done...?
			////std::this_thread::sleep_for(std::chrono::seconds(1));

			//auto mp = FString(ActorMemoryPath.data());
			//auto pawn = spGame->SpawnPawn(mp, v, r, false);
			//FName tag;
			//StaticVariables::CreateName(L"AnimatedActor", 0, &tag);
			//pawn->Tag = tag;
		}

		//if (!strcmp(Context->Class->GetName(), "LEXSeqAct_SpawnPawn") && !strcmp(Function->GetName(), "HookedSpawn"))
		//{
		//	// Make sure the variables are set
		//	SetSequenceString(static_cast<USequenceOp*>(Context), L"Type", LEAnimViewer::ActorFullMemoryPath);
		//}

		// Still process it
		return true;
	}


private:
	static bool AllowPausing;
	static void DumpActors() {

	}

};

// Static variable initialization
bool LEAnimViewer::AllowPausing = false;
tIsWindowFocused LEAnimViewer::IsWindowFocused = nullptr;
tIsWindowFocused LEAnimViewer::IsWindowFocused_orig = nullptr;

#ifdef GAMELE1
wchar_t LEAnimViewer::ActorFullMemoryPath[1024];
std::wstring LEAnimViewer::ActorMemoryPath = L"BIOA_NOR_C.HMM.hench_pilot";
std::wstring LEAnimViewer::ActorPackageFile = L"BIOA_NOR10_01_DS1";
#elif defined GAMELE2
wchar_t LEAnimViewer::ActorFullMemoryPath[1024];
std::wstring LEAnimViewer::ActorMemoryPath = L"BioHench_Vixen.hench_vixen_01";
std::wstring LEAnimViewer::ActorPackageFile = L"BioH_Vixen_01";
#elif defined GAMELE3
#error FIX ME UP FOR LE3
wchar_t LEAnimViewer::ActorFullMemoryPath[1024];
std::wstring LEAnimViewer::ActorMemoryPath = L"BioHench_Vixen.hench_vixen_01";
std::wstring LEAnimViewer::ActorPackageFile = L"BioH_Vixen_01";
#endif