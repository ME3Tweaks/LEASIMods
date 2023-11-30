#pragma once

#include "Common.h"
#include "ME3Tweaks/ME3TweaksHeader.h"

// Typedefs
typedef void (*tSFXNameConstructor)(FName* outValue, const wchar_t* nameValue, int nameNumber, BOOL createIfNotFoundMaybe, BOOL unk2);

class StaticVariables
{
	// Function pointers
	static tSFXNameConstructor sfxNameConstructor;

	// Object pointers
	static UWorld** GWorldPtr;

public:

	static UWorld* GWorld() {
		if (GWorldPtr) return *GWorldPtr;
		constexpr auto byte_pattern =
#ifdef GAMELE1 
			"48 8b 0d 6b 2b 47 01 e8 2e 63 30 00 0f 28 f8 48 8b 0d 5c 2b 47 01";
#elif defined(GAMELE2)
			"48 8b 0d e6 a7 64 01 e8 a1 51 7b 00";
#elif defined(GAMELE3)
			"48 8b 0d ee 33 78 01 e8 21 f3 78 00";
#endif
		GWorldPtr = static_cast<UWorld**>(findAddressLeaMov("GWorld", byte_pattern));
		return *GWorldPtr;
	}

	// Creates a new name in the game process.
	// Note: return value is not used; it only indicates if locating the name generation function succeeded
	static BOOL CreateName(const wchar_t* name, const int number, FName* outName)
	{
		if (sfxNameConstructor == nullptr)
		{
			// This is so we can use the macro for slightly cleaner code.
			const auto InterfacePtr = ISharedProxyInterface::SPIInterfacePtr;

			// Byte signature is the same across all 3 games
			INIT_FIND_PATTERN_POSTHOOK(sfxNameConstructor, /*"40 55 56 57 41*/ "54 41 55 41 56 41 57 48 81 ec 00 07 00 00");
		}

		sfxNameConstructor(outName, name, number, TRUE, 0);
		return TRUE; // We made a name
	}

	static BOOL CreateName(const wchar_t* name, FName* outName)
	{
		// This may be more expensive but it's less
		// nonsense code.
		// It's still a bunch of nonsense.
		auto str = std::wstring(name);
		auto lastIndex = str.find_last_of(L"_");
		if (lastIndex > 0 && lastIndex < str.length() - 1) // > 0 because we can't have empty name, -1 because it must have at least one digit.
		{
			auto baseStr = str.substr(0, lastIndex);
			auto numStr = str.substr(lastIndex + 1);
			if (numStr.length() > 0 && numStr.substr(0, 1) == wstring(L"0")) 
			{
				// Thane_02 is not an indexed name, it's the string.
				return CreateName(name, 0, outName);
			}

			// c++ doesn't really have a tryparseint.
			try
			{
				size_t numRead;
				int idx = std::stoi(numStr, &numRead);
				if (idx >= 0 && numRead == numStr.length())
				{
					return CreateName(baseStr.c_str(), idx, outName);
				}
			}
			catch (...)
			{
				// Don't really care what the error is.
			}
		}

		// Just do the default
		return CreateName(name, 0, outName);
	}
};
UWorld** StaticVariables::GWorldPtr = nullptr;
tSFXNameConstructor StaticVariables::sfxNameConstructor = nullptr;
