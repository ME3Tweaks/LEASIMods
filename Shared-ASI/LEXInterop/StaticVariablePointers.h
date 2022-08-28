#pragma once

#include "../Common.h"
#include "../ME3Tweaks/ME3TweaksHeader.h"
#include "LEXInterop.h"
#include "SharedData.h"

// Typedefs
typedef void (*tSFXNameConstructor)(FName* outValue, const wchar_t* nameValue, int nameNumber, BOOL createIfNotFoundMaybe, BOOL unk2);

class StaticVariables
{
private:
	// Function pointers
	static tSFXNameConstructor sfxNameConstructor;

	// Object pointers
	static UWorld* GWorldPtr;

public:
	static UWorld* GWorld() {
		if (GWorldPtr) return GWorldPtr;
#ifdef GAMELE1
		constexpr auto byte_pattern = "48 8b 0d 6b 2b 47 01 e8 2e 63 30 00 0f 28 f8 48 8b 0d 5c 2b 47 01";
#elif defined(GAMELE2)
#error Find GWorld for LE2!
#elif defined(GAMELE3)
#error Find GWorld for LE3!
#endif
		GWorldPtr = static_cast<UWorld*>(findAddressLeaMov(SharedData::SPIInterfacePtr, "GWorld", byte_pattern));
		return GWorldPtr;
	}

	// Creates a new name in the game process.
	// Note: return value is not used; it only indicates if locating the name generation function succeeded
	static BOOL CreateName(const wchar_t* name, const int number, FName* outName)
	{
		if (sfxNameConstructor == nullptr)
		{
			// This is so we can use the macro for slightly cleaner code.
			const auto InterfacePtr = SharedData::SPIInterfacePtr;

			// Byte signature is the same across all 3 games
			INIT_FIND_PATTERN_POSTHOOK(sfxNameConstructor, /*"40 55 56 57 41*/ "54 41 55 41 56 41 57 48 81 ec 00 07 00 00");
		}

		sfxNameConstructor(outName, name, number, TRUE, 0);
		return TRUE; // We made a name
	}
};
UWorld* StaticVariables::GWorldPtr = nullptr;
tSFXNameConstructor StaticVariables::sfxNameConstructor = nullptr;


