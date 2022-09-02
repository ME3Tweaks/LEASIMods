#pragma once

#include "../Common.h"
#include "../Interface.h"
#include "../ME3Tweaks/ME3TweaksHeader.h"
#include "UtilityMethods.h"
#include "../StaticVariablePointers.h"


// Typedefs
#if defined(GAMELE1) || defined(GAMELE2)
typedef BOOL(*tFarMoveActor)(UWorld* world, AActor* actor, FVector& destPos, BOOL test, BOOL noCollisionCheck, BOOL attachMove);
#elif GAMELE3
typedef BOOL(*tFarMoveActor)(UWorld* world, AActor* actor, FVector& destPos, BOOL test, BOOL noCollisionCheck, BOOL attachMove, BOOL unknown);
#endif



static BOOL FarMove(AActor* actor, FVector& destPos, const BOOL test, const BOOL noCollisionCheck, const BOOL attachMove)
{
	static tFarMoveActor FarMoveActor = nullptr;
	if (!FarMoveActor)
	{
		//variable name for use by the macro
		ISharedProxyInterface* InterfacePtr = ISharedProxyInterface::SPIInterfacePtr;
#if defined(GAMELE1) || defined(GAMELE2)
		// LE1 and LE2 have same byte signature
		INIT_FIND_PATTERN_POSTHOOK(FarMoveActor,/*"40 55 53 57 41*/ "54 41 56 48 8d 6c 24 d9 48 81 ec a0 00 00 00");
#elif GAMELE3
		// LE3 method has an extra bool parameter
		INIT_FIND_PATTERN_POSTHOOK(FarMoveActor,/*"40 55 53 57 41*/ "54 41 57 48 8d 6c 24 e1 48 81 ec b0 00 00 00");
#endif

		// Rel offset 
		constexpr BYTE relOffsetChange[] = {0x26}; // REL OFFSET (Same in all 3 games)
		PatchMemory((void*)((intptr_t)FarMoveActor + 40), relOffsetChange, 1);
		// Change JNZ jump offset to point to location test code (post checks)

		// Not sure if this is actually required but here to ensure the other jump can't occur
		constexpr BYTE jumpInstructionChange[] = {0xEB}; // JMP NEAR
		PatchMemory((void*)((intptr_t)FarMoveActor + 51), jumpInstructionChange, 1);
		// Change JNE to JMP when testing bStatic/bMovable
	}
	return FarMoveActor(StaticVariables::GWorld(), actor, destPos, test, noCollisionCheck, attachMove
#ifdef GAMELE3
	, 0
#endif
	);
}

static BOOL SetRotationWrapper(AActor* actor, const FRotator& rotation)
{
	if (static bool initialized = false; !initialized)
	{
		//variable name for use by the macro
		ISharedProxyInterface* InterfacePtr = ISharedProxyInterface::SPIInterfacePtr;

		//AActor::SetRotation calls the MoveActor function.
		//This disables MoveActor's check for bStatic and bMoveable, so that we can rotate anything

		void* moveActorMoveableCheck = nullptr;

#ifdef GAMELE1 
		INIT_FIND_PATTERN(moveActorMoveableCheck, "e0 4c 8b 8d 18 04 00 00 48 8b b5 28 04 00 00")
#elif defined(GAMELE2)
		INIT_FIND_PATTERN(moveActorMoveableCheck, "dc 4c 8b 8d 68 04 00 00 48 8b b5 78 04 00 00")
#elif defined(GAMELE3)
		INIT_FIND_PATTERN(moveActorMoveableCheck, "dc 4c 8b 8d 68 04 00 00 48 8b b5 78 04 00 00")
#endif

		constexpr BYTE jumpZero[] = { 0x0 }; // set JNZ to "jump" zero bytes
		PatchMemory(moveActorMoveableCheck, jumpZero, 1);
		initialized = true;
	}
	return actor->SetRotation(rotation);
}