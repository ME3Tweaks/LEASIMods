#pragma once

#include <stdio.h>
#include <io.h>
#include <fcntl.h>     /* for _O_TEXT and _O_BINARY */  
#include <string>
#include <fstream>
#include <iostream>
#include <ostream>
#include <streambuf>
#include <cstdarg>
#include <vector>
#include <locale> 
#include <codecvt>
#include <chrono>

#include "../Common.h"

#ifndef _NOSDK

#ifdef GAMELE1
#include "../../LE1-ASI-Plugins/LE1-SDK/SdkHeaders.h"
#define LOCATION Location
#endif
#ifdef GAMELE2
#include "../../LE2-ASI-Plugins/LE2-SDK/SdkHeaders.h"
#define LOCATION Location
#endif
#ifdef GAMELE3
#include "../../LE3-ASI-Plugins/LE3-SDK/SdkHeaders.h"
#define LOCATION location
#endif

#include "Utilities.h"

#endif


using namespace std;

char GetBit(int value, int bit) {
	return (value >> bit) & 1;
}

#ifndef _NOSDK

/// <summary>
/// Gets objects in memory of a specific class type. Returns a TArray with objects that can be casted to that type.
/// </summary>
/// <param name="type"></param>
/// <returns></returns>
TArray<UObject*> FindObjectsOfType(UClass* type)
{
	TArray<UObject*> foundObjects;
	const auto objCount = UObject::GObjObjects()->Count;
	const auto objArray = UObject::GObjObjects()->Data;
	for (auto j = 0; j < objCount; j++)
	{
		auto obj = objArray[j];
		if (obj && obj->IsA(type))
		{
			const auto name = obj->Name.GetName();
			if (strstr(name, "Default_"))// || actor->bStatic || !actor->bMovable)
			{
				continue;
			}
			foundObjects.Add(obj);
		}
	}
	return foundObjects;
}

/// <summary>
/// Gets the first object in memory of the specified type. Ensure you check for NULL.
/// </summary>
/// <param name="type"></param>
/// <returns></returns>
UObject* FindObjectOfType(UClass* type)
{
	const auto objCount = UObject::GObjObjects()->Count;
	const auto objArray = UObject::GObjObjects()->Data;
	for (auto j = 0; j < objCount; j++)
	{
		auto obj = objArray[j];
		if (obj && obj->IsA(type))
		{
			const auto name = obj->Name.GetName();
			if (strstr(name, "Default_"))// || actor->bStatic || !actor->bMovable)
			{
				continue;
			}
			return obj;
		}
	}
	return NULL;
}

/// <summary>
/// Searches for an object with the exact name.
/// </summary>
///	<param name="FullName">The full name of the object to find</param>
/// <returns>First found object, null otherwise</returns>
UObject* FindNamedObject(const char* FullName)
{
	const auto objCount = UObject::GObjObjects()->Count;
	const auto objArray = UObject::GObjObjects()->Data;
	for (auto j = 0; j < objCount; j++)
	{
		auto obj = objArray[j];
		if (obj && _strcmpi(obj->GetFullName(), FullName) == 0)
		{
			return obj;
		}
	}
	return NULL;
}

std::string GuidToString(FGuid guid)
{
	char guid_cstr[39];
	snprintf(guid_cstr, sizeof(guid_cstr),
		"{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
		guid.A, guid.B, guid.C,
		GetBit(guid.D, 0), GetBit(guid.D, 1), GetBit(guid.D, 2), GetBit(guid.D, 3),
		GetBit(guid.D, 4), GetBit(guid.D, 5), GetBit(guid.D, 6), GetBit(guid.D, 7));

	return std::string(guid_cstr);
}
#endif

const std::wstring wstring_format(const wchar_t* const zcFormat, ...) {

	// initialize use of the variable argument array
	va_list vaArgs;
	va_start(vaArgs, zcFormat);

	// reliably acquire the size
	// from a copy of the variable argument array
	// and a functionally reliable call to mock the formatting
	va_list vaArgsCopy;
	va_copy(vaArgsCopy, vaArgs);
	const int iLen = std::vswprintf(NULL, 0, zcFormat, vaArgsCopy);
	va_end(vaArgsCopy);

	// return a formatted string without risking memory mismanagement
	// and without assuming any compiler or platform specific behavior
	std::vector<wchar_t> zc(iLen + 1);
	std::vswprintf(zc.data(), zc.size(), zcFormat, vaArgs);
	va_end(vaArgs);
	return std::wstring(zc.data(), iLen);
}

const std::string string_format(const char* const zcFormat, ...) {

	// initialize use of the variable argument array
	va_list vaArgs;
	va_start(vaArgs, zcFormat);

	// reliably acquire the size
	// from a copy of the variable argument array
	// and a functionally reliable call to mock the formatting
	va_list vaArgsCopy;
	va_copy(vaArgsCopy, vaArgs);
	const int iLen = std::vsnprintf(NULL, 0, zcFormat, vaArgsCopy);
	va_end(vaArgsCopy);

	// return a formatted string without risking memory mismanagement
	// and without assuming any compiler or platform specific behavior
	std::vector<char> zc(iLen + 1);
	std::vsnprintf(zc.data(), zc.size(), zcFormat, vaArgs);
	va_end(vaArgs);
	return std::string(zc.data(), iLen);
}

/// <summary>
/// Converts a wide string to a string
/// </summary>
/// <param name="wstr"></param>
/// <returns></returns>
std::string ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

/// <summary>
/// Converts a string to a wide string
/// </summary>
/// <param name="wstr"></param>
/// <returns></returns>
std::wstring s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

/// <summary>
/// Modified from OT ASI code to just write to file
/// </summary>
class ME3TweaksASILogger
{
public:
	const char* logfname;

	ME3TweaksASILogger(const char* loggername, const char* _logfname) {
		logfname = _logfname;
		fopen_s(&log, logfname, "w");

		boottime = GetTickCount64();
		writeToLog("ME3Tweaks ASI Logger - By Mgamerz\n"s, false);
		auto currentTime = std::chrono::system_clock::now(); // get the time
		auto formattedTime = std::chrono::system_clock::to_time_t(currentTime); // convert it to time_t type (loses some precision)
		writeToLog(string_format("ASI boot time: %s\n", std::ctime(&formattedTime)), false);
		writeToLog(string(loggername) + "\n", false);
		writeToLog(string_format("Logging to %s%s\n", workingdir().c_str(), _logfname), true);
		writeToLog("--------------------------------------------------------\n", false);
	}

	void writeToLog(wstring wstr, bool bTimeStamp, bool newLine = false)
	{
		writeToLog(ws2s(wstr), bTimeStamp, newLine);
	}

	/// <summary>
	/// Writes to the log file.
	/// </summary>
	/// <param name="str"></param>
	/// <param name="bTimeStamp"></param>
	void writeToLog(string str, bool bTimeStamp, bool newLine = false, bool outputToConsole = true) {
		if (bTimeStamp) {
			string timeStamp = getTimestampStr();
			fprintf(log, timeStamp.c_str());
			if (outputToConsole)
				writeMsg(L"%hs", timeStamp.c_str());
			//free((char*)timeStamp);
		}
		/*if (!log) {
			std::cout << "LOG ISNT INITALIZED";
		}*/
		fprintf(log, "%s", str.data());
		if (outputToConsole)
			writeMsg(L"%hs", str.data());

		if (newLine)
		{
			fprintf(log, "\n");
			if (outputToConsole)
				writeMsg(L"\n");
		}

		numLinesWritten++;
		if (numLinesWritten > 10) {
			fflush(log);
			numLinesWritten = 0;
		}
	}

	void writeWideToLog(std::wstring_view wstr, bool bTimeStamp, bool outputToConsole = true) {
		fwprintf(log, L"%s", wstr.data());
		if (outputToConsole)
			writeln(L"%s", wstr.data());
		fflush(log);
	}

	void writeWideLineToLog(std::wstring_view wstr, bool bTimeStamp, bool outputToConsole = true) {
		if (bTimeStamp) {
			wstring timeStamp = getTimestampStrW();
			fwprintf(log, timeStamp.c_str());
			if (outputToConsole)
				writeMsg(L"%s", timeStamp.c_str());
		}
		fwprintf(log, L"%s\n", wstr.data());
		if (outputToConsole)
			writeln(L"%s", wstr.data());
		fflush(log);
	}

	void flush() {
		if (log) {
			fflush(log);
			numLinesWritten = 0;
		}
	}

	void close()
	{
		if (log)
		{
			fflush(log);
			fclose(log);
		}
	}

private:
	int numLinesWritten = 0;
	FILE* log;
	ULONGLONG boottime = 0;

	std::string workingdir()
	{
		char buf[256];
		GetCurrentDirectoryA(256, buf);
		return std::string(buf) + '\\';
	}

	string getTimestampStr() {
		ULONGLONG currenttime = GetTickCount64();
		ULONGLONG secondsSinceBoot = (currenttime - boottime) / 1000;
		int ms = (currenttime - boottime) % 1000;
		return string_format("[%llu.%d] ", secondsSinceBoot, ms);
	}

	wstring getTimestampStrW() {
		ULONGLONG currenttime = GetTickCount64();
		ULONGLONG secondsSinceBoot = (currenttime - boottime) / 1000;
		int ms = (currenttime - boottime) % 1000;
		return wstring_format(L"[%llu.%d] ", secondsSinceBoot, ms);
	}
};


/*Checks if w2 is part of w1*/
bool isPartOf(char* w1, const char* w2)
{
	int i = 0;
	int j = 0;


	while (w1[i] != '\0') {
		if (w1[i] == w2[j])
		{
			int init = i;
			while (w1[i] == w2[j] && w2[j] != '\0')
			{
				j++;
				i++;
			}
			if (w2[j] == '\0') {
				return true;
			}
			j = 0;
		}
		i++;
	}
	return false;
}

std::string wchar2string(wchar_t* str)
{
	std::string mystring;
	while (*str)
		mystring += (char)*str++;
	return  mystring;
}

bool IsDefaultObject(UObject* obj)
{
	return *reinterpret_cast<uint64*>(&obj->ObjectFlags) & 0x0000000000000200;
}