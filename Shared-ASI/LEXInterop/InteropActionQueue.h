#pragma once
#include <concurrent_queue.h>

struct ActionBase
{
	virtual ~ActionBase() = default;
	virtual void Execute() = 0;
};

//Actions are posted to this queue from the pipe thread, then popped, executed, and deleted from the main thread
static concurrency::concurrent_queue<ActionBase*> InteropActionQueue{};
