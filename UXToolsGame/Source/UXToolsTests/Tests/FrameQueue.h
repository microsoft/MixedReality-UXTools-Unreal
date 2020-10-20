// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"

#include "Containers/Queue.h"

#include <functional>

// Helper to enqueue items to run on subsequent frames
// Each item runs on a different frame.
class FFrameQueue
{
public:
	void Init(FTimerManager* TimerManagerIn)
	{
		TimerManager = TimerManagerIn;
		Reset();
	}

	void Reset()
	{
		Functions.Empty();
		Paused = false;
	}

	void Pause() { Paused = true; }

	void Resume()
	{
		if (Paused)
		{
			Paused = false;
			ScheduleTick();
		}
	}

	void Enqueue(TFunction<void()>&& Func)
	{
		if (Functions.IsEmpty() && !Paused)
		{
			ScheduleTick();
		}

		Functions.Enqueue(Func);
	}

	void Skip(int NumFrames = 1)
	{
		for (int i = 0; i < NumFrames; i++)
		{
			Enqueue([] {});
		}
	}

private:
	void ScheduleTick() { TimerManager->SetTimerForNextTick(std::bind(&FFrameQueue::RunNextFunction, this)); }

	void RunNextFunction()
	{
		if (!Functions.IsEmpty() && !Paused)
		{
			TFunction<void()> Func;
			Functions.Dequeue(Func);
			Func();
		}

		if (!Functions.IsEmpty() && !Paused)
		{
			ScheduleTick();
		}
	}

	TQueue<TFunction<void()>> Functions;
	FTimerManager* TimerManager;
	bool Paused = false;
};
