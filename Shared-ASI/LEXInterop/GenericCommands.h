#pragma once

#include "../ConsoleCommandParsing.h"
#include "../ME3Tweaks/ME3TweaksHeader.h"
#include "../Common.h"
#include "../StaticVariablePointers.h"
#include "InteropActionQueue.h"

struct CauseEventAction final : ActionBase
{
	wstring EventName;

	explicit CauseEventAction(wstring eventName) : EventName(std::move(eventName)){}

	void Execute() override
	{
		if (const auto player = reinterpret_cast<ABioPlayerController*>(FindObjectOfType(ABioPlayerController::StaticClass())))
		{
			FName foundName;
			StaticVariables::CreateName(EventName.c_str(), 0, &foundName);
			player->CauseEvent(foundName);
		}
	}
};

struct RemoteEventAction final : ActionBase
{
	wstring EventName;

	explicit RemoteEventAction(wstring eventName) : EventName(std::move(eventName)) {}

	void Execute() override
	{
		if (const auto bioWorldInfo = reinterpret_cast<ABioWorldInfo*>(FindObjectOfType(ABioWorldInfo::StaticClass())))
		{
			FName foundName;
			StaticVariables::CreateName(EventName.c_str(), 0, &foundName);
			bioWorldInfo->eventCauseEvent(foundName);
		}
	}
};

struct ConsoleCommandAction final : ActionBase
{
	wstring Command;

	explicit ConsoleCommandAction(wstring command) : Command(std::move(command)) {}

	void Execute() override
	{
		writeln(L"Console command: %s", Command.c_str());

		//printf(eventName);
		/*auto playerController = reinterpret_cast<APlayerController*>(FindObjectOfType(APlayerController::StaticClass()));
		if (playerController)
		{
			playerController->ConsoleCommand(Command, false);
		}

		auto console = reinterpret_cast<UConsole*>(FindObjectOfType(UConsole::StaticClass()));
		if (console)
		{
			console->ConsoleCommand(Command);
		}

		auto viewport = reinterpret_cast<UGameViewportClient*>(FindObjectOfType(UGameViewportClient::StaticClass()));
		if (viewport)
		{
			viewport->ConsoleCommand(Command);
		}*/

		if (const auto player = reinterpret_cast<ASFXPawn_Player*>(FindObjectOfType(ASFXPawn_Player::StaticClass())))
		{
			player->ConsoleCommand(FString(const_cast<wchar_t*>(Command.c_str())), false);
		}
	}
};

#ifdef GAMELE1 
struct CachePackageAction final : ActionBase
{
	wstring PackageName;

	explicit CachePackageAction(wstring packageName) : PackageName(std::move(packageName)) {}

	void Execute() override
	{
		CacheContent(CacheContentWrapperClassPointer, PackageName.data(), true, true);
	}
};
#endif

struct StreamLevelAction final : ActionBase
{
	enum StreamChangeKind
	{
		StreamOut,
		StreamIn,
		OnlyLoad
	};

	wstring LevelName;
	StreamChangeKind ChangeKind;

	explicit StreamLevelAction(wstring levelName, const StreamChangeKind changeKind): LevelName(std::move(levelName)), ChangeKind(changeKind) {}

	void Execute() override
	{
		if (const auto cheatMan = reinterpret_cast<UBioCheatManager*>(FindObjectOfType(UBioCheatManager::StaticClass())))
		{
			FName levelName;
			StaticVariables::CreateName(LevelName.c_str(), 0, &levelName);
			switch (ChangeKind) {
			case StreamOut:
				cheatMan->StreamLevelOut(levelName);
				break;
			case StreamIn:
				cheatMan->StreamLevelIn(levelName);
				break;
			case OnlyLoad:
				cheatMan->OnlyLoadLevel(levelName);
				break;
			}
		}
	}
};

class GenericCommands {
public:
	static bool HandleCommand(char* command)
	{
#ifdef GAMELE1 
		// 'CACHEPACKAGE <PackageFileFullPath>'
		// Registers a package so game methods can find it
		if (IsCmd(&command, "CACHEPACKAGE "))
		{
			InteropActionQueue.push(new CachePackageAction(s2ws(command)));
			return true;
		}
#endif

		// 'CAUSEEVENT <EventName>'
		// Causes a ConsoleEvent to occur in kismet
		if (IsCmd(&command, "CAUSEEVENT "))
		{
			InteropActionQueue.push(new CauseEventAction(s2ws(command)));
			return true;
		}

		// 'CAUSEEVENT <EventName>'
		// Causes a ConsoleEvent to occur in kismet
		if (IsCmd(&command, "REMOTEEVENT "))
		{
			InteropActionQueue.push(new RemoteEventAction(s2ws(command)));
			return true;
		}

		// 'CONSOLECOMMAND <Command String>'
		// Sends a console command to be executed in the context of PlayerController
		if (IsCmd(&command, "CONSOLECOMMAND "))
		{
			InteropActionQueue.push(new ConsoleCommandAction(s2ws(command)));
			return true;
		}
		
		// 'STREAMLEVELIN <LevelFileName>'
		// Streams a level in and sets it to visible
		if (IsCmd(&command, "STREAMLEVELIN "))
		{
			InteropActionQueue.push(new StreamLevelAction(s2ws(command), StreamLevelAction::StreamIn));
			return true;
		}

		// 'STREAMLEVELOUT <LevelFileName>'
		// Streams a level out and removes it from the current level
		if (IsCmd(&command, "STREAMLEVELOUT "))
		{
			InteropActionQueue.push(new StreamLevelAction(s2ws(command), StreamLevelAction::StreamOut));
			return true;
		}

		// 'ONLYLOADLEVEL <LevelFileName>'
		// Streams a level in without setting it to visible
		if (IsCmd(&command, "ONLYLOADLEVEL "))
		{
			InteropActionQueue.push(new StreamLevelAction(s2ws(command), StreamLevelAction::OnlyLoad));
			return true;
		}

  // We did not handle this command.
		return false;
	}
};
