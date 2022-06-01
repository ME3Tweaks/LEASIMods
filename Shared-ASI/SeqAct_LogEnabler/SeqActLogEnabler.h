#include <stdio.h>
#include <io.h>
#include <string>
#include <fstream>
#include <iostream>
#include <ostream>
#include <streambuf>
#include <sstream>

#define ASINAME L"SeqActLogEnabler"
#define MYHOOK "SeqActLogEnabler_"
#define VERSION L"4.0.0"

#ifdef GAMELE1
#include "../../LE1-ASI-Plugins/LE1-SDK/Interface.h"
SPI_PLUGINSIDE_SUPPORT(ASINAME, VERSION, L"ME3Tweaks", SPI_GAME_LE1, SPI_VERSION_ANY);
#endif
#ifdef GAMELE2
#include "../../LE2-ASI-Plugins/LE2-SDK/Interface.h"
SPI_PLUGINSIDE_SUPPORT(ASINAME, VERSION, L"ME3Tweaks", SPI_GAME_LE2, SPI_VERSION_ANY);
#endif
#ifdef GAMELE3
#include "../../LE3-ASI-Plugins/LE3-SDK/Interface.h"
SPI_PLUGINSIDE_SUPPORT(ASINAME, VERSION, L"ME3Tweaks", SPI_GAME_LE3, SPI_VERSION_ANY);
#endif

#include "../../Shared-ASI/Common.h"
#include "../../Shared-ASI/ME3Tweaks/ME3TweaksHeader.h"
#include "../../Shared-ASI/ME3Tweaks/ScreenLogger.h"

SPI_PLUGINSIDE_POSTLOAD;
SPI_PLUGINSIDE_ASYNCATTACH;

ME3TweaksASILogger logger("SeqAct_Log Enabler v4", "SeqActLog.log");
ScreenLogger screenLogger(L"SeqAct_Log Enabler v4");

std::wstringstream ss;

std::wstringstream& operator<<(std::wstringstream& ss, FString& fStr)
{

	for (auto i = 0; i < fStr.Count && fStr(i) != 0; i++)
	{
		ss << fStr(i);
	}
	return ss;
}


USequenceVariable* ResolveSeqVar_External(USeqVar_External* seqExtObj)
{
	// Look for parent sequence, find matching var link
	USequence* parentSeq = seqExtObj->ParentSequence;
	if (parentSeq != nullptr)
	{
		// Enumerate over the parent sequence objects
		for (int i = 0; i < parentSeq->VariableLinks.Count; i++)
		{
			if (!strcmp(parentSeq->VariableLinks.Data[i].LinkVar.GetName(), seqExtObj->VarName.GetName())) // Find matching link
			{
				// Find matching linked variable
				auto link = parentSeq->VariableLinks.Data[i];
				for (int j = 0; j < link.LinkedVariables.Count; j++) {
					auto variable = link.LinkedVariables.Data[j];
					if (variable != nullptr)
					{
						// See if this is also an external variable, in which case we have to go up further
						if (variable->IsA(USeqVar_External::StaticClass()))
						{
							return ResolveSeqVar_External(static_cast<USeqVar_External*>(variable));
						}
						else
						{
							// This is the object we're looking for!
							return variable;
						}
					}
				}
			}
		}
	}

	return nullptr; // Not found
}


// ProcessEvent hook
// ======================================================================

typedef void (*tProcessEvent)(UObject* Context, UFunction* Function, void* Parms, void* Result);
tProcessEvent ProcessEvent = nullptr;
tProcessEvent ProcessEvent_orig = nullptr;


void PrintSequenceVariable(USequenceVariable* seqVar)
{
	// Exteranl goes first cause of some UE3 black magic where it acts like an object.
	auto className = seqVar->Class->GetName();
	if (seqVar->IsA(USeqVar_External::StaticClass()))
	{
		auto seqExtObj = static_cast<USeqVar_External*>(seqVar);
		auto referencedObj = ResolveSeqVar_External(seqExtObj);
		if (referencedObj != nullptr)
		{
			PrintSequenceVariable(referencedObj);
		}
		else
		{
			ss << L"SeqVar_External couldn't resolve variable ";
			ss << seqExtObj->VariableLabel.Data;
		}
	}
	else	if (seqVar->IsA(USeqVar_String::StaticClass()))
	{
		ss << static_cast<USeqVar_String*>(seqVar)->StrValue << " ";
	}
	else if (seqVar->IsA(USeqVar_Float::StaticClass()))
	{
		ss << static_cast<USeqVar_Float*>(seqVar)->FloatValue << " ";
	}
	else if (seqVar->IsA(USeqVar_Int::StaticClass()))
	{
		ss << static_cast<USeqVar_Int*>(seqVar)->IntValue << " ";
	}
	else if (seqVar->IsA(USeqVar_Bool::StaticClass()))
	{
		ss << (static_cast<USeqVar_Bool*>(seqVar)->bValue ? "True" : "False") << " ";
	}
	else if (seqVar->IsA(USeqVar_Vector::StaticClass()))
	{
		auto vector = static_cast<USeqVar_Vector*>(seqVar);
		ss << "Vector X=" << vector->VectValue.X << " Y=" << vector->VectValue.Y << " Z=" << vector->VectValue.Z << " ";
	}
	else if (seqVar->IsA(USeqVar_Object::StaticClass()))
	{
		auto seqVarObj = static_cast<USeqVar_Object*>(seqVar);
		auto referencedObj = seqVarObj->ObjValue;
		if (referencedObj != nullptr)
		{
			ss << referencedObj->GetName() << " ";
		}
	}
	else if (seqVar->IsA(USeqVar_Name::StaticClass()))
	{
		ss << static_cast<USeqVar_Name*>(seqVar)->NameValue.GetName() << " ";
	}
	else
	{
		ss << L"Unknown SeqVar type: ";
		ss << seqVar->Class->GetName();
	}
}

void ProcessEvent_hook(UObject* Context, UFunction* Function, void* Parms, void* Result)
{
	if (!strcmp(Function->GetFullName(), "Function Engine.SequenceOp.Activated") && Context->IsA(USeqAct_Log::StaticClass()))
	{
		ss = std::wstringstream(); // Make new because i have no idea what i'm doing
		auto seqLog = reinterpret_cast<USeqAct_Log*>(Context);
		auto numVarLinks = seqLog->VariableLinks.Count;

		if (seqLog->bOutputObjCommentToScreen == 1ul)
		{
			auto numLines = seqLog->m_aObjComment.Num();
			for (auto k = 0; k < numLines; k++)
			{
				ss << seqLog->m_aObjComment(k).Data;
				ss << L" ";
			}
		}

		for (auto i = 0; i < numVarLinks; i++)
		{
			auto numVars = seqLog->VariableLinks(i).LinkedVariables.Count;
			for (auto j = 0; j < numVars; j++)
			{
				auto seqVar = seqLog->VariableLinks(i).LinkedVariables(j);
				PrintSequenceVariable(seqVar);
			}
		}

		wstring msg = ss.str();
		writeln(L"SeqAct_Log: %s", msg.c_str());
		if (!msg.empty())
		{
			logger.writeToLog(msg.c_str(), true, true);
			screenLogger.LogMessage(msg);
		}
	}
	else if (!strcmp(Function->GetFullName(), "Function SFXGame.BioHUD.PostRender"))
	{
		auto hud = static_cast<ABioHUD*>(Context);
		screenLogger.PostRenderer(hud);
	}
	ProcessEvent_orig(Context, Function, Parms, Result);
}



SPI_IMPLEMENT_ATTACH
{
	Common::OpenConsole();

	auto _ = SDKInitializer::Instance();

	INIT_FIND_PATTERN_POSTHOOK(ProcessEvent, LE_PATTERN_POSTHOOK_PROCESSEVENT);
	INIT_HOOK_PATTERN(ProcessEvent);

	return true;
}

SPI_IMPLEMENT_DETACH
{
	logger.flush();
	Common::CloseConsole();
	return true;
}
