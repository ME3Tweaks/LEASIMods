#pragma once
#include <winbase.h>

#include "../Common.h"
#include "FileLoader.h"
#include "InteropActionQueue.h"
#include "LEAnimViewer.h"
#include "LELiveLevelEditor.h"
#include "LEPathfindingGPS.h"
#include "GenericCommands.h"

inline void ProcessCommand(char* str, DWORD bytesRead, const HANDLE pipe)
{
	writeln("Received command: %hs", str);

	const bool handled =
	   FileLoader::HandleCommand(str, bytesRead, pipe)
	|| GenericCommands::HandleCommand(str)
	|| LEPathfindingGPS::HandleCommand(str)
	|| LELiveLevelEditor::HandleCommand(str)
	|| LEAnimViewer::HandleCommand(str);

	if (!handled) {
		writeln("Unhandled command!");
	}
	//if (!handled) handled = LE1AnimViewer::HandleCommand(str);
}

constexpr auto LEXPIPE_COMMAND_BUFFER_SIZE = 1024;

inline void HandlePipe()
{
	// Setup the LEX <-> LE game pipe
	char buffer[LEXPIPE_COMMAND_BUFFER_SIZE]{};
	DWORD bytesRead;
	HANDLE hPipe;
#if defined GAMELE1
	hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\LEX_LE1_COMM_PIPE"),
#elif defined GAMELE2
	hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\LEX_LE2_COMM_PIPE"),
#elif defined GAMELE3
	hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\LEX_LE3_COMM_PIPE"),
#endif
		PIPE_ACCESS_INBOUND,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
		1,
		LEXPIPE_COMMAND_BUFFER_SIZE * 16,
		LEXPIPE_COMMAND_BUFFER_SIZE * 16,
		NMPWAIT_USE_DEFAULT_WAIT,
		nullptr);

	if (hPipe != nullptr)
	{
		writeln("PIPED UP");

		while (hPipe != INVALID_HANDLE_VALUE)
		{
			if (ConnectNamedPipe(hPipe, nullptr) != FALSE)   // wait for someone to connect to the pipe
			{
				while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) != FALSE)
				{
					/* add terminating zero */
					buffer[bytesRead] = '\0';
					ProcessCommand(buffer, bytesRead, hPipe);
				}
			}

			//writeln("FLUSHING THE PIPES AWAY");
			DisconnectNamedPipe(hPipe);
		}
	}
	else
	{
		writeln("COULD NOT CREATE INTEROP PIPE");
	}
}