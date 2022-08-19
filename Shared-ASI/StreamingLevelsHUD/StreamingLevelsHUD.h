#include <shlwapi.h>
#include <psapi.h>
#include <string>
#include <fstream>
#include <ostream>
#include <streambuf>
#include <sstream>


#define ASINAME L"StreamingLevelsHUD"
#define MYHOOK "StreamingLevelsHUD_"
#define VERSION L"4.0.0"

#ifdef GAMELE1
#include "../../LE1-ASI-Plugins/LE1-SDK/Interface.h"
SPI_PLUGINSIDE_SUPPORT(ASINAME, VERSION, L"ME3Tweaks", SPI_GAME_LE1, SPI_VERSION_ANY);
#define WINDOWNAME "Mass Effect"
#endif
#ifdef GAMELE2
#include "../../LE2-ASI-Plugins/LE2-SDK/Interface.h"
SPI_PLUGINSIDE_SUPPORT(ASINAME, VERSION, L"ME3Tweaks", SPI_GAME_LE2, SPI_VERSION_ANY);
#define WINDOWNAME "Mass Effect 2"
#endif
#ifdef GAMELE3
#include "../../LE3-ASI-Plugins/LE3-SDK/Interface.h"
SPI_PLUGINSIDE_SUPPORT(ASINAME, VERSION, L"ME3Tweaks", SPI_GAME_LE3, SPI_VERSION_ANY);
#define WINDOWNAME "Mass Effect 3"
#endif

#include "../../Shared-ASI/Common.h"
#include "../../Shared-ASI/ME3Tweaks/ME3TweaksHeader.h"

SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;

// ======================================================================


// ProcessEvent hook
// Renders the HUD for Streaming Levels
// ======================================================================


typedef void (*tProcessEvent)(UObject* Context, UFunction* Function, void* Parms, void* Result);
tProcessEvent ProcessEvent = nullptr;
tProcessEvent ProcessEvent_orig = nullptr;

size_t MaxMemoryHit = 0;
bool DrawSLH = true;
bool CanToggleDrawSLH = false;
char lastTouchedTriggerStream[512];
float textScale = 1.0f;
float lineHeight = 12.0f;
static void RenderTextSLH(std::wstring msg, const float x, const float y, const char r, const char g, const char b, const float alpha, UCanvas* can)
{
	can->SetDrawColor(r, g, b, alpha * 255);
	can->SetPos(x, y + 64); //+ is Y start. To prevent overlay on top of the power bar thing
	can->DrawTextW(FString{ const_cast<wchar_t*>(msg.c_str()) }, 1, textScale, textScale, nullptr);
}

const char* FormatBytes(size_t bytes, char* keepInStackStr)
{
	const char* sizes[4] = { "B", "KB", "MB", "GB" };

	int i;
	double dblByte = bytes;
	for (i = 0; i < 4 && bytes >= 1024; i++, bytes /= 1024)
		dblByte = bytes / 1024.0;

	sprintf(keepInStackStr, "%.2f", dblByte);

	return strcat(strcat(keepInStackStr, " "), sizes[i]);
}

int line = 0;
PROCESS_MEMORY_COUNTERS pmc;

void SetTextScale()
{
	HWND activeWindow = FindWindowA(NULL, WINDOWNAME);
	if (activeWindow)
	{
		RECT rcOwner;
		if (GetWindowRect(activeWindow, &rcOwner))
		{
			long width = rcOwner.right - rcOwner.left;
			long height = rcOwner.bottom - rcOwner.top;

			if (width > 2560 && height > 1440)
			{
				textScale = 2.0f;
			}
			else if (width > 1920 && height > 1080)
			{
				textScale = 1.5f;
			}
			else
			{
				textScale = 1.0f;
			}
			lineHeight = 12.0f * textScale;
		}
	}
}

void ProcessEvent_hook(UObject* Context, UFunction* Function, void* Parms, void* Result)
{
	auto funcFullName = Function->GetFullName();
	if (!strcmp(funcFullName, "Function SFXGame.BioTriggerStream.Touch"))
	{
		auto rparms = reinterpret_cast<ABioTriggerStream_eventTouch_Parms*>(Parms);
		if (rparms->Other->IsA(ASFXPawn_Player::StaticClass()))
		{
			auto bts = reinterpret_cast<ABioTriggerStream*>(Context);
			strcpy(lastTouchedTriggerStream, bts->GetFullPath());
		}
	}
	else if (!strcmp(Function->Name.GetName(), "DrawHUD"))
	{
		//writeln(L"Func: %hs", Function->GetFullName());
		auto hud = reinterpret_cast<AHUD*>(Context);
		if (hud != nullptr)
		{
			hud->Canvas->SetDrawColor(0, 0, 0, 255);
			hud->Canvas->SetPos(0, 0);
			hud->Canvas->DrawBox(1920, 1080);
		}
	}
	else if (!strcmp(funcFullName, "Function SFXGame.BioHUD.PostRender"))
	{
		line = 0;
		auto hud = reinterpret_cast<ABioHUD*>(Context);
		if (hud != nullptr)
		{
			// Toggle drawing/not drawing
			if ((GetKeyState('T') & 0x8000) && (GetKeyState(VK_CONTROL) & 0x8000)) {
				if (CanToggleDrawSLH) {
					CanToggleDrawSLH = false; // Will not activate combo again until you re-press combo
					DrawSLH = !DrawSLH;
				}
			}
			else
			{
				if (!(GetKeyState('T') & 0x8000) || !(GetKeyState(VK_CONTROL) & 0x8000)) {
					CanToggleDrawSLH = true; // can press key combo again
				}
			}

			// Render mem usage
			if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
			{
				unsigned char r = 0;
				unsigned char g = 255;

				// Flash if high crit
				char str[18] = ""; //Allocate
				float alpha = 1.0f;

				if (DrawSLH)
				{
					wchar_t objectName[512];
					swprintf_s(objectName, 512, L"Memory usage: %S (%llu bytes)", FormatBytes(pmc.PagefileUsage, str), pmc.PagefileUsage);
					RenderTextSLH(objectName, 5.0f, line * lineHeight, r, g, 0, alpha, hud->Canvas);
				}
				line++;

				// Max Hit
				if (pmc.PagefileUsage > MaxMemoryHit)
				{
					MaxMemoryHit = pmc.PagefileUsage;
				}

				if (DrawSLH) {
					wchar_t objectName[512];
					swprintf_s(objectName, 512, L"Max memory hit: %S (%llu bytes)", FormatBytes(MaxMemoryHit, str), MaxMemoryHit);
					RenderTextSLH(objectName, 5.0f, line * lineHeight, r, g, 0, alpha, hud->Canvas);
				}

				line++;
			}

			if (DrawSLH && hud->WorldInfo)
			{
				SetTextScale();
				int yIndex = 3; //Start at line 3 (starting at 0)

				//screenLogger->ClearMessages();
				//logger.writeToConsoleOnly(string_format("Number of streaming levels: %d\n", hud->WorldInfo->StreamingLevels.Count), true);
				wchar_t lastHit[600];
				swprintf_s(lastHit, 600, L"Last BioTriggerStream: %hs", lastTouchedTriggerStream);
				RenderTextSLH(lastHit, 5, yIndex * lineHeight, 0, 255, 64, 1.0f, hud->Canvas);
				yIndex++;

				if (hud->WorldInfo->StreamingLevels.Any()) {
					for (int i = 0; i < hud->WorldInfo->StreamingLevels.Count; i++) {
						std::wstringstream ss;
						ULevelStreaming* sl = hud->WorldInfo->StreamingLevels.Data[i];
						if (sl->bShouldBeLoaded || sl->bIsVisible) {
							unsigned char r = 255;
							unsigned char g = 255;
							unsigned char b = 255;

							if (!sl->bIsVisible && sl->bHasLoadRequestPending)
							{
								ss << ">> ";
							}

							ss << sl->PackageName.GetName();
							if (sl->PackageName.Number > 0)
							{
								ss << "_" << sl->PackageName.Number - 1;
							}
							if (sl->bIsVisible)
							{
								ss << " Visible";
								r = 0;
								g = 255;
								b = 0;
							}
							else if (sl->bHasLoadRequestPending)
							{
								ss << " Loading";
								r = 255;
								g = 229;
								b = 0;
							}
							else if (sl->bHasUnloadRequestPending)
							{
								ss << " Unloading";
								r = 0;
								g = 255;
								b = 229;
							}
							else if (sl->bShouldBeLoaded && sl->LoadedLevel)
							{
								ss << " Loaded";
								r = 255;
								g = 255;
								b = 0;
							}
							else if (sl->bShouldBeLoaded)
							{
								ss << " Pending load";
								r = 185;
								g = 169;
								b = 0;
							}
							const std::wstring msg = ss.str();
							RenderTextSLH(msg, 5, yIndex * lineHeight, r, g, b, 1.0f, hud->Canvas);
							yIndex++;
						}
					}
				}
			}
		}
	}

	ProcessEvent_orig(Context, Function, Parms, Result);
}

// ======================================================================


SPI_IMPLEMENT_ATTACH
{
	auto _ = SDKInitializer::Instance();

	INIT_FIND_PATTERN_POSTHOOK(ProcessEvent, LE_PATTERN_POSTHOOK_PROCESSEVENT);
	INIT_HOOK_PATTERN(ProcessEvent);

	return true;
}

SPI_IMPLEMENT_DETACH
{
	return true;
}
