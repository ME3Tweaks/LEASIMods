#pragma once

#include <string>

#ifdef GAMELE1
#include "../../LE1-ASI-Plugins/LE1-SDK/SdkHeaders.h"
#endif
#ifdef GAMELE2
#include "../../LE2-ASI-Plugins/LE2-SDK/SdkHeaders.h"
#endif
#ifdef GAMELE3
#include "../../LE3-ASI-Plugins/LE3-SDK/SdkHeaders.h"
#endif

using namespace std;
using BYTE = unsigned char;

//adapted from WarrantyVoider's ME3OnTheHook
class ScreenLogger
{
	wchar_t* Name;
	wchar_t* MessageBuffer[20];

	static void RenderText(wchar_t* msg, const float x, const float y, const float r, const float g, const float b, UCanvas* can)
	{
		can->SetDrawColor(BYTE(r) * 255, BYTE(g) * 255, BYTE(b) * 255, 255);
		can->SetPos(x, y);
		FLinearColor drawColor;
		drawColor.R = r;
		drawColor.G = g;
		drawColor.B = b;
		drawColor.A = 1.0f;
		FVector2D glowBorder;
		glowBorder.X = 2;
		glowBorder.Y = 2;
		FFontRenderInfo renderInfo;
		renderInfo.bClipText = true;
		renderInfo.bEnableShadow = true;
		renderInfo.GlowInfo.bEnableGlow = false;
		renderInfo.GlowInfo.GlowColor = drawColor;
		renderInfo.GlowInfo.GlowInnerRadius = glowBorder;
		renderInfo.GlowInfo.GlowOuterRadius = glowBorder;
		can->DrawText(FString(msg), 1, 1.0f, 1.0f, &renderInfo);
	}
public:

	ScreenLogger(wchar_t* name) : Name(name)
	{
		ZeroMemory(MessageBuffer, sizeof(MessageBuffer));
	}

	~ScreenLogger()
	{
		for (auto& msg : MessageBuffer)
		{
			if (msg)
			{
				free(msg);
			}
		}
	}
	
	void PostRenderer(ABioHUD* hud) const
	{
		RenderText(Name, 0, 0, 0, 1, 0, hud->Canvas);
		for (auto i = 0; i < 20; i++)
		{
			if (MessageBuffer[i])
			{
				RenderText(MessageBuffer[i], 0, float(i * 12 + 12), 0, 1, 0, hud->Canvas);
			}
		}
	}

	void LogMessage(const wstring& text)
	{
		const auto mtext = static_cast<wchar_t*>(malloc(sizeof(wchar_t) * 1024));
		swprintf(mtext, 1024, L"%s", text.c_str());
		if (MessageBuffer[19])
		{
			free(MessageBuffer[19]);
		}
		for (auto i = 19; i > 0; i--)
		{
			MessageBuffer[i] = MessageBuffer[i - 1];
		}
		MessageBuffer[0] = mtext;
	}
};