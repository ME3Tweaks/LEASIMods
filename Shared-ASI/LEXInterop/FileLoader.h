#pragma once

#include "InteropActionQueue.h"
#include "../Common.h"
#include "../ME3Tweaks/ME3TweaksHeader.h"
#include "../ConsoleCommandParsing.h"


class ThreadSafeCounter
{
public:
	ThreadSafeCounter() : Counter(0) {}

	void Increment()
	{
		InterlockedIncrement((LPLONG)&Counter);
	}

	void Decrement()
	{
		InterlockedDecrement((LPLONG)&Counter);
	}

private:
	ThreadSafeCounter(const ThreadSafeCounter& Other) {}
	void operator=(const ThreadSafeCounter& Other) {}

	volatile int Counter;
};

struct FPackagePreCacheInfo
{
	ThreadSafeCounter* SynchronizationObject;
	void* PackageData;
	int PackageDataSize;

	FPackagePreCacheInfo() : SynchronizationObject(nullptr), PackageData(nullptr), PackageDataSize(0) {}

	~FPackagePreCacheInfo()
	{
		if (SynchronizationObject)
		{
			GMalloc.Free(SynchronizationObject);
		}
	}
};


//Must be initialized in the asi's attach function
inline TMap<FString, FPackagePreCacheInfo>* PackagePrecacheMap;

struct AddToPrecacheMapAction final : ActionBase
{
	wstring FileName;
	void* PackageData;
	int PackageSize;

	explicit AddToPrecacheMapAction(wstring fileName, void* packageData, int packageSize) :
		FileName(std::move(fileName)), PackageData(packageData), PackageSize(packageSize)
	{
	}

	void Execute() override
	{
		FString fileName = MakeCopyOfFString(FileName.data());

		//Set would replace the data, but doing so would leak memory. 
		if (FPackagePreCacheInfo* existingPrecacheInfo = PackagePrecacheMap->Find(&fileName); existingPrecacheInfo)
		{
			existingPrecacheInfo->SynchronizationObject->Increment();

			GMalloc.Free(existingPrecacheInfo->PackageData);
			existingPrecacheInfo->PackageData = PackageData;
			existingPrecacheInfo->PackageDataSize = PackageSize;
			existingPrecacheInfo->SynchronizationObject->Decrement();
			GMalloc.Free(fileName.Data);
		}
		else
		{
			FPackagePreCacheInfo& precacheInfo = PackagePrecacheMap->Set(fileName, FPackagePreCacheInfo());

			precacheInfo.SynchronizationObject = new(GMalloc.Malloc(4)) ThreadSafeCounter;
			precacheInfo.SynchronizationObject->Increment();

			precacheInfo.PackageData = PackageData;
			precacheInfo.PackageDataSize = PackageSize;

			precacheInfo.SynchronizationObject->Decrement();
		}
	}
};

class FileLoader {

public:

	static bool HandleCommand(char* command, DWORD bytesRead, const HANDLE pipe)
	{LRESULT
		const char* start = command;
		if (IsCmd(&command, "PRECACHE_FILE "))
		{
			const char* fileName = strtok_s(command, " ", &command);
			const auto packageSize = strtol(command, &command, 10);

			//move past the null terminator to the packageData
			command += strlen(command) + 1;

			void* packageData = GMalloc.Malloc(packageSize);

			const auto remainingBytesInCmdBuff = bytesRead - (command - start);

			if (packageSize <= remainingBytesInCmdBuff)
			{
				memcpy(packageData, command, packageSize);
			}
			else
			{
				auto dataPtr = static_cast<BYTE*>(packageData);
				const auto endPtr = dataPtr + packageSize;
				memcpy(packageData, command, remainingBytesInCmdBuff);
				dataPtr += remainingBytesInCmdBuff;
				
				while (dataPtr < endPtr)
				{
					if (!ReadFile(pipe, dataPtr, endPtr - dataPtr, &bytesRead, nullptr))
					{
						//full package not transmitted.
						//give up. Not much else we can do ¯\_(ツ)_/¯
						GMalloc.Free(packageData);
						return true;
					}
					dataPtr += bytesRead;
				}
			}

			InteropActionQueue.push(new AddToPrecacheMapAction(s2ws(fileName), packageData, packageSize));
			return true;
		}
		return false;
	}

	//no instantiation of static class
	FileLoader() = delete;
};