// Shared across all three games
struct OutParmInfo
{
	UProperty* Prop;
	BYTE* PropAddr;
	OutParmInfo* Next;
};

// Code execution frame in UnrealScript
#pragma pack(4)
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

//============================================
// FVector and FMatrix math
//============================================

FVector operator* (const FVector& vec, float multiplier)
{
	return FVector{ vec.X * multiplier, vec.Y * multiplier, vec.Z * multiplier };
}

FVector operator/ (const FVector& vec, float divisor)
{
	return FVector{ vec.X / divisor, vec.Y / divisor, vec.Z / divisor };
}

FVector operator+ (const FVector& vec, float multiplier)
{
	return FVector{ vec.X + multiplier, vec.Y + multiplier, vec.Z + multiplier };
}

FVector operator- (const FVector& vec, float divisor)
{
	return FVector{ vec.X - divisor, vec.Y - divisor, vec.Z - divisor };
}

FVector& operator*= (FVector& vec, float multiplier)
{
	vec.X *= multiplier;
	vec.Y *= multiplier;
	vec.Z *= multiplier;
	return vec;
}

FVector& operator/= (FVector& vec, float divisor)
{
	vec.X /= divisor;
	vec.Y /= divisor;
	vec.Z /= divisor;
	return vec;
}

FVector& operator+= (FVector& vec, float multiplier)
{
	vec.X += multiplier;
	vec.Y += multiplier;
	vec.Z += multiplier;
	return vec;
}

FVector& operator-= (FVector& vec, float divisor)
{
	vec.X -= divisor;
	vec.Y -= divisor;
	vec.Z -= divisor;
	return vec;
}

FVector operator+ (const FVector& vec, FVector vec2)
{
	return FVector{ vec.X + vec2.X, vec.Y + vec2.Y, vec.Z + vec2.Z };
}

FVector operator- (const FVector& vec, FVector vec2)
{
	return FVector{ vec.X - vec2.X, vec.Y - vec2.Y, vec.Z - vec2.Z };
}

FVector& operator+= (FVector& vec, FVector vec2)
{
	vec.X += vec2.X;
	vec.Y += vec2.Y;
	vec.Z += vec2.Z;
	return vec;
}

FVector& operator-= (FVector& vec, FVector vec2)
{
	vec.X -= vec2.X;
	vec.Y -= vec2.Y;
	vec.Z -= vec2.Z;
	return vec;
}

float Dot(FVector vector1, FVector vector2)
{
	return (vector1.X * vector2.X)
		 + (vector1.Y * vector2.Y)
		 + (vector1.Z * vector2.Z);
}

int DegreesToUnrealRotationUnits(float degrees)
{
	return degrees * 65536.f / 360.f;
}

int RadiansToUnrealRotationUnits(float radians)
{
	return radians * 180.f / 3.14159265f * 65536.f / 360.f;
}

float UnrealRotationUnitsToDegrees(int unrealRotationUnits)
{
	return unrealRotationUnits * 360.f / 65536.f;
}

float UnrealRotationUnitsToRadians(int unrealRotationUnits)
{
	return unrealRotationUnits * 360.f / 65536.f * 3.14159265f / 180.f;
}

constexpr FMatrix IdentityMatrix
{
	{1, 0, 0, 0},
	{0, 1, 0, 0},
	{0, 0, 1, 0},
	{0, 0, 0, 1}
};

void MatrixDecompose(FMatrix m, FVector& translation, FVector& scale, float& pitchRad, float& yawRad, float& rollRad)
{
	translation = FVector{ m.WPlane.X, m.WPlane.Y, m.WPlane.Z };

	FVector xAxis = m.XPlane;
	FVector yAxis = m.YPlane;
	FVector zAxis = m.ZPlane;

	scale = FVector{ sqrt(xAxis.X * xAxis.X + xAxis.Y * xAxis.Y + xAxis.Z * xAxis.Z),
					sqrt(yAxis.X * yAxis.X + yAxis.Y * yAxis.Y + yAxis.Z * yAxis.Z),
					sqrt(zAxis.X * zAxis.X + zAxis.Y * zAxis.Y + zAxis.Z * zAxis.Z) };

	constexpr float epsilon = 1e-6f;

	if (abs(scale.X) < epsilon || abs(scale.Y) < epsilon || abs(scale.Z) < epsilon)
	{
		pitchRad = yawRad = rollRad = 0;
		return;
	}

	xAxis.X /= scale.X;
	xAxis.Y /= scale.X;
	xAxis.Z /= scale.X;

	yAxis.X /= scale.Y;
	yAxis.Y /= scale.Y;
	yAxis.Z /= scale.Y;

	zAxis.X /= scale.Z;
	zAxis.Y /= scale.Z;
	zAxis.Z /= scale.Z;

	pitchRad = atan2(xAxis.Z, sqrt(pow(xAxis.X, 2.f) + pow(xAxis.Y, 2.f)));
	yawRad = atan2(xAxis.Y, xAxis.X);

	auto syAxis = FVector{ -sin(yawRad), cos(yawRad), 0.f };

	rollRad = atan2(Dot(zAxis, syAxis), Dot(yAxis, syAxis));
}

FMatrix MatrixCompose(FVector translation, FVector scale, float pitchRad, float yawRad, float rollRad)
{
	float sp = sin(pitchRad);
	float sy = sin(yawRad);
	float sr = sin(rollRad);
	float cp = cos(pitchRad);
	float cy = cos(yawRad);
	float cr = cos(rollRad);

	float sX = scale.X;
	float sY = scale.Y;
	float sZ = scale.Z;

	return FMatrix
	{
		{
			cp * cy * sX,
			cp * sX * sy,
			sX * sp,
			0.f
		},
		{
			sY * (cy * sp * sr - cr * sy),
			sY * (cr * cy + sp * sr * sy),
			-cp * sY * sr,
			0.f
		},
		{
			-sZ * (cr * cy * sp + sr * sy),
			sZ * (cy * sr - cr * sp * sy),
			cp * cr * sZ,
			0.f
		},
		{
			translation,
			1.f
		}
	};
}

FMatrix operator* (const FMatrix& matrix1, const FMatrix& matrix2)
{
	typedef float MatrixAs2DArray[4][4];
	MatrixAs2DArray& m1 = *((MatrixAs2DArray*)&matrix1);
	MatrixAs2DArray& m2 = *((MatrixAs2DArray*)&matrix2);
	FMatrix result;
	MatrixAs2DArray& r = *((MatrixAs2DArray*)&result);
	r[0][0] = m1[0][0] * m2[0][0] + m1[0][1] * m2[1][0] + m1[0][2] * m2[2][0] + m1[0][3] * m2[3][0];
	r[0][1] = m1[0][0] * m2[0][1] + m1[0][1] * m2[1][1] + m1[0][2] * m2[2][1] + m1[0][3] * m2[3][1];
	r[0][2] = m1[0][0] * m2[0][2] + m1[0][1] * m2[1][2] + m1[0][2] * m2[2][2] + m1[0][3] * m2[3][2];
	r[0][3] = m1[0][0] * m2[0][3] + m1[0][1] * m2[1][3] + m1[0][2] * m2[2][3] + m1[0][3] * m2[3][3];
	
	r[1][0] = m1[1][0] * m2[0][0] + m1[1][1] * m2[1][0] + m1[1][2] * m2[2][0] + m1[1][3] * m2[3][0];
	r[1][1] = m1[1][0] * m2[0][1] + m1[1][1] * m2[1][1] + m1[1][2] * m2[2][1] + m1[1][3] * m2[3][1];
	r[1][2] = m1[1][0] * m2[0][2] + m1[1][1] * m2[1][2] + m1[1][2] * m2[2][2] + m1[1][3] * m2[3][2];
	r[1][3] = m1[1][0] * m2[0][3] + m1[1][1] * m2[1][3] + m1[1][2] * m2[2][3] + m1[1][3] * m2[3][3];
	
	r[2][0] = m1[2][0] * m2[0][0] + m1[2][1] * m2[1][0] + m1[2][2] * m2[2][0] + m1[2][3] * m2[3][0];
	r[2][1] = m1[2][0] * m2[0][1] + m1[2][1] * m2[1][1] + m1[2][2] * m2[2][1] + m1[2][3] * m2[3][1];
	r[2][2] = m1[2][0] * m2[0][2] + m1[2][1] * m2[1][2] + m1[2][2] * m2[2][2] + m1[2][3] * m2[3][2];
	r[2][3] = m1[2][0] * m2[0][3] + m1[2][1] * m2[1][3] + m1[2][2] * m2[2][3] + m1[2][3] * m2[3][3];
	
	r[3][0] = m1[3][0] * m2[0][0] + m1[3][1] * m2[1][0] + m1[3][2] * m2[2][0] + m1[3][3] * m2[3][0];
	r[3][1] = m1[3][0] * m2[0][1] + m1[3][1] * m2[1][1] + m1[3][2] * m2[2][1] + m1[3][3] * m2[3][1];
	r[3][2] = m1[3][0] * m2[0][2] + m1[3][1] * m2[1][2] + m1[3][2] * m2[2][2] + m1[3][3] * m2[3][2];
	r[3][3] = m1[3][0] * m2[0][3] + m1[3][1] * m2[1][3] + m1[3][2] * m2[2][3] + m1[3][3] * m2[3][3];
	return result;
}

//============================================
// TArray iterator support (so it can be used in range-based for loops and stl algorithms)
//============================================

template< class T >
T* begin(TArray<T>& arr) { return arr.Data; }
template< class T >
T* end(TArray<T>& arr) { return arr.Data + arr.Count; }
template< class T >
T const* cbegin(TArray<T> const& arr) { return arr.Data; }
template< class T >
T* cend(TArray<T> const& arr) { return arr.Data + arr.Count; }
template< class T >
T const* begin(TArray<T> const& arr) { return cbegin(arr); }
template< class T >
T const* end(TArray<T> const& arr) { return cend(arr); }

//============================================
// TArray manipulation functions defined here so that they have access to GMalloc
//============================================

//Returns the index of the first slot added.
template<class T>
int AddUninitializedSpaceToTArray(TArray<T>& arr, int numElements)
{
	const INT originalCount = arr.Count;
	if ((arr.Count += numElements) > arr.Max)
	{
		arr.Max = arr.Count; //Would probably be better to allocate slack, but this is simple and it works
		arr.Data = (T*)GMalloc.Realloc(arr.Data, numElements * sizeof(T));
	}

	return originalCount;
}

template<class T>
TArray<T> MakeCopyOfTArray(TArray<T>& arr)
{
	TArray<T> newArr;
	newArr.Max = newArr.Count = arr.Count;
	newArr.Data = static_cast<T*>(GMalloc.Malloc(arr.Count * sizeof(T)));
	memcpy(filename.Data, This->Filename.Data, filename.Count * sizeof(T));
	return newArr;
}

#include "TMap.h"