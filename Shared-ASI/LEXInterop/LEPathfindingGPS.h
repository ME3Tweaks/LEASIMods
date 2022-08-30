#pragma once
#include "../ConsoleCommandParsing.h"
#include "../ME3Tweaks/ME3TweaksHeader.h"
#include "../Common.h"
#include "UtilityMethods.h"

class LEPathfindingGPS
{
public:
	// If the GPS is currently active and should be sending messages to LEX
	static bool playerGPSActive;

	// Return true if handled.
	// Return false if not handled.
	static bool HandleCommand(char* command)
	{
		if (IsCmd(&command, "ACTIVATE_PLAYERGPS"))
		{
			writeln("Activating player GPS");
			playerGPSActive = true;
			return true;
		}
		if (IsCmd(&command, "DEACTIVATE_PLAYERGPS"))
		{
			writeln("Deactivating player GPS");
			playerGPSActive = false;
			return true;
		}

		return false; // Was not handled
	}

	// Return true if other features should also be able to handle this function call
	// Return false if other features shouldn't be able to also handle this function call
	static bool ProcessEvent(UObject* Context, UFunction* Function, void* Parms, void* Result)
	{
		if (playerGPSActive && IsA<ABioPlayerController>(Context) && strcmp(Function->GetName(), "PlayerTick") == 0)
		{
			if (const auto playerController = static_cast<ABioPlayerController*>(Context)) {
				// PLAYER GPS
				if (playerController->Pawn != nullptr)
				{
					// What happens when you don't have an interpolation method
					std::wstringstream ss;
					ss << "PATHFINDING_GPS PLAYERLOC=" << playerController->Pawn->LOCATION.X << "," << playerController->Pawn->LOCATION.Y << "," << playerController->Pawn->LOCATION.Z;
					SendStringToLEX(ss.str());

					std::wstringstream ss2;
					ss2 << "PATHFINDING_GPS PLAYERROT=" << playerController->Pawn->Rotation.Pitch << "," << playerController->Pawn->Rotation.Yaw << "," << playerController->Pawn->Rotation.Roll;
					SendStringToLEX(ss2.str());
				}
			}
		}

		return true; // Other features can use this function call
	}
};

// Static variable initialization
bool LEPathfindingGPS::playerGPSActive = false;
