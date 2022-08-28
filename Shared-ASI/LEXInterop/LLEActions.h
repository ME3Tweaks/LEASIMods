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