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
	static int SelectedComponentIndex; //for editing a component in a CollectionActor. Ignored if SelectedActor is not a Collectionactor
	static bool DrawLineToSelected;
	static FColor TraceLineColor;
	static float CoordinateScale;
	
private:
	static void DumpActors()
	{
		IsLLEActive = true;
		SelectedActor = nullptr; // We deselect the actor
		SelectedComponentIndex = -1;
		const auto objCount = UObject::GObjObjects()->Count;
		const auto objArray = UObject::GObjObjects()->Data;

		Actors.Count = 0; //clear the array without de-allocating any memory.

		const auto actorClass = AActor::StaticClass();
		for (auto j = 0; j < objCount; j++)
		{
			const auto obj = objArray[j];
			if (obj && obj->IsA(actorClass))
			{
				const auto actor = reinterpret_cast<AActor*>(obj);
				if (IsDefaultObject(actor) || IsA<ABioWorldInfo>(actor))// || actor->bStatic || !actor->bMovable)
				{
					continue;
				}
				Actors.Add(actor); // We will send them to LEX after we bundle it all up
			}
		}

		// This originally used to count first but that was removed for other design issues

		// Tell LEX we're about to send over an actor list, so it can clear it and be ready for new data.
		SendStringToLEX(L"LIVELEVELEDITOR ACTORDUMPSTART");

		// Send the actor information to LEX
		for (int i = 0; i < Actors.Count; i++)
		{
			if (Actors.Data[i]->IsA(actorClass))
			{
				const auto actor = reinterpret_cast<AActor*>(Actors.Data[i]);

				// String interps in C++ :/
				std::wostringstream ss2; // This is not declared outside the loop cause otherwise it carries forward
				ss2 << L"LIVELEVELEDITOR ACTORINFO ";

				const auto name = actor->Name.GetName();
				ss2 << L"MAP=" << GetContainingMapName(actor);
				ss2 << L":ACTOR=" << name;
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

				if (actor->IsA(AStaticMeshCollectionActor::StaticClass()))
				{
					const auto strStartPos = ss2.tellp();
					const auto smca = reinterpret_cast<AStaticMeshCollectionActor*>(actor);
					for (int j = 0; j < smca->Components.Count; j++)
					{
						const auto component = smca->Components(j);
						if (IsA<UStaticMeshComponent>(component))
						{
							//everything up to TAG is the same for every component, so just reset position and overwrite
							ss2.seekp(strStartPos);
							ss2 << L":COMPNAME=" << component->GetName();
							ss2 << L":COMPIDX=" << j;
							ss2 << L'\0';
							//This will send the entire string, which may contain garbage on the end if, for example, a previous component had a longer name.
							//This isn't a problem, since we explicitly put a null at the end, and LEX reads until a null.
							SendStringToLEX(ss2.str());
						}
					}
					continue;
				}
				
				ss2 << L'\0';
				SendStringToLEX(ss2.str());
			}
		}

		// Tell LEX we're done.
		SendStringToLEX(L"LIVELEVELEDITOR ACTORDUMPFINISHED");
	}

	// Gets an actor with the specified full name from the specified map file in memory
	// This should probably be invalidated or always found new
	// Maybe enumerate the actors list...?
	static void UpdateSelectedActor(char* selectionStr) {
		const auto delims = " ";
		char* nextToken;
		const char* mapName = strtok_s(selectionStr, delims, &nextToken);
		const char* actorName = strtok_s(nullptr, delims, &nextToken);

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
					if (IsA<AStaticMeshCollectionActor>(obj))
					{
						const int compIdx = (int)strtol(nextToken, &nextToken, 10);
						const auto smca = reinterpret_cast<AStaticMeshCollectionActor*>(obj);
						if (compIdx < 0 || compIdx >= smca->Components.Count)
						{
							goto NotFound;
						}
						SelectedComponentIndex = compIdx;
					}
					else
					{
						SelectedComponentIndex = -1;
					}
					SelectedActor = reinterpret_cast<AActor*>(obj);
					SendStringToLEX(L"LIVELEVELEDITOR ACTORSELECTED"); // We have selected an actor
					return;
				}
			}
		}
	NotFound:
		SendStringToLEX(L"LIVELEVELEDITOR ACTORSELECTED"); // We didn't find an actor but we finished the selection routine.
		SelectedActor = nullptr; // Didn't find!
		SelectedComponentIndex = -1;
	}

	static void SendActorData()
	{
		if (SelectedActor) {
			FVector translation{};
			float scale{};
			FVector scale3D{};
			FRotator rotation{};
			if (IsA<AStaticMeshCollectionActor>(SelectedActor))
			{
				const auto smca = reinterpret_cast<AStaticMeshCollectionActor*>(SelectedActor);
				const auto compC = smca->Components.Data[SelectedComponentIndex];
				if (compC && compC->IsA(UStaticMeshComponent::StaticClass()))
				{
					const auto smc = reinterpret_cast<UStaticMeshComponent*>(compC);
					float pitch_rad;
					float yaw_rad;
					float roll_rad;
					MatrixDecompose(smc->CachedParentToWorld, translation, scale3D, pitch_rad, yaw_rad, roll_rad);
					rotation = FRotator{ RadiansToUnrealRotationUnits(pitch_rad),
										 RadiansToUnrealRotationUnits(yaw_rad),
										 RadiansToUnrealRotationUnits(roll_rad) };
					scale = smc->Scale;
					scale3D /= scale;
				}
			}
			else
			{
				translation = SelectedActor->LOCATION;
				scale = SelectedActor->DrawScale;
				scale3D = SelectedActor->DrawScale3D;
				rotation = SelectedActor->Rotation;
			}
			std::wstringstream ss1;
			ss1 << L"LIVELEVELEDITOR ACTORLOC " << translation.X << " " << translation.Y << " " << translation.Z;
			ss1 << L'\0';
			SendStringToLEX(ss1.str());

			std::wstringstream ss2;
			ss2 << L"LIVELEVELEDITOR ACTORROT " << rotation.Pitch << " " << rotation.Yaw << " " << rotation.Roll;
			ss2 << L'\0';
			SendStringToLEX(ss2.str());

			std::wstringstream ss3;
			ss3 << L"LIVELEVELEDITOR ACTORSCALE " << scale << " " << scale3D.X << " " << scale3D.Y << " " << scale3D.Z;
			ss3 << L'\0';
			SendStringToLEX(ss3.str());
		}
	}

	static void SetActorPosition(const float x, const float y, const float z)
	{
		if (SelectedActor) {
			if (IsA<AStaticMeshCollectionActor>(SelectedActor))
			{
				const auto smca = reinterpret_cast<AStaticMeshCollectionActor*>(SelectedActor);
				const auto compC = smca->Components.Data[SelectedComponentIndex];
				if (compC && compC->IsA(UStaticMeshComponent::StaticClass()))
				{
					InteropActionQueue.push(new ComponentMoveAction(reinterpret_cast<UStaticMeshComponent*>(compC), FVector{ x, y, z }));
				}

			}
			else
			{
				InteropActionQueue.push(new MoveAction(SelectedActor, FVector{ x, y, z }));
			}
		}
	}

	static void SetActorRotation(const int pitch, const int yaw, const int roll)
	{
		if (SelectedActor) {
			if (IsA<AStaticMeshCollectionActor>(SelectedActor))
			{
				const auto smca = reinterpret_cast<AStaticMeshCollectionActor*>(SelectedActor);
				const auto compC = smca->Components.Data[SelectedComponentIndex];
				if (compC && compC->IsA(UStaticMeshComponent::StaticClass()))
				{
					InteropActionQueue.push(new ComponentRotateAction(reinterpret_cast<UStaticMeshComponent*>(compC), FRotator{pitch, yaw, roll}));
				}
			}
			else
			{
				InteropActionQueue.push(new RotateAction(SelectedActor, FRotator{ pitch, yaw, roll }));
			}
		}
	}

	static void SetActorDrawScale3D(const float scaleX, const float scaleY, const float scaleZ)
	{
		if (SelectedActor) {
			if (IsA<AStaticMeshCollectionActor>(SelectedActor))
			{
				const auto smca = reinterpret_cast<AStaticMeshCollectionActor*>(SelectedActor);
				const auto compC = smca->Components.Data[SelectedComponentIndex];
				if (compC && compC->IsA(UStaticMeshComponent::StaticClass()))
				{
					InteropActionQueue.push(new ComponentScale3DAction(reinterpret_cast<UStaticMeshComponent*>(compC), FVector{ scaleX, scaleY, scaleZ }));
				}
			}
			else
			{
				InteropActionQueue.push(new Scale3DAction(SelectedActor, FVector{ scaleX, scaleY, scaleZ }));
			}
		}
	}

	static void SetActorDrawScale(const float scale)
	{
		if (SelectedActor) {
			if (IsA<AStaticMeshCollectionActor>(SelectedActor))
			{
				const auto smca = reinterpret_cast<AStaticMeshCollectionActor*>(SelectedActor);
				const auto compC = smca->Components.Data[SelectedComponentIndex];
				if (compC && compC->IsA(UStaticMeshComponent::StaticClass()))
				{
					InteropActionQueue.push(new ComponentScaleAction(reinterpret_cast<UStaticMeshComponent*>(compC), scale));
				}
			}
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
				if (IsLLEActive)
				{
					if (DrawLineToSelected) {
						auto line_end = SelectedActor->LOCATION;
						if (SelectedComponentIndex >= 0 && IsA<AStaticMeshCollectionActor>(SelectedActor))
						{
							const auto smca = reinterpret_cast<AStaticMeshCollectionActor*>(SelectedActor);
							const auto comp = smca->Components(SelectedComponentIndex);
							if (IsA<UStaticMeshComponent>(comp))
							{
								line_end = static_cast<FVector>(reinterpret_cast<UStaticMeshComponent*>(comp)->CachedParentToWorld.WPlane);
							}
						}
						hud->DrawDebugLine(SharedData::cachedPlayerPosition, line_end, TraceLineColor.R, TraceLineColor.G, TraceLineColor.B, TRUE);
						hud->DrawDebugCoordinateSystem(line_end, FRotator(), CoordinateScale, TRUE);
					}
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
		if (IsCmd(&command, "LLE_DEACTIVATE"))
		{
			IsLLEActive = false;
			SelectedActor = nullptr;
			SelectedComponentIndex = -1;
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

		if (IsCmd(&command, "LLE_TRACE_COLOR "))
		{
			TraceLineColor.R = (BYTE)strtol(command, &command, 10);
			TraceLineColor.G = (BYTE)strtol(command, &command, 10);
			TraceLineColor.B = (BYTE)strtol(command, &command, 10);
			return true;
		}

		if (IsCmd(&command, "LLE_AXES_Scale "))
		{
			CoordinateScale = strtof(command, &command);
			return true;
		}

		if (IsCmd(&command, "LLE_SELECT_ACTOR "))
		{
			UpdateSelectedActor(command);
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
int LELiveLevelEditor::SelectedComponentIndex = -1;
bool LELiveLevelEditor::DrawLineToSelected = true;
bool LELiveLevelEditor::IsLLEActive = false; 
FColor LELiveLevelEditor::TraceLineColor = FColor{0,255,255,255}; //B, G, R, A
float LELiveLevelEditor::CoordinateScale = 100;