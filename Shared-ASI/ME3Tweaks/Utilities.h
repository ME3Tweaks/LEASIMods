// Shared across all three games
struct OutParmInfo
{
	UProperty* Prop;
	BYTE* PropAddr;
	OutParmInfo* Next;
};

// Code execution frame in UnrealScript
struct FFrame
{
	void* vtable;
#if defined GAMELE1 || defined GAMELE2
	int unks[3];
#elif defined GAMELE3
	int unks[4];
#endif
	UStruct* Node;
	UObject* Object;
	BYTE* Code;
	BYTE* Locals;
	FFrame* PreviousFrame;
	OutParmInfo* OutParms;
};

// A GNative function which takes no arguments and returns TRUE.
void AlwaysPositiveNative(UObject* pObject, void* pFrame, void* pResult)
{
    ((FFrame*)(pFrame))->Code++;
    *(long long*)pResult = TRUE;
}

// A GNative function which takes no arguments and returns FALSE.
void AlwaysNegativeNative(UObject* pObject, void* pFrame, void* pResult)
{
    ((FFrame*)(pFrame))->Code++;
    *(long long*)pResult = FALSE;
}
