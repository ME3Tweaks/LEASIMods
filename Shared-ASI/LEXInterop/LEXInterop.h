#pragma once
#ifndef MYHOOK
#define MYHOOK "LEXInterop_"

#include "../../Shared-ASI/Common.h"
#include "../../Shared-ASI/Interface.h"
#include "../../Shared-ASI/ME3Tweaks/ME3TweaksHeader.h"
#include "UtilityMethods.h"

// We should have a #define to make string formatting not suck

// Checks if a string starts with another
bool startsWith(const char* pre, const char* str)
{
	size_t lenpre = strlen(pre),
		lenstr = strlen(str);
	return lenstr < lenpre ? false : memcmp(pre, str, lenpre) == 0;
}

// Checks if a string starts with another
bool stringStartsWith(const char* prefix, const std::string& fullstring)
{
	return fullstring.rfind(prefix, 0) == 0;
}

// VARIABLE LOOKUP
// Searches for the specified byte pattern, which is a 7-byte mov or lea instruction, with the 'source' operand being the address being calculated
void* findAddressLeaMov(ISharedProxyInterface* InterfacePtr, const char* name, const char* bytePattern)
{
	void* patternAddr;
	if (const auto rc = InterfacePtr->FindPattern(&patternAddr, bytePattern);
		rc != SPIReturn::Success)
	{
		writeln(L"Failed to find %hs pattern: %d / %s", name, rc, SPIReturnToString(rc));
		return nullptr; // Will be 0
	}

	// This is the address of the instruction
	const auto instruction = static_cast<BYTE*>(patternAddr);
	const auto RIPaddress = instruction + 7; // Relative Instruction Pointer (after instruction)
	const auto offset = *reinterpret_cast<int32_t*>(instruction + 3); // Offset listed in the instruction
	return RIPaddress + offset; // Added together we get the actual address
}

// Populates common class pointers
// This can probably be improved to only require one lookup and then offset with that
void LoadCommonClassPointers(ISharedProxyInterface* InterfacePtr)
{
	// 0x7ff71216d5ef DRM free | MOV RAX, qword ptr [GPackageFileCache]
	//auto addr = findAddressLeaMov(InterfacePtr, "GPackageFileCache", "48 8b 05 ca 8c 5e 01 48 8b 08 4c 8b 71 10 83 7b 08 ff 75 46");
	//if (addr != nullptr)
	//{
	//	GPackageFileCache = static_cast<void*>(addr);
	//	writeln("Found GPackageFileCache at %p", GPackageFileCache);
	//}
	//else
	//{
	//	writeln(" >> FAILED TO FIND GPackageFileCache!");
	//}
}


// Additional includes in this single header
// Global data handlers
#include "SharedData.h"
#include "StaticVariablePointers.h"

#include "LEXCommunications.h"
#include "LE1AnimViewer.h"
#include "LE1GenericCommands.h"
#include "LEPathfindingGPS.h"
#include "LELiveLevelEditor.h"

#include "UtilityMethods.h"

#include "LEXPipe.h"

#endif