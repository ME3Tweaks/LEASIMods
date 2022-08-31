#pragma once

#include "../ME3Tweaks/ME3TweaksHeader.h"
#include "AdditionalFunctions.h"
#include "InteropActionQueue.h"

struct MoveAction final : ActionBase
{
	AActor* Actor;
	FVector Vector;

	MoveAction(AActor* actor, const FVector vector) : Actor(actor), Vector(vector) {}

	void Execute() override
	{
		FarMove(Actor, Vector, 0, 1, 0);
	}
};

struct Scale3DAction final : ActionBase
{
	AActor* Actor;
	FVector ScaleVector;

	Scale3DAction(AActor* actor, const FVector scaleVector) : Actor(actor), ScaleVector(scaleVector) {}

	void Execute() override
	{
		Actor->SetDrawScale3D(ScaleVector);
	}
};

struct ScaleAction final : ActionBase
{
	AActor* Actor;
	float Scale;

	ScaleAction(AActor* actor, const float scale) : Actor(actor), Scale(scale) {}

	void Execute() override
	{
		Actor->SetDrawScale(Scale);
	}
};

struct RotateAction final : ActionBase
{
	AActor* Actor;
	FRotator Rotator;

	RotateAction(AActor* actor, const FRotator rotator) : Actor(actor), Rotator(rotator) {}

	void Execute() override
	{
		SetRotationWrapper(Actor, Rotator);
	}
};

struct ComponentMoveAction final : ActionBase
{
	UStaticMeshComponent* Component;
	FVector Vector;

	ComponentMoveAction(UStaticMeshComponent* component, const FVector vector) : Component(component), Vector(vector) {}

	void Execute() override
	{
		FVector translation;
		FVector scale3D;
		float pitch;
		float yaw;
		float roll;
		MatrixDecompose(Component->CachedParentToWorld, translation, scale3D, pitch, yaw, roll);
		Component->CachedParentToWorld = IdentityMatrix;
		Component->Rotation = FRotator{ RadiansToUnrealRotationUnits(pitch), RadiansToUnrealRotationUnits(yaw), RadiansToUnrealRotationUnits(roll) };
		Component->Scale3D = scale3D / Component->Scale;
		Component->SetTranslation(Vector);
		Component->CachedParentToWorld = MatrixCompose(Vector, scale3D, pitch, yaw, roll);
	}
};

struct ComponentScaleAction final : ActionBase
{
	UStaticMeshComponent* Component;
	float Scale;

	ComponentScaleAction(UStaticMeshComponent* component, const float scale) : Component(component), Scale(scale) {}

	void Execute() override
	{
		FVector translation;
		FVector scale3D;
		float pitch;
		float yaw;
		float roll;
		MatrixDecompose(Component->CachedParentToWorld, translation, scale3D, pitch, yaw, roll);
		Component->CachedParentToWorld = IdentityMatrix;
		Component->Translation = translation;
		Component->Rotation = FRotator{ RadiansToUnrealRotationUnits(pitch), RadiansToUnrealRotationUnits(yaw), RadiansToUnrealRotationUnits(roll) };
		scale3D /= Component->Scale;
		Component->Scale3D = scale3D;
		Component->SetScale(Scale);
		scale3D *= Scale;
		Component->CachedParentToWorld = MatrixCompose(translation, scale3D, pitch, yaw, roll);
	}
};

struct ComponentScale3DAction final : ActionBase
{
	UStaticMeshComponent* Component;
	FVector ScaleVector;

	ComponentScale3DAction(UStaticMeshComponent* component, const FVector scaleVector) : Component(component), ScaleVector(scaleVector) {}

	void Execute() override
	{
		FVector translation;
		FVector scale;
		float pitch;
		float yaw;
		float roll;
		MatrixDecompose(Component->CachedParentToWorld, translation, scale, pitch, yaw, roll);
		Component->CachedParentToWorld = IdentityMatrix;
		Component->Translation = translation;
		Component->Rotation = FRotator{ RadiansToUnrealRotationUnits(pitch), RadiansToUnrealRotationUnits(yaw), RadiansToUnrealRotationUnits(roll) };
		Component->SetScale3D(ScaleVector);
		Component->CachedParentToWorld = MatrixCompose(translation, ScaleVector * Component->Scale, pitch, yaw, roll);
	}
};

struct ComponentRotateAction final : ActionBase
{
	UStaticMeshComponent* Component;
	FRotator Rotator;

	ComponentRotateAction(UStaticMeshComponent* component, const FRotator rotator) : Component(component), Rotator(rotator) {}

	void Execute() override
	{
		FVector translation;
		FVector scale3D;
		float pitch;
		float yaw;
		float roll;
		MatrixDecompose(Component->CachedParentToWorld, translation, scale3D, pitch, yaw, roll);
		Component->CachedParentToWorld = IdentityMatrix;
		Component->Translation = translation;
		Component->Scale3D = scale3D / Component->Scale;
		Component->SetRotation(Rotator);
		pitch = UnrealRotationUnitsToRadians(Rotator.Pitch);
		yaw = UnrealRotationUnitsToRadians(Rotator.Yaw);
		roll = UnrealRotationUnitsToRadians(Rotator.Roll);
		Component->CachedParentToWorld = MatrixCompose(translation, scale3D, pitch, yaw, roll);
	}
};