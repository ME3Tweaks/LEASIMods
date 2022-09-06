#pragma once
#include <sstream>
#include <memoryapi.h>
#include <processthreadsapi.h>

#include "../Common.h"
#include "../ME3Tweaks/ME3TweaksHeader.h"

// We should have a #define to make string formatting not suck

// Checks if a string starts with another
bool startsWith(const char* pre, const char* str)
{
	const size_t lenpre = strlen(pre);
	const size_t lenstr = strlen(str);
	return lenstr < lenpre ? false : memcmp(pre, str, lenpre) == 0;
}

// Checks if a string starts with another
bool stringStartsWith(const char* prefix, const std::string& fullstring)
{
	return fullstring.rfind(prefix, 0) == 0;
}

// Populates common class pointers
// This can probably be improved to only require one lookup and then offset with that
void LoadCommonClassPointers()
{
	// 0x7ff71216d5ef DRM free | MOV RAX, qword ptr [GPackageFileCache]
	//auto addr = findAddressLeaMov("GPackageFileCache", "48 8b 05 ca 8c 5e 01 48 8b 08 4c 8b 71 10 83 7b 08 ff 75 46");
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


// Not sure this works!
bool NopOutMemory(void* startPos, const int patchSize)
{
	//make the memory we're going to patch writeable
	DWORD  oldProtect;
	if (!VirtualProtect(startPos, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect))
		return false;

	// Nop it out
	writeln(L"Nopping memory: %p for %i bytes", startPos, patchSize);
	for (int i = 0; i < patchSize; i++)
	{
		*(char*)((int64)startPos + i) = 0x90; // This line only has 3 underlines in visual studio
	}

	//restore the memory's old protection level
	VirtualProtect(startPos, patchSize, oldProtect, &oldProtect);
	FlushInstructionCache(GetCurrentProcess(), startPos, patchSize);
	return true;
}

bool PatchMemory(void* address, const void* patch, const SIZE_T patchSize)
{
	//make the memory we're going to patch writeable
	DWORD  oldProtect;
	if (!VirtualProtect(address, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect))
		return false;

	//overwrite with our patch
	memcpy(address, patch, patchSize);

	//restore the memory's old protection level
	VirtualProtect(address, patchSize, oldProtect, &oldProtect);
	FlushInstructionCache(GetCurrentProcess(), address, patchSize);
	return true;
}

char* GetUObjectClassName(const UObject* object)
{
	static char cOutBuffer[256];

	sprintf_s(cOutBuffer, "%s", object->Class->GetName());

	return cOutBuffer;
}

FName GetContainingMapName(const UObject* object)
{
	const UObject* obj = object;
	FName name{}; //0-initialized FName is "None"
	while (obj->Class && obj->Outer)
	{
		obj = obj->Outer;
		name = obj->Name;
	}
	return name;
}

// Gets the string up to the end of the next space. The input parameter will be modified.
std::string GetCommandParam(std::string& commandStr)
{
	const auto spacePos = commandStr.find(' ', 0);
	if (spacePos == string::npos)
	{
		return commandStr; // Return the rest of the string
	}

	auto param = commandStr.substr(0, spacePos);
	commandStr = commandStr.substr(spacePos + 1, commandStr.length() - 1 - spacePos);
	return param;
}

inline std::wostream& operator<< (std::wostream& out, FString const& fString)
{
	out.write(fString.Data, fString.Count);
	return out;
}

// Strings sent to LEX should use the following format:
// [FEATURENAME] [Args...]
// e.g. PATHFINDING_GPS PLAYERLOC=1,2,3
void SendStringToLEX(const wstring& wstr, const UINT timeout = 100) {
	if (const auto handle = FindWindow(nullptr, L"Legendary Explorer"))
	{
		constexpr unsigned long SENT_FROM_LE1 = 0x02AC00C7;
		constexpr unsigned long SENT_FROM_LE2 = 0x02AC00C6;
		constexpr unsigned long SENT_FROM_LE3 = 0x02AC00C5;
		COPYDATASTRUCT cds;
		ZeroMemory(&cds, sizeof(COPYDATASTRUCT));

#if defined GAMELE1
		cds.dwData = SENT_FROM_LE1;
#elif defined GAMELE2
		cds.dwData = SENT_FROM_LE2;
#elif defined GAMELE3
		cds.dwData = SENT_FROM_LE3;
#endif
		cds.cbData = (wstr.length() + 1) * sizeof(wchar_t);
		cds.lpData = PVOID(wstr.c_str());
		SendMessageTimeout(handle, WM_COPYDATA, NULL, reinterpret_cast<LPARAM>(&cds), 0, timeout, nullptr);
	}
}

void SetSequenceString(USequenceOp* op, const wchar_t* linkName, wchar_t* value)
{
	const auto numVarLinks = op->VariableLinks.Num();
	for (auto i = 0; i < numVarLinks; i++)
	{
		const auto& varLink = op->VariableLinks(i);
		for (auto j = 0; j < varLink.LinkedVariables.Count; ++j)
		{
			const auto seqVar = varLink.LinkedVariables(j);
			if (!_wcsnicmp(varLink.LinkDesc.Data, linkName, 256) && IsA<USeqVar_String>(seqVar))
			{
				auto* strVar = reinterpret_cast<USeqVar_String*>(seqVar);
				auto strVal = strVar->StrValue;
				strVal.Data = value;
				strVal.Count = wcslen(value);
				strVar->StrValue = strVal; // Value is updated but kismet behavior doesn't change
			}
		}
	}
}

void SendMessageToLEX(USequenceOp* op)
{
	wstringstream wss;
	const auto numVarLinks = op->VariableLinks.Num();
	for (auto i = 0; i < numVarLinks; i++)
	{
		const auto& varLink = op->VariableLinks(i);
		for (auto j = 0; j < varLink.LinkedVariables.Count; ++j)
		{
			const auto seqVar = varLink.LinkedVariables(j);
			if (!_wcsnicmp(varLink.LinkDesc.Data, L"MessageName", 12) && IsA<USeqVar_String>(seqVar))
			{
				const USeqVar_String* strVar = static_cast<USeqVar_String*>(seqVar);
				wss << strVar->StrValue;
			}
			else if (!_wcsnicmp(varLink.LinkDesc.Data, L"String", 7) && IsA<USeqVar_String>(seqVar))
			{
				const USeqVar_String* strVar = static_cast<USeqVar_String*>(seqVar);
				wss << L" string " << strVar->StrValue;
			}
			else if (!_wcsnicmp(varLink.LinkDesc.Data, L"Vector", 7) && IsA<USeqVar_Vector>(seqVar))
			{
				const USeqVar_Vector* vectorVar = static_cast<USeqVar_Vector*>(seqVar);
				wss << L" vector " << vectorVar->VectValue.X << L" " << vectorVar->VectValue.Y << L" " << vectorVar->VectValue.Z;
			}
			else if (!_wcsnicmp(varLink.LinkDesc.Data, L"Float", 5) && IsA<USeqVar_Float>(seqVar))
			{
				const USeqVar_Float* floatVar = static_cast<USeqVar_Float*>(seqVar);
				wss << L" float " << floatVar->FloatValue;
			}
			else if (!_wcsnicmp(varLink.LinkDesc.Data, L"Int", 3) && IsA<USeqVar_Int>(seqVar))
			{
				const USeqVar_Int* intVar = static_cast<USeqVar_Int*>(seqVar);
				wss << L" int " << intVar->IntValue;
			}
			else if (!_wcsnicmp(varLink.LinkDesc.Data, L"Bool", 4) && IsA<USeqVar_Bool>(seqVar))
			{
				const USeqVar_Bool* boolVar = static_cast<USeqVar_Bool*>(seqVar);
				wss << L" bool " << boolVar->bValue;
			}
		}
	}
	const wstring msg = wss.str();
	SendStringToLEX(msg);
}


// Utility methods
float ToRadians(const int unrealRotationUnits)
{
	return unrealRotationUnits * 360.0f / 65536.0f * 3.1415926535897931f / 180.0f;
}

// Game utility methods
#ifdef GAMELE1
typedef BOOL(*tCachePackage)(void* parm1, wchar_t* filePath, bool replaceIfExisting, bool warnIfExists);
tCachePackage CacheContent = nullptr;

tCachePackage CacheContentWrapper = nullptr; // capture pointer
tCachePackage CacheContentWrapper_orig = nullptr;
void* CacheContentWrapperClassPointer = nullptr; // Pointer we will use when we call the orig method on our own
BOOL CacheContentWrapper_hook(void* parm1, wchar_t* filePath, bool replaceIfExisting, bool warnIfExists)
{
	CacheContentWrapperClassPointer = parm1;
	return CacheContentWrapper_orig(parm1, filePath, replaceIfExisting, warnIfExists);
}
#endif