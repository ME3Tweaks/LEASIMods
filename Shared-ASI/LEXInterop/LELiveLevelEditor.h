#pragma once
#include <ostream>
#include <set>
#include <algorithm>

#include "../ME3Tweaks/ME3TweaksHeader.h"
#include "../ConsoleCommandParsing.h"
#include "LLEActions.h"
#include "InteropActionQueue.h"
#include "json.h"
using json = nlohmann::json;

//needed so that std::set<FName> will compile
inline bool operator< (const FName& a, const FName& b)
{
	return *reinterpret_cast<const uint64*>(&a) < *reinterpret_cast<const uint64*>(&b);
}

class LELiveLevelEditor final
{
public:
	static bool IsLLEActive;
	static bool IsPendingDeactivation;
	static AActor* SelectedActor;
	static FName SelectedActorMapName;
	static int SelectedComponentIndex; //for editing a component in a CollectionActor. Ignored if SelectedActor is not a Collectionactor
	static bool DrawLineToSelected;
	static FLinearColor TraceLineColor;
	static float TraceWidth;
	static float CoordinateScale;
	static std::set<FName> LevelsLoadedByLLE;
	
private:
	static void ClearSelectedActor()
	{
		SelectedActor = nullptr;
		SelectedActorMapName = {};
		SelectedComponentIndex = -1;
	}

	//consolidates all transformation data into the properties, to simplify editing them
	static void NormalizeSMC(UStaticMeshComponent* smc)
	{
		FVector translation;
		FVector scale3D;
		float pitch_rad;
		float yaw_rad;
		float roll_rad;
		FMatrix tmp = smc->CachedParentToWorld;
		smc->CachedParentToWorld = IdentityMatrix;
		if (smc->AbsoluteTranslation)
		{
			tmp.WPlane = FPlane{ {0, 0, 0}, 1 };
		}
		if (smc->AbsoluteRotation || smc->AbsoluteScale)
		{
			MatrixDecompose(tmp, translation, scale3D, pitch_rad, yaw_rad, roll_rad);
			if (smc->AbsoluteRotation)
			{
				pitch_rad = yaw_rad = roll_rad = 0;
			}
			if (smc->AbsoluteScale)
			{
				scale3D = FVector{ 1,1,1 };
			}
			tmp = MatrixCompose(translation, scale3D, pitch_rad, yaw_rad, roll_rad);
		}
		const auto pitch = UnrealRotationUnitsToRadians(smc->Rotation.Pitch);
		const auto yaw = UnrealRotationUnitsToRadians(smc->Rotation.Yaw);
		const auto roll = UnrealRotationUnitsToRadians(smc->Rotation.Roll);
		tmp = MatrixCompose(smc->Translation, smc->Scale3D * smc->Scale, pitch, yaw, roll) * tmp;

		MatrixDecompose(tmp, translation, scale3D, pitch_rad, yaw_rad, roll_rad);
		smc->Translation = translation;
		smc->Rotation = FRotator{ RadiansToUnrealRotationUnits(pitch_rad), RadiansToUnrealRotationUnits(yaw_rad), RadiansToUnrealRotationUnits(roll_rad) };
		smc->Scale = 1;
		smc->Scale3D = scale3D;
	}

	static json AppendActorsInLevel(ULevelBase* const level)
	{
		json actorsArray = json::array();
		for (const auto actor : level->Actors)
		{
			if (!actor || IsDefaultObject(actor) || IsA<ABioWorldInfo>(actor))
			{
				continue;
			}
			json actorJson;
			actorJson["Name"] = actor->GetInstancedName();
			if (actor->IsA(AStaticMeshCollectionActor::StaticClass()))
			{
				const auto smca = reinterpret_cast<AStaticMeshCollectionActor*>(actor);
				json components = json::array();
				for (const auto component : smca->Components)
				{
					if (component && IsA<UStaticMeshComponent>(component))
					{
						NormalizeSMC(reinterpret_cast<UStaticMeshComponent*>(component));
						components.push_back(component->GetInstancedName());
					}
					else
					{
						components.push_back(nullptr);
					}
				}
				actorJson["Components"] = components;
			}
			else
			{
				const auto tag = actor->Tag.Instanced();
				if (strlen(tag) > 0 && _strcmpi(tag, actor->Class->GetName()) != 0)
				{
					// Tag != ClassName
					actorJson["Tag"] = tag;
				}
			}
			actorsArray.push_back(actorJson);
		}
		return actorsArray;
	}

	static void RefreshLevels(const AWorldInfo* worldInfo, const std::vector<FName>* removedLevels = nullptr)
	{
		if (removedLevels && SelectedActor)
		{
			for (FName levelName : *removedLevels)
			{
				if (levelName == SelectedActorMapName)
				{
					ClearSelectedActor();
					break;
				}
			}
		}
		LevelsLoadedByLLE.clear();
		const auto mainLevel = reinterpret_cast<ULevelBase*>(worldInfo->Outer);
		const auto mainPackage = mainLevel->Outer->Outer;
		LevelsLoadedByLLE.insert(mainPackage->Name);

		/* JSON format example
		[
		   {
		      "Name":"BioD_Nor",
		      "Actors":[
		         {
		            "Name":"BioPawn_0",
		            "Tag":"Liara"
		         },
		         {
		            "Name":"StaticMeshCollectionActor_23",
		            "Components":[
		               "StaticMeshActor_34",
		               "StaticMeshActor_36"
		            ]
		         }
		      ]
		   }
		]
		 */
		json topLevelJson = json::array();

		json mainPackageJson;
		mainPackageJson["Name"] = mainPackage->GetInstancedName();
		mainPackageJson["Actors"] = AppendActorsInLevel(mainLevel);
		topLevelJson.push_back(mainPackageJson);
		
		for (const auto streaming_level : worldInfo->StreamingLevels)
		{
			if (!streaming_level->bIsVisible || !streaming_level->LoadedLevel)
			{
				continue;
			}
			LevelsLoadedByLLE.insert(streaming_level->PackageName);

			json streamingLevelJson;
			streamingLevelJson["Name"] = streaming_level->PackageName.Instanced();
			streamingLevelJson["Actors"] = AppendActorsInLevel(streaming_level->LoadedLevel);
			topLevelJson.push_back(streamingLevelJson);
		}

		std::wostringstream ss;
		ss << L"LIVELEVELEDITOR LEVELSUPDATE ";
		ss << s2ws(topLevelJson.dump());
		SendStringToLEX(ss.str(), 1000);
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
				if (_strcmpi(mapName, objMapName.Instanced()) != 0)
					continue; // Go to next object.

				//writeln(L"%hs", name);
				if (_strcmpi(actorName, obj->GetInstancedName()) == 0)
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
					SelectedActorMapName = objMapName;
					SendStringToLEX(L"LIVELEVELEDITOR ACTORSELECTED"); // We have selected an actor
					return;
				}
			}
		}
	NotFound:
		ClearSelectedActor();
		SendStringToLEX(L"LIVELEVELEDITOR ACTORSELECTED"); // We didn't find an actor but we finished the selection routine.
	}

	static void SendActorData()
	{
		if (SelectedActor) {
			FVector translation{};
			float scale = 0;
			FVector scale3D{};
			FRotator rotation{};
			if (IsA<AStaticMeshCollectionActor>(SelectedActor))
			{
				const auto smca = reinterpret_cast<AStaticMeshCollectionActor*>(SelectedActor);
				const auto compC = smca->Components.Data[SelectedComponentIndex];
				if (compC && compC->IsA(UStaticMeshComponent::StaticClass()))
				{
					const auto smc = reinterpret_cast<UStaticMeshComponent*>(compC);

					translation = smc->Translation;
					scale = smc->Scale;
					scale3D = smc->Scale3D;
					rotation = smc->Rotation;
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
			else
			{
				InteropActionQueue.push(new ScaleAction(SelectedActor, scale));
			}
		}
	}

public:
	// Return true if other features should also be able to handle this function call
	// Return false if other features shouldn't be able to also handle this function call
	static bool ProcessEvent(UObject* Context, UFunction* Function, void* Parms, void* Result)
	{
		if (!IsLLEActive)
			return true; // We have nothing to handle here
		
		const auto funcName = Function->GetName();
		if (strcmp(funcName, "PostRender") == 0 && IsA<ABioHUD>(Context))
		{
			const auto hud = reinterpret_cast<ABioHUD*>(Context);
			if (hud != nullptr)
			{
				hud->FlushPersistentDebugLines();
				if (IsPendingDeactivation)
				{
					IsLLEActive = false;
					ClearSelectedActor();
					LevelsLoadedByLLE.clear();
					return true;
				}
				if (hud->WorldInfo && hud->WorldInfo->Outer && hud->WorldInfo->Outer->Outer && hud->WorldInfo->Outer->Outer->Outer && IsA<ULevelBase>(hud->WorldInfo->Outer))
				{
					const auto streamingLevels = hud->WorldInfo->StreamingLevels;
					std::set<FName> visibleLevels{};

					//Main level
					visibleLevels.insert(hud->WorldInfo->Outer->Outer->Outer->Name);

					for (const auto streaming_level : streamingLevels)
					{
						if (streaming_level->bIsVisible && streaming_level->LoadedLevel)
						{
							visibleLevels.insert(streaming_level->PackageName);
						}
					}

					//check if any levels have been unloaded
					std::vector<FName> diff;
					std::set_difference(LevelsLoadedByLLE.begin(), LevelsLoadedByLLE.end(), visibleLevels.begin(), visibleLevels.end(), std::back_inserter(diff));
					if (!diff.empty())
					{
						RefreshLevels(hud->WorldInfo, &diff);
					}
					else
					{
						//check for newly loaded levels
						std::set_difference(visibleLevels.begin(), visibleLevels.end(), LevelsLoadedByLLE.begin(), LevelsLoadedByLLE.end(), std::back_inserter(diff));
						if (!diff.empty())
						{
							RefreshLevels(hud->WorldInfo);
						}
					}

				}
				if (SelectedActor && DrawLineToSelected) {
					auto line_end = SelectedActor->LOCATION;
					if (SelectedComponentIndex >= 0 && IsA<AStaticMeshCollectionActor>(SelectedActor))
					{
						const auto smca = reinterpret_cast<AStaticMeshCollectionActor*>(SelectedActor);
						const auto comp = smca->Components(SelectedComponentIndex);
						if (IsA<UStaticMeshComponent>(comp))
						{
							line_end = reinterpret_cast<UStaticMeshComponent*>(comp)->Translation;
						}
					}
					DrawDebugLine(SharedData::cachedPlayerPosition, line_end, TraceLineColor, TraceWidth);
					DrawCoordinateSystem(line_end, CoordinateScale, TraceWidth);

					//DO NOT DELETE! Lines of non-zero width will NOT be rendered unless a line of 0 width is also drawn.
					DrawDebugLine(SharedData::cachedPlayerPosition, line_end, TraceLineColor, 0);
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
			ClearSelectedActor();
			LevelsLoadedByLLE.clear();
			SendStringToLEX(L"LIVELEVELEDITOR READY");
			IsLLEActive = true;
			return true;
		}
		if (IsCmd(&command, "LLE_DEACTIVATE"))
		{
			IsLLEActive = false;
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
			TraceLineColor.R = strtof(command, &command);
			TraceLineColor.G = strtof(command, &command);
			TraceLineColor.B = strtof(command, &command);
			return true;
		}

		if (IsCmd(&command, "LLE_TRACE_WIDTH "))
		{
			TraceWidth = strtof(command, &command);
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

	//no instantiation of static class
	LELiveLevelEditor() = delete;
};

// Static variable initialization
AActor* LELiveLevelEditor::SelectedActor = nullptr;
FName LELiveLevelEditor::SelectedActorMapName{};
int LELiveLevelEditor::SelectedComponentIndex = -1;
bool LELiveLevelEditor::DrawLineToSelected = true;
bool LELiveLevelEditor::IsLLEActive = false; 
bool LELiveLevelEditor::IsPendingDeactivation = false; 
FLinearColor LELiveLevelEditor::TraceLineColor = FLinearColor{1, 1, 0, 1};
float LELiveLevelEditor::TraceWidth = 3.f;
float LELiveLevelEditor::CoordinateScale = 100;
std::set<FName> LELiveLevelEditor::LevelsLoadedByLLE{};