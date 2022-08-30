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