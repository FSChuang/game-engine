// Focused unit tests for Engine::Timeline (ENGINEERING_SPEC.md §10). Pure logic, no SDL window.
// Every test injects a fake monotonic anchor (a captured double advanced by hand) instead of
// real time, so results are exact and independent of wall-clock timing (F.I.R.S.T.: repeatable).
// Test values are chosen as exact binary fractions so results are exactly representable in
// double, avoiding the need for epsilon-based comparison.

#include "Engine/Time/Timeline.h"

#include <cstdio>
#include <functional>
#include <stdexcept>

static int g_Failures = 0;

static void Check(bool condition, const char* name)
{
	if (condition)
	{
		std::printf("[PASS] %s\n", name);
	}
	else
	{
		std::printf("[FAIL] %s\n", name);
		++g_Failures;
	}
}

// Runs action and reports whether it threw std::invalid_argument (any other outcome, including
// no exception, counts as false).
static bool ThrowsInvalidArgument(const std::function<void()>& action)
{
	try
	{
		action();
	}
	catch (const std::invalid_argument&)
	{
		return true;
	}
	return false;
}

int main()
{
	// Arrange / Act / Assert: scale 1.0 advances logical time exactly with the anchor.
	{
		double fakeTime = 0.0;
		Engine::Timeline timeline([&fakeTime]() { return fakeTime; });

		fakeTime = 2.0;
		double delta = timeline.GetDeltaTime();

		Check(delta == 2.0, "GetDeltaTime_ScaleOne_MatchesAnchorAdvance");
		Check(timeline.GetTime() == 2.0, "GetTime_ScaleOne_AccumulatesAnchorAdvance");
	}

	// Act / Assert: scale 0.5 advances logical time at half the anchor's rate.
	{
		double fakeTime = 0.0;
		Engine::Timeline timeline([&fakeTime]() { return fakeTime; });
		timeline.SetScale(0.5);

		fakeTime = 4.0;
		double delta = timeline.GetDeltaTime();

		Check(delta == 2.0, "GetDeltaTime_ScaleHalf_AdvancesAtHalfRate");
	}

	// Act / Assert: scale 2.0 advances logical time at double the anchor's rate.
	{
		double fakeTime = 0.0;
		Engine::Timeline timeline([&fakeTime]() { return fakeTime; });
		timeline.SetScale(2.0);

		fakeTime = 1.5;
		double delta = timeline.GetDeltaTime();

		Check(delta == 3.0, "GetDeltaTime_ScaleTwo_AdvancesAtDoubleRate");
	}

	// Act / Assert: changing scale does not jump the current logical time, and only affects
	// time elapsed after the change (sampled at the top level, no unsampled interval involved).
	{
		double fakeTime = 0.0;
		Engine::Timeline timeline([&fakeTime]() { return fakeTime; });

		fakeTime = 3.0;
		timeline.GetDeltaTime();
		Check(timeline.GetTime() == 3.0, "ScaleChange_BeforeChange_TimeReflectsOriginalScale");

		timeline.SetScale(2.0);
		Check(timeline.GetTime() == 3.0, "ScaleChange_AtMomentOfChange_TimeDoesNotJump");

		fakeTime = 4.0;
		double delta = timeline.GetDeltaTime();
		Check(delta == 2.0, "ScaleChange_AfterChange_NewDeltaUsesNewScale");
		Check(timeline.GetTime() == 5.0, "ScaleChange_AfterChange_TimeContinuesFromPriorValue");
	}

	// Act / Assert (Issue 1, case 1): an anchor interval that elapses BEFORE a scale change
	// (1.0 -> 2.0) must be credited at the OLD scale, never retroactively reinterpreted; only a
	// later, newly-elapsed interval uses the new scale.
	{
		double fakeTime = 0.0;
		Engine::Timeline timeline([&fakeTime]() { return fakeTime; });

		fakeTime = 1.0; // one anchor unit elapses, unsampled, still under scale 1.0
		timeline.SetScale(2.0); // must not reinterpret the unsampled unit at the new scale

		double firstDelta = timeline.GetDeltaTime();
		Check(firstDelta == 1.0, "UnsampledIntervalBeforeScaleChange_OneToTwo_CreditsOldScale");
		Check(timeline.GetTime() == 1.0, "UnsampledIntervalBeforeScaleChange_OneToTwo_TimeMatchesOldScale");

		fakeTime = 2.0; // one further anchor unit, now under scale 2.0
		double secondDelta = timeline.GetDeltaTime();
		Check(secondDelta == 2.0, "UnsampledIntervalBeforeScaleChange_OneToTwo_SubsequentDeltaUsesNewScale");
		Check(timeline.GetTime() == 3.0, "UnsampledIntervalBeforeScaleChange_OneToTwo_TimeAccumulatesBothIntervals");
	}

	// Act / Assert (Issue 1, case 2): the same transition, in the other direction (2.0 -> 0.5).
	{
		double fakeTime = 0.0;
		Engine::Timeline timeline([&fakeTime]() { return fakeTime; });
		timeline.SetScale(2.0);

		fakeTime = 1.0; // one anchor unit elapses, unsampled, still under scale 2.0
		timeline.SetScale(0.5);

		double firstDelta = timeline.GetDeltaTime();
		Check(firstDelta == 2.0, "UnsampledIntervalBeforeScaleChange_TwoToHalf_CreditsOldScale");

		fakeTime = 3.0; // two further anchor units, now under scale 0.5
		double secondDelta = timeline.GetDeltaTime();
		Check(secondDelta == 1.0, "UnsampledIntervalBeforeScaleChange_TwoToHalf_SubsequentDeltaUsesNewScale");
	}

	// Act / Assert (Issue 1, case 3): the same rule applies to tic size, independent of scale.
	{
		double fakeTime = 0.0;
		Engine::Timeline timeline([&fakeTime]() { return fakeTime; });

		fakeTime = 4.0; // elapses, unsampled, still under tic size 1.0
		timeline.SetTicSize(2.0);

		double firstDelta = timeline.GetDeltaTime();
		Check(firstDelta == 4.0, "UnsampledIntervalBeforeTicSizeChange_CreditsOldTicSize");

		fakeTime = 8.0; // four further anchor units, now under tic size 2.0
		double secondDelta = timeline.GetDeltaTime();
		Check(secondDelta == 2.0, "UnsampledIntervalBeforeTicSizeChange_SubsequentDeltaUsesNewTicSize");
	}

	// Act / Assert (Issue 1, case 4): GetTime stays continuous through a scale change — the
	// flushed interval is credited once, at the old scale, not by rescaling the whole history.
	{
		double fakeTime = 0.0;
		Engine::Timeline timeline([&fakeTime]() { return fakeTime; });

		fakeTime = 10.0;
		timeline.GetDeltaTime(); // two sampled intervals of accumulated history: GetTime() == 10.0

		fakeTime = 11.0; // one further anchor unit, unsampled
		double timeBeforeScaleChange = timeline.GetTime();
		timeline.SetScale(2.0);
		double timeAfterScaleChange = timeline.GetTime();

		// The flush must add exactly the one unsampled unit at the OLD scale (1.0), landing at
		// 11.0 — not e.g. 22.0, which is what accidentally rescaling all prior history would give.
		Check(timeAfterScaleChange == timeBeforeScaleChange + 1.0,
		      "ScaleChange_GetTime_OnlyCreditsUnsampledIntervalAtOldScale_NotEntireHistory");
		Check(timeAfterScaleChange == 11.0, "ScaleChange_GetTime_ContinuousThroughChange");
	}

	// Act / Assert (Issue 1, case 5): the elapsed delta flushed by SetScale()/SetTicSize() is not
	// silently lost — it is still delivered the next time GetDeltaTime() is called, even with no
	// further anchor advance in between.
	{
		double fakeTime = 0.0;
		Engine::Timeline timeline([&fakeTime]() { return fakeTime; });

		fakeTime = 5.0;
		timeline.SetScale(2.0); // flushes the 5.0-unit interval into the pending delta

		double delta = timeline.GetDeltaTime();
		Check(delta == 5.0, "SetScale_FlushedDelta_IsNotLost_StillDeliveredByNextGetDeltaTime");
	}

	// Act / Assert: pause freezes logical time and zeroes delta even as the anchor advances.
	{
		double fakeTime = 0.0;
		Engine::Timeline timeline([&fakeTime]() { return fakeTime; });

		fakeTime = 2.0;
		timeline.GetDeltaTime();

		timeline.Pause();
		Check(timeline.IsPaused(), "Pause_SetsIsPaused");

		fakeTime = 12.0;
		double delta = timeline.GetDeltaTime();

		Check(delta == 0.0, "Pause_GetDeltaTime_ReturnsZeroWhileAnchorAdvances");
		Check(timeline.GetTime() == 2.0, "Pause_GetTime_RemainsFixedWhileAnchorAdvances");
	}

	// Act / Assert: unpausing resumes from the paused position, with no catch-up jump for the
	// time that passed while paused.
	{
		double fakeTime = 0.0;
		Engine::Timeline timeline([&fakeTime]() { return fakeTime; });

		fakeTime = 1.0;
		timeline.GetDeltaTime();

		timeline.Pause();
		fakeTime = 6.0; // time passes while paused; never sampled by GetDeltaTime()
		timeline.Unpause();

		Check(!timeline.IsPaused(), "Unpause_ClearsIsPaused");
		Check(timeline.GetTime() == 1.0, "Unpause_TimeUnchangedAtMomentOfUnpause");

		fakeTime = 7.0;
		double delta = timeline.GetDeltaTime();

		Check(delta == 1.0, "Unpause_SubsequentDelta_ExcludesPausedDuration");
		Check(timeline.GetTime() == 2.0, "Unpause_SubsequentTime_ExcludesPausedDuration");
	}

	// Act / Assert: pending delta flushed by Pause() itself is not lost — it is delivered on the
	// very next GetDeltaTime() call, before any post-unpause time is added.
	{
		double fakeTime = 0.0;
		Engine::Timeline timeline([&fakeTime]() { return fakeTime; });

		fakeTime = 3.0; // elapses, unsampled
		timeline.Pause(); // flushes the 3.0-unit interval into the pending delta, then freezes

		double delta = timeline.GetDeltaTime();
		Check(delta == 3.0, "Pause_FlushedDelta_IsNotLost_StillDeliveredByNextGetDeltaTime");
		Check(timeline.IsPaused(), "Pause_StillPausedAfterDeliveringFlushedDelta");
	}

	// Act / Assert: scale and tic size still behave correctly after a pause/unpause cycle.
	{
		double fakeTime = 0.0;
		Engine::Timeline timeline([&fakeTime]() { return fakeTime; });

		fakeTime = 1.0;
		timeline.GetDeltaTime();

		timeline.Pause();
		fakeTime = 5.0;
		timeline.Unpause();

		timeline.SetScale(2.0);
		fakeTime = 6.0;
		double delta = timeline.GetDeltaTime();

		Check(delta == 2.0, "ScaleAfterPauseUnpause_StillAppliesCorrectly");
		Check(timeline.GetTime() == 3.0, "TimeAfterPauseUnpause_StillAccumulatesCorrectly");
	}

	// Act / Assert: a child Timeline anchored to a parent advances when the parent advances.
	{
		double fakeTime = 0.0;
		Engine::Timeline parent([&fakeTime]() { return fakeTime; });
		Engine::Timeline child(parent);

		fakeTime = 4.0;
		double parentDelta = parent.GetDeltaTime();
		double childDelta = child.GetDeltaTime();

		Check(parentDelta == 4.0, "ChildTimeline_ParentAdvance_ParentDeltaMatchesAnchor");
		Check(childDelta == 4.0, "ChildTimeline_ParentAdvance_PropagatesToChild");
		Check(child.GetTime() == 4.0, "ChildTimeline_ParentAdvance_ChildTimeMatchesParentDelta");
	}

	// Act / Assert: scaling the parent changes the rate the child observes, composing with the
	// child's own scale (parent 0.5x + child 1.0x => child advances at half the anchor's rate).
	{
		double fakeTime = 0.0;
		Engine::Timeline parent([&fakeTime]() { return fakeTime; });
		Engine::Timeline child(parent);
		parent.SetScale(0.5);

		fakeTime = 4.0;
		parent.GetDeltaTime();
		double childDelta = child.GetDeltaTime();

		Check(childDelta == 2.0, "ChildTimeline_ParentScaleHalf_ChildInheritsHalfRate");
	}

	// Act / Assert: pausing the parent freezes the child even though the child itself was never
	// paused; this falls out of the child observing the parent's frozen logical time.
	{
		double fakeTime = 0.0;
		Engine::Timeline parent([&fakeTime]() { return fakeTime; });
		Engine::Timeline child(parent);

		parent.Pause();
		fakeTime = 5.0;
		parent.GetDeltaTime();
		double childDelta = child.GetDeltaTime();

		Check(!child.IsPaused(), "ChildTimeline_ParentPause_ChildItselfRemainsUnpaused");
		Check(childDelta == 0.0, "ChildTimeline_ParentPause_ChildDoesNotAdvance");
	}

	// Act / Assert: child scale composes with parent scale (parent 0.5x + child 2.0x => child
	// advances at the anchor's real-time rate).
	{
		double fakeTime = 0.0;
		Engine::Timeline parent([&fakeTime]() { return fakeTime; });
		Engine::Timeline child(parent);
		parent.SetScale(0.5);
		child.SetScale(2.0);

		fakeTime = 4.0;
		parent.GetDeltaTime();
		double childDelta = child.GetDeltaTime();

		Check(childDelta == 4.0, "ChildTimeline_ParentHalfChildDouble_ComposesToAnchorRate");
	}

	// Act / Assert: tic size scales logical time against anchor time, independent of scale.
	{
		double fakeTime = 0.0;
		Engine::Timeline timeline([&fakeTime]() { return fakeTime; });
		timeline.SetTicSize(2.0);

		fakeTime = 4.0;
		double delta = timeline.GetDeltaTime();

		Check(delta == 2.0, "TicSize_Two_HalvesLogicalAdvancePerAnchorUnit");
	}
	{
		double fakeTime = 0.0;
		Engine::Timeline timeline([&fakeTime]() { return fakeTime; });
		timeline.SetTicSize(0.5);

		fakeTime = 1.0;
		double delta = timeline.GetDeltaTime();

		Check(delta == 2.0, "TicSize_Half_DoublesLogicalAdvancePerAnchorUnit");
	}

	// Act / Assert (Issue 2): invalid scale is rejected via std::invalid_argument, and the
	// previous valid value survives the rejected call.
	{
		Engine::Timeline timeline([]() { return 0.0; });

		Check(ThrowsInvalidArgument([&timeline]() { timeline.SetScale(0.0); }), "SetScale_Zero_Throws");
		Check(timeline.GetScale() == 1.0, "SetScale_Zero_PreviousValueRetained");

		Check(ThrowsInvalidArgument([&timeline]() { timeline.SetScale(-1.0); }), "SetScale_Negative_Throws");
		Check(timeline.GetScale() == 1.0, "SetScale_Negative_PreviousValueRetained");
	}

	// Act / Assert (Issue 2): invalid tic size is rejected via std::invalid_argument, and the
	// previous valid value survives the rejected call.
	{
		Engine::Timeline timeline([]() { return 0.0; });

		Check(ThrowsInvalidArgument([&timeline]() { timeline.SetTicSize(0.0); }), "SetTicSize_Zero_Throws");
		Check(timeline.GetTicSize() == 1.0, "SetTicSize_Zero_PreviousValueRetained");

		Check(ThrowsInvalidArgument([&timeline]() { timeline.SetTicSize(-1.0); }), "SetTicSize_Negative_Throws");
		Check(timeline.GetTicSize() == 1.0, "SetTicSize_Negative_PreviousValueRetained");
	}

	// Act / Assert: GetDeltaTime() only ever reports newly elapsed logical time, never the
	// cumulative total.
	{
		double fakeTime = 0.0;
		Engine::Timeline timeline([&fakeTime]() { return fakeTime; });

		fakeTime = 3.0;
		double firstDelta = timeline.GetDeltaTime();

		fakeTime = 5.0;
		double secondDelta = timeline.GetDeltaTime();

		Check(firstDelta == 3.0, "GetDeltaTime_FirstSample_ReturnsElapsedSinceConstruction");
		Check(secondDelta == 2.0, "GetDeltaTime_SecondSample_ReturnsOnlyNewlyElapsedTime");
		Check(timeline.GetTime() == 5.0, "GetDeltaTime_RepeatedSampling_TimeAccumulatesCorrectly");
	}

	std::printf("\n%s\n", g_Failures == 0 ? "All tests passed." : "Some tests FAILED.");
	return g_Failures == 0 ? 0 : 1;
}
