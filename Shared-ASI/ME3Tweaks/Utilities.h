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
