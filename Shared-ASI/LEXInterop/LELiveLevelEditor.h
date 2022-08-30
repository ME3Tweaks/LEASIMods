#pragma once
#include <ostream>


#include "../ME3Tweaks/ME3TweaksHeader.h"
#include "../ConsoleCommandParsing.h"
#include "LLEActions.h"
#include "InteropActionQueue.h"


class LELiveLevelEditor
{
public:
	static bool IsLLEActive;
	static TArray<UObject*> Actors;
	static AActor* SelectedActor;
	static bool DrawLineToSelected;
private:
	static void DumpActors()
	{
		IsLLEActive = true;
		SelectedActor = nullptr; // We deselect the actor
		const auto objCount = UObject::GObjObjects()->Count;
		const auto objArray = UObject::GObjObjects()->Data;

		Actors.Count = 0; //clear the array without de-allocating any memory.

		const auto actorClass = AActor::StaticClass();
		for (auto j = 0; j < objCount; j++)
		{
			const auto obj = objArray[j];
			if (obj && obj->IsA(actorClass))
			{
				const auto actor = static_cast<AActor*>(obj);
				if (actor) {
					if (IsDefaultObject(actor) || IsA<ABioWorldInfo>(actor))// || actor->bStatic || !actor->bMovable)
					{
						continue;
					}
					Actors.Add(actor); // We will send them to LEX after we bundle it all up
				}
			}
		}

		// This originally used to count first but that was removed for other design issues

		// Tell LEX we're about to send over an actor list, so it can clear it and be ready for new data.
		SendStringToLEX(L"LIVELEVELEDITOR ACTORDUMPSTART", 100);

		// Send the actor information to LEX
		int numSent = 0;
		for (int i = 0; i < Actors.Count; i++)
		{
			if (Actors.Data[i]->IsA(actorClass))
			{
				const auto actor = static_cast<AActor*>(Actors.Data[i]);

				//if (actor->IsA(AStaticLightCollectionActor::StaticClass()))
				//{
				//	auto slca = static_cast<AStaticLightCollectionActor*>(actor);
				//	for (int i = 0; i < slca->Components.Count; i++)
				//	{
				//		SetSLCAComponentPosition(slca, i, 0, 0, 0);
				//	}
				//}

				//if (actor->IsA(AStaticMeshCollectionActor::StaticClass()))
				//{
				//	auto slca = static_cast<AStaticMeshCollectionActor*>(actor);
				//	for (int i = 0; i < slca->Components.Count; i++)
				//	{
				//		SetSLCAComponentPosition(slca, i, 0, 0, 0);
				//	}
				//}

				// String interps in C++ :/
				std::wstringstream ss2; // This is not declared outside the loop cause otherwise it carries forward
				ss2 << L"LIVELEVELEDITOR ACTORINFO ";

				const auto name = actor->Name.GetName();
				ss2 << GetContainingMapName(actor);
				ss2 << L':' << name;
				const auto index = actor->Name.Number;
				if (index > 0)
				{
					ss2 << L'_' << index - 1;
				}

				/*if (actor->bStatic || !actor->bMovable)
				{
					ss2 << ":static";
				}*/

				const auto tag = actor->Tag.GetName();
				if (strlen(tag) > 0 && _strcmpi(tag, actor->Class->GetName()) != 0)
				{
					// Tag != ClassName
					ss2 << L":TAG=" << tag;
				}

				numSent++;
				ss2 << L'\0';
				SendStringToLEX(ss2.str(), 100);
			}
		}

		// Tell LEX we're done.
		SendStringToLEX(L"LIVELEVELEDITOR ACTORDUMPFINISHED");
	}

	// Gets an actor with the specified full name from the specified map file in memory
	// This should probably be invalidated or always found new
	// Maybe enumerate the actors list...?
	static void UpdateSelectedActor(const char* mapName, const char* actorName) {
		//writeln(L"Selecting act for: %hs", actorName);
		const auto objCount = UObject::GObjObjects()->Count;
		const auto objArray = UObject::GObjObjects()->Data;

		const auto actorClass = AActor::StaticClass();
		for (auto j = 0; j < objCount; j++)
		{
			const auto obj = objArray[j];
			if (obj && obj->IsA(actorClass)) {
				const auto objMapName = GetContainingMapName(obj);
				if (_strcmpi(mapName, objMapName) != 0)
					continue; // Go to next object.

				const auto name = obj->GetFullName(false);
				//writeln(L"%hs", name);
				if (strcmp(actorName, name) == 0)
				{
					SendStringToLEX(L"LIVELEVELEDITOR ACTORSELECTED"); // We have selected an actor
					SelectedActor = static_cast<AActor*>(obj);
					return;
				}
			}
		}

		SendStringToLEX(L"LIVELEVELEDITOR ACTORSELECTED"); // We didn't find an actor but we finished the selection routine.
		SelectedActor = nullptr; // Didn't find!
	}

	static void SendActorData()
	{
		if (SelectedActor) {
			std::wstringstream ss1;
			ss1 << L"LIVELEVELEDITOR ACTORLOC " << SelectedActor->LOCATION.X << " " << SelectedActor->LOCATION.Y << " " << SelectedActor->LOCATION.Z;
			ss1 << L'\0';
			SendStringToLEX(ss1.str());

			std::wstringstream ss2;
			ss2 << L"LIVELEVELEDITOR ACTORROT " << SelectedActor->Rotation.Pitch << " " << SelectedActor->Rotation.Yaw << " " << SelectedActor->Rotation.Roll;
			ss2 << L'\0';
			SendStringToLEX(ss2.str());

			std::wstringstream ss3;
			ss3 << L"LIVELEVELEDITOR ACTORSCALE " << SelectedActor->DrawScale << " " << SelectedActor->DrawScale3D.X << " " << SelectedActor->DrawScale3D.Y << " " << SelectedActor->DrawScale3D.Z;
			ss3 << L'\0';
			SendStringToLEX(ss3.str());
		}
	}

	static void SetActorPosition(const float x, const float y, const float z)
	{
		if (SelectedActor) {
			InteropActionQueue.push(new MoveAction(SelectedActor, FVector{ x, y, z }));
		}
	}

	static void SetSLCAComponentPosition(const AStaticLightCollectionActor* smca, const int componentIdx, float x, float y, float z)
	{
		if (smca)
		{
			const auto compC = smca->Components.Data[componentIdx];
			if (compC && compC->IsA(ULightComponent::StaticClass()))
			{
				// Todo: This (UI needs sub-component selector)
				const auto comp = static_cast<ULightComponent*>(compC);
				comp->WorldToLight = FMatrix();
				comp->LightToWorld = FMatrix();
			}
		}
	}

	static void SetSLCAComponentPosition(const AStaticMeshCollectionActor* smca, const int componentIdx, float x, float y, float z)
	{
		if (smca)
		{
			const auto compC = smca->Components.Data[componentIdx];
			if (compC && compC->IsA(UStaticMeshComponent::StaticClass()))
			{
				// Todo: This (UI needs sub-component selector)
				const auto comp = static_cast<UStaticMeshComponent*>(compC);
				comp->CachedParentToWorld = FMatrix();
				comp->LocalToWorld = FMatrix();
			}
		}
	}

	static void SetActorRotation(const int pitch, const int yaw, const int roll)
	{
		if (SelectedActor) {
			InteropActionQueue.push(new RotateAction(SelectedActor, FRotator{ pitch, yaw, roll }));
		}
	}

	static void SetActorDrawScale3D(const float scaleX, const float scaleY, const float scaleZ)
	{
		if (SelectedActor) {
			InteropActionQueue.push(new Scale3DAction(SelectedActor, FVector{ scaleX, scaleY, scaleZ }));
		}
	}

	static void SetActorDrawScale(const float scale)
	{
		if (SelectedActor) {
			InteropActionQueue.push(new ScaleAction(SelectedActor, scale));
		}
	}

public:
	// Return true if other features should also be able to handle this function call
	// Return false if other features shouldn't be able to also handle this function call
	static bool ProcessEvent(UObject* Context, UFunction* Function, void* Parms, void* Result)
	{
		if (SelectedActor == nullptr)
			return true; // We have nothing to handle here

		// PostRender
		const auto funcName = Function->GetName();

		// This isn't that efficient since we could skip this every time if
		// we weren't drawing the line. But we have to be able to flush it out.

		if (strcmp(funcName, "PostRender") == 0)
		{
			const auto hud = reinterpret_cast<ABioHUD*>(Context);
			if (hud != nullptr)
			{
				hud->FlushPersistentDebugLines(); // Clear it out
				if (DrawLineToSelected) {

					hud->DrawDebugLine(SharedData::cachedPlayerPosition, SelectedActor->LOCATION, 255, 255, 255, TRUE);
				}
			}
		}
		return true;
	}

	// Return true if handled.
	// Return false if not handled.
	static bool HandleCommand(char* command)
	{
		if (IsCmd(&command, "LLE_TEST_ACTIVE"))
		{
			// We can receive this command, so we are ready
			SendStringToLEX(L"LIVELEVELEDITOR READY");
			return true;
		}

		if (IsCmd(&command, "LLE_DUMP_ACTORS"))
		{
			DumpActors();
			return true;
		}

		if (IsCmd(&command, "LLE_SHOW_TRACE"))
		{
			DrawLineToSelected = true;
			return true;
		}

		if (IsCmd(&command, "LLE_HIDE_TRACE"))
		{
			DrawLineToSelected = false;
			return true;
		}

		if (IsCmd(&command, "LLE_SELECT_ACTOR "))
		{
			const auto delims = " ";
			char* nextToken;
			const char* mapName = strtok_s(command, delims, &nextToken);
			const char* objName = strtok_s(nullptr, delims, &nextToken);
			UpdateSelectedActor(mapName, objName);
			return true;
		}

		if (IsCmd(&command, "LLE_GET_ACTOR_POSDATA"))
		{
			SendActorData();
			return true;
		}

		if (IsCmd(&command, "LLE_UPDATE_ACTOR_POS "))
		{
			const auto posX = strtof(command, &command);
			const auto posY = strtof(command, &command);
			const auto posZ = strtof(command, &command);

			SetActorPosition(posX, posY, posZ);
			return true;
		}

		if (IsCmd(&command, "LLE_UPDATE_ACTOR_ROT "))
		{
			const auto pitch = strtol(command, &command, 10);
			const auto yaw = strtol(command, &command, 10);
			const auto roll = strtol(command, &command, 10);

			SetActorRotation(pitch, yaw, roll);
			return true;
		}

		if (IsCmd(&command, "LLE_SET_ACTOR_DRAWSCALE3D "))
		{
			const auto scaleX = strtof(command, &command);
			const auto scaleY = strtof(command, &command);
			const auto scaleZ = strtof(command, &command);

			SetActorDrawScale3D(scaleX, scaleY, scaleZ);
			return true;
		}

		if (IsCmd(&command, "LLE_SET_ACTOR_DRAWSCALE "))
		{
			const auto scale = strtof(command, &command);

			SetActorDrawScale(scale);
			return true;
		}

		return false;
	}
};

// Static variable initialization
TArray<UObject*> LELiveLevelEditor::Actors = TArray<UObject*>();
AActor* LELiveLevelEditor::SelectedActor = nullptr;
bool LELiveLevelEditor::DrawLineToSelected = true;
bool LELiveLevelEditor::IsLLEActive = false;