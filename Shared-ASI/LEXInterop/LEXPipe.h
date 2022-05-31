void ProcessCommand(char str[1024], DWORD dword)
{
	// Remove /r/n
	auto test = str;
	while (*test != '\r')
	{
		test++;
	}
	*test = 0; // This will remove \r\n from the string

	writeln("Received command: %hs", str);

	bool handled = LE1GenericCommands::HandleCommand(str);
	if (!handled) handled = LEPathfindingGPS::HandleCommand(str);
	if (!handled) handled = LELiveLevelEditor::HandleCommand(str);
	if (!handled) handled = LEAnimViewer::HandleCommand(str);
	//if (!handled) handled = LE1AnimViewer::HandleCommand(str);
}

void HandlePipe()
{
	// Setup the LEX <-> LE game pipe
	char buffer[1024];
	DWORD dwRead;

#ifdef LE1
	hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\LEX_LE1_COMM_PIPE"),
#elif LE2
	hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\LEX_LE2_COMM_PIPE"),
#elif LE3
	hPipe = CreateNamedPipe(TEXT("\\\\.\\pipe\\LEX_LE3_COMM_PIPE"),
#endif
		PIPE_ACCESS_INBOUND,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
		1,
		1024 * 16,
		1024 * 16,
		NMPWAIT_USE_DEFAULT_WAIT,
		NULL);

	if (hPipe != nullptr)
		writeln("PIPED UP");
	else
		writeln("COULD NOT CREATE INTEROP PIPE");

	while (hPipe != INVALID_HANDLE_VALUE)
	{
		if (ConnectNamedPipe(hPipe, NULL) != FALSE)   // wait for someone to connect to the pipe
		{
			while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) != FALSE)
			{
				/* add terminating zero */
				buffer[dwRead] = '\0';
				ProcessCommand(buffer, dwRead);
			}
		}

		//writeln("FLUSHING THE PIPES AWAY");
		DisconnectNamedPipe(hPipe);
	}
}