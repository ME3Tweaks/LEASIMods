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

struct HideAction final : ActionBase
{
	AActor* Actor;
	bool Hidden;

	HideAction(AActor* actor, const bool hidden) : Actor(actor), Hidden(hidden) {}

	void Execute() override
	{
		Actor->SetHidden(Hidden);
	}
};

struct ComponentMoveAction final : ActionBase
{
	UStaticMeshComponent* Component;
	FVector Vector;

	ComponentMoveAction(UStaticMeshComponent* component, const FVector vector) : Component(component), Vector(vector) {}

	void Execute() override
	{
		Component->SetTranslation(Vector);
	}
};

struct ComponentScaleAction final : ActionBase
{
	UStaticMeshComponent* Component;
	float Scale;

	ComponentScaleAction(UStaticMeshComponent* component, const float scale) : Component(component), Scale(scale) {}

	void Execute() override
	{
		Component->SetScale(Scale);
	}
};

struct ComponentScale3DAction final : ActionBase
{
	UStaticMeshComponent* Component;
	FVector ScaleVector;

	ComponentScale3DAction(UStaticMeshComponent* component, const FVector scaleVector) : Component(component), ScaleVector(scaleVector) {}

	void Execute() override
	{
		Component->SetScale3D(ScaleVector);
	}
};

struct ComponentRotateAction final : ActionBase
{
	UStaticMeshComponent* Component;
	FRotator Rotator;

	ComponentRotateAction(UStaticMeshComponent* component, const FRotator rotator) : Component(component), Rotator(rotator) {}

	void Execute() override
	{
		Component->SetRotation(Rotator);
	}
};