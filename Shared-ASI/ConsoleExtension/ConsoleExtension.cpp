#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

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

#define MYHOOK GAMETAG "ConsoleExtension"

#include "../Common.h"
#include "../ConsoleCommandParsing.h"
#include "../Interface.h"
#include "../ME3Tweaks/ME3TweaksHeader.h"
#include "../StaticVariablePointers.h"

#define VERSION "2.0.0"

SPI_PLUGINSIDE_SUPPORT(GAMETAG L"ConsoleExtension", L"SirCxyrtyx", L"" VERSION, SPI_GAME, SPI_VERSION_LATEST);
SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;

//#define LOGGING

#ifdef LOGGING 
ME3TweaksASILogger logger(GAMETAG "ConsoleExtension" VERSION, GAMETAG "ConsoleExtension.txt");
#endif

constexpr auto savedCamsFileName = "savedCams";
constexpr auto savedCamsLength = 100;


FTPOV cachedPOV;
FTPOV povToLoad;
bool shouldSetCamPOV;

FTPOV savedPOVs[savedCamsLength];

void writeCamArray(const std::string& file_name, FTPOV* data)
{
	std::ofstream out;
	out.open(file_name, std::ios::binary);
	out.write(reinterpret_cast<char*>(data), sizeof(FTPOV) * savedCamsLength);
	out.close();
};

void readCamArray(const std::string& file_name, FTPOV* data)
{
	std::ifstream in;
	in.open(file_name, std::ios::binary);
	if (in.good())
	{
		in.read(reinterpret_cast<char*>(data), savedCamsLength * sizeof(FTPOV));
	}
	in.close();
};

#define GET_TOKEN() \
	wchar_t* context = nullptr; \
	const wchar_t* const token = wcstok_s(cmd, seps, &context); \
	if (token == nullptr) { return FALSE; }

typedef unsigned (*tExecHandler)(UEngine* Context, wchar_t* cmd, void* unk);
tExecHandler ExecHandler = nullptr;
tExecHandler ExecHandler_orig = nullptr;
unsigned ExecHandler_hook(UEngine* Context, wchar_t* cmd, void* unk)
{
	const auto seps = L" \t";
	if (ExecHandler_orig(Context, cmd, unk))
	{
		return TRUE;
	}
	if (IsCmd(&cmd, L"savecam"))
	{
		GET_TOKEN()

		if (const int i = _wtoi(token); i >= 0 && i < savedCamsLength)
		{
			savedPOVs[i] = cachedPOV;
			writeCamArray(savedCamsFileName, savedPOVs);
		}
	}
	else if (IsCmd(&cmd, L"loadcam"))
	{
		GET_TOKEN()

		if (const int i = _wtoi(token); i >= 0 && i < savedCamsLength)
		{
			readCamArray(savedCamsFileName, savedPOVs);
			povToLoad = savedPOVs[i];
			shouldSetCamPOV = true;
		}
	}
#ifdef GAMELE3
	//LE1 and LE2 have these commands built in

	else if (IsCmd(&cmd, L"streamlevelin"))
	{
		GET_TOKEN()

		if (const auto cheatMan = reinterpret_cast<UBioCheatManager*>(FindObjectOfType(UBioCheatManager::StaticClass())))
		{
			FName levelName;
			StaticVariables::CreateName(token, &levelName);
			cheatMan->StreamLevelIn(levelName);
		}
	}
	else if (IsCmd(&cmd, L"streamlevelout"))
	{
		GET_TOKEN()

		if (const auto cheatMan = reinterpret_cast<UBioCheatManager*>(FindObjectOfType(UBioCheatManager::StaticClass())))
		{
			FName levelName;
			StaticVariables::CreateName(token, &levelName);
			cheatMan->StreamLevelOut(levelName);
		}
	}
	else if (IsCmd(&cmd, L"onlyloadlevel"))
	{
		GET_TOKEN()

		if (const auto cheatMan = reinterpret_cast<UBioCheatManager*>(FindObjectOfType(UBioCheatManager::StaticClass())))
		{
			FName levelName;
			StaticVariables::CreateName(token, &levelName);
			cheatMan->OnlyLoadLevel(levelName);
		}
	}
#endif

	return FALSE;
}

#ifdef GAMELE3
// This the constructor for StaticConstructObject in LE3
#define PATTERN_CONSTRUCT_OBJECT /*48 8b c4 48 89*/ "50 10 56 57 41 55 41 56 41 57 48 83 ec 70 48 c7 40 b8 fe ff ff ff" // LE3
typedef void* (*tStaticConstructObject) (UClass* Class, UObject* InOuter, FName Name, long long SetFlags, UObject* Template, void* outputDevice, UObject* SubobjectRoot, struct FObjectInstancingGraph* InstanceGraph);
tStaticConstructObject StaticConstructObject = nullptr;
#endif

#ifdef GAMELE3
#define PLAYERTICK_FUNCNAME "Function SFXGame.BioPlayerController.PlayerTick"
#else
#define PLAYERTICK_FUNCNAME "Function Engine.PlayerController.PlayerTick"
#endif

typedef void (*tProcessEvent)(UObject* Context, UFunction* Function, void* Parms, void* Result);
tProcessEvent ProcessEvent = nullptr;
tProcessEvent ProcessEvent_orig = nullptr;
void ProcessEvent_hook(UObject* Context, UFunction* Function, void* Parms, void* Result)
{
	if (IsA<APlayerController>(Context) && isPartOf(Function->GetFullName(), PLAYERTICK_FUNCNAME))
	{
		const auto playerController = static_cast<APlayerController*>(Context);
		cachedPOV = playerController->PlayerCamera->CameraCache.POV;
	}
	else if (shouldSetCamPOV && !strcmp(Function->GetFullName(), "Function Engine.Camera.UpdateCamera"))
	{
		if (const auto camera = static_cast<ACamera*>(Context); IsA<AActor>(camera->PCOwner))
		{
			shouldSetCamPOV = false;
			const auto actor = static_cast<AActor*>(camera->PCOwner);
			actor->LOCATION = povToLoad.LOCATION;
			actor->Rotation = povToLoad.Rotation;
		}
	}
	ProcessEvent_orig(Context, Function, Parms, Result);
}

SPI_IMPLEMENT_ATTACH
{
#ifdef LOGGING
	Common::OpenConsole();
#endif

	INIT_CHECK_SDK()
	INIT_POSTHOOK(ExecHandler, LE_PATTERN_POSTHOOK_EXEC)
	INIT_POSTHOOK(ProcessEvent, LE_PATTERN_POSTHOOK_PROCESSEVENT)
#ifdef GAMELE3
	INIT_FIND_PATTERN_POSTHOOK(StaticConstructObject, PATTERN_CONSTRUCT_OBJECT)
#endif
	return true;
}

SPI_IMPLEMENT_DETACH
{
#ifdef LOGGING
	Common::CloseConsole();
#endif
	return true;
}