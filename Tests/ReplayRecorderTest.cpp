// Focused unit tests for Engine::ReplayRecorder and the replay control events (Milestone 4
// Checkpoint 4). Pure logic on a fake clock. This target links EngineCore only — no SDL, no
// Timeline, no networking — which is itself the proof the replay core needs none of them.

#include "Engine/Event/ReplayRecorder.h"

#include <cstdio>
#include <vector>

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

using namespace Engine;

namespace
{
	constexpr EventPriority ControlPriority = EventPriority::Critical;

	// A manager + recorder on a clock the test advances by hand, plus a log of every gameplay
	// event that is DISPATCHED (so replayed events are observable exactly like live ones).
	struct Rig
	{
		explicit Rig(ReplayRecorder::EventFilter filter = {})
			: Events([this] { return Now; }), Recorder(Events, [this] { return Now; }, std::move(filter))
		{
			for (EventType type : { EventType::Input, EventType::Collision, EventType::Death, EventType::Spawn })
			{
				Subscriptions.emplace_back(Events, type, [this](const Event& event) { Dispatched.push_back(event); });
			}
		}

		void Control(EventPayload control)
		{
			Events.Raise(std::move(control), ControlPriority);
			Events.DispatchDue();
		}
		void Start() { Control(ReplayStartEventData{}); }
		void Stop() { Control(ReplayStopEventData{}); }
		void Play() { Control(ReplayPlayEventData{}); }

		// Raises a gameplay event now and dispatches it.
		void Emit(EventPayload payload, EventPriority priority = EventPriority::Normal)
		{
			Events.Raise(std::move(payload), priority);
			Events.DispatchDue();
		}

		static EventPayload Primary() { return InputEventData{ InputAction::ActionPrimary, true }; }
		static EventPayload Left(bool pressed) { return InputEventData{ InputAction::MoveLeft, pressed }; }

		double Now = 0.0;
		EventManager Events;
		ReplayRecorder Recorder;
		std::vector<Event> Dispatched;
		std::vector<ScopedSubscription> Subscriptions;
	};

	ObjectId SpawnSubject(const Event& event)
	{
		return std::get<SpawnEventData>(event.Payload).Subject;
	}
}

int main()
{
	// 1-3. The three control events are ordinary events: they travel through EventManager and
	// carry their own EventType.
	{
		double now = 0.0;
		EventManager events([&] { return now; });
		std::vector<EventType> seen;
		for (EventType type : { EventType::ReplayStart, EventType::ReplayStop, EventType::ReplayPlay })
		{
			events.Subscribe(type, [&seen](const Event& event) { seen.push_back(event.Type); });
		}
		events.Raise(ReplayStartEventData{});
		events.Raise(ReplayStopEventData{});
		events.Raise(ReplayPlayEventData{});
		events.DispatchDue();
		Check(seen.size() == 3 && seen[0] == EventType::ReplayStart, "ControlEvent_ReplayStart_RoundTripsThroughEventManager");
		Check(seen.size() == 3 && seen[1] == EventType::ReplayStop, "ControlEvent_ReplayStop_RoundTripsThroughEventManager");
		Check(seen.size() == 3 && seen[2] == EventType::ReplayPlay, "ControlEvent_ReplayPlay_RoundTripsThroughEventManager");
		Check(TypeOf(EventPayload{ ReplayStartEventData{} }) == EventType::ReplayStart &&
		          TypeOf(EventPayload{ ReplayStopEventData{} }) == EventType::ReplayStop &&
		          TypeOf(EventPayload{ ReplayPlayEventData{} }) == EventType::ReplayPlay,
		      "ControlEvent_TypeOf_MatchesPayload");
	}

	// 4. Recording begins only when ReplayStart is HANDLED, not when it is raised.
	{
		Rig rig;
		rig.Emit(Rig::Primary()); // before any recording: not captured
		rig.Events.Raise(ReplayStartEventData{}, ControlPriority);
		Check(!rig.Recorder.IsRecording(), "Recording_NotStartedByMerelyRaisingReplayStart");
		rig.Events.DispatchDue();
		Check(rig.Recorder.IsRecording() && rig.Recorder.RecordedEvents().empty(), "Recording_StartsWhenReplayStartIsHandled");
	}

	// 5 & 6. Events are stored with RELATIVE time, in dispatch order.
	{
		Rig rig;
		rig.Now = 100.0;
		rig.Start();
		rig.Now = 100.5;
		rig.Emit(Rig::Left(true));
		rig.Now = 102.0;
		rig.Emit(Rig::Primary());
		rig.Now = 103.5;
		rig.Emit(Rig::Left(false));
		const auto& log = rig.Recorder.RecordedEvents();
		Check(log.size() == 3 && log[0].RelativeTime == 0.5 && log[1].RelativeTime == 2.0 && log[2].RelativeTime == 3.5,
		      "Recording_StoresTimeRelativeToRecordingStart");
		Check(log.size() == 3 && std::get<InputEventData>(log[0].Payload).Action == InputAction::MoveLeft &&
		          std::get<InputEventData>(log[1].Payload).Action == InputAction::ActionPrimary &&
		          !std::get<InputEventData>(log[2].Payload).Pressed,
		      "Recording_PreservesEventOrder");
	}

	// 7. Control events are never part of the recording (no recursion).
	{
		Rig rig;
		rig.Start();
		rig.Emit(Rig::Primary());
		rig.Control(ReplayStartEventData{}); // restart mid-recording
		rig.Emit(Rig::Primary());
		rig.Stop();
		rig.Play();
		bool onlyGameplay = true;
		for (const RecordedEvent& recorded : rig.Recorder.RecordedEvents())
		{
			EventType type = TypeOf(recorded.Payload);
			onlyGameplay = onlyGameplay && (type == EventType::Input || type == EventType::Collision ||
			                                 type == EventType::Death || type == EventType::Spawn);
		}
		Check(onlyGameplay && rig.Recorder.RecordedEvents().size() == 1, "Recording_ControlEventsAreNeverRecorded");
	}

	// 8. Stop freezes the log.
	{
		Rig rig;
		rig.Start();
		rig.Now = 1.0;
		rig.Emit(Rig::Primary());
		rig.Now = 2.0;
		rig.Stop();
		Check(!rig.Recorder.IsRecording() && rig.Recorder.HasRecording() && rig.Recorder.RecordedDuration() == 2.0,
		      "Stop_EndsRecordingAndKeepsIt");
		rig.Now = 3.0;
		rig.Emit(Rig::Primary());
		rig.Emit(Rig::Left(true));
		Check(rig.Recorder.RecordedEvents().size() == 1, "Stop_FreezesTheLog");
	}

	// 9 & 10. Play schedules each event at a NEW origin + its relative time — never the original
	// absolute timestamp.
	{
		Rig rig;
		rig.Now = 100.0;
		rig.Start();
		rig.Now = 101.0;
		rig.Emit(Rig::Left(true));
		rig.Now = 103.0;
		rig.Emit(Rig::Primary());
		rig.Now = 104.0;
		rig.Stop();
		rig.Dispatched.clear();

		rig.Now = 500.0;
		rig.Play();
		Check(rig.Recorder.IsPlaying() && rig.Recorder.PlaybackStartTime() == 500.0, "Play_EstablishesNewPlaybackOrigin");
		Check(rig.Events.PendingCount() == 2 && rig.Dispatched.empty(), "Play_SchedulesEventsWithoutDispatchingEarly");

		rig.Now = 500.9;
		rig.Events.DispatchDue();
		Check(rig.Dispatched.empty(), "Play_NothingDispatchedBeforeFirstRelativeTime");
		rig.Now = 501.0;
		rig.Events.DispatchDue();
		Check(rig.Dispatched.size() == 1 && rig.Dispatched[0].Timestamp == 501.0, "Play_FirstEventDueAtOriginPlusRelativeTime");
		rig.Now = 503.0;
		rig.Events.DispatchDue();
		Check(rig.Dispatched.size() == 2 && rig.Dispatched[1].Timestamp == 503.0, "Play_SecondEventDueAtOriginPlusRelativeTime");
		Check(rig.Dispatched[0].Timestamp != 101.0 && rig.Dispatched[1].Timestamp != 103.0,
		      "Play_NeverReusesOriginalAbsoluteTimestamps");
	}

	// Equal-time events replay in the order they were originally DISPATCHED, even when their
	// priorities would order them differently in a fresh queue (the Death -> Spawn case); each
	// event's original priority is still kept in the log.
	{
		Rig rig;
		rig.Start();
		rig.Now = 1.0;
		rig.Emit(SpawnEventData{ 1, { 0.0f, 0.0f } }, EventPriority::Low);
		rig.Events.Raise(SpawnEventData{ 2, { 0.0f, 0.0f } }, EventPriority::Critical);
		rig.Events.Raise(SpawnEventData{ 3, { 0.0f, 0.0f } }, EventPriority::Low);
		rig.Events.DispatchDue(); // live order: 1, then 2 (Critical), then 3
		rig.Now = 2.0;
		rig.Stop();
		const auto& log = rig.Recorder.RecordedEvents();
		Check(log.size() == 3 && log[0].Priority == EventPriority::Low && log[1].Priority == EventPriority::Critical,
		      "Recording_KeepsEachEventsOriginalPriority");
		rig.Dispatched.clear();
		rig.Now = 10.0;
		rig.Play();
		rig.Now = 20.0;
		rig.Events.DispatchDue();
		bool sameOrderAsLive = rig.Dispatched.size() == 3 && SpawnSubject(rig.Dispatched[0]) == 1 &&
		                       SpawnSubject(rig.Dispatched[1]) == 2 && SpawnSubject(rig.Dispatched[2]) == 3;
		Check(sameOrderAsLive, "Play_EqualTimeEvents_ReplayInRecordedDispatchOrder");
	}

	// The causal Death -> Spawn chain specifically (Spawn outranks Death in priority).
	{
		Rig rig;
		rig.Start();
		rig.Now = 1.0;
		rig.Emit(DeathEventData{ 1, 0 }, EventPriority::High);
		rig.Emit(SpawnEventData{ 1, { 0.0f, 0.0f } }, EventPriority::Critical);
		rig.Now = 2.0;
		rig.Stop();
		rig.Dispatched.clear();
		rig.Now = 10.0;
		rig.Play();
		rig.Now = 20.0;
		rig.Events.DispatchDue();
		Check(rig.Dispatched.size() == 2 && rig.Dispatched[0].Type == EventType::Death &&
		          rig.Dispatched[1].Type == EventType::Spawn,
		      "Play_DeathStillPrecedesSpawn_DespiteSpawnsHigherPriority");
	}

	// Collision is kept in the log but NOT re-dispatched (its consequences replay instead).
	{
		Rig rig;
		rig.Start();
		rig.Now = 1.0;
		rig.Emit(CollisionEventData{ 1, 2 });
		rig.Emit(DeathEventData{ 1, 2 });
		rig.Now = 2.0;
		rig.Stop();
		Check(rig.Recorder.RecordedEvents().size() == 2, "Collision_IsKeptInTheRecordedLog");
		rig.Dispatched.clear();
		rig.Now = 10.0;
		rig.Play();
		rig.Now = 20.0;
		rig.Events.DispatchDue();
		Check(rig.Dispatched.size() == 1 && rig.Dispatched[0].Type == EventType::Death,
		      "Collision_IsNotRedispatchedDuringPlayback");
	}

	// 11. Empty replay handling: nothing recorded, or an empty recording, never plays.
	{
		Rig rig;
		rig.Play();
		Check(!rig.Recorder.IsPlaying() && !rig.Recorder.HasRecording() && rig.Events.PendingCount() == 0,
		      "Empty_PlayWithNoRecording_IsIgnored");
		rig.Start();
		rig.Stop(); // started and stopped in the same instant, nothing in between
		Check(!rig.Recorder.HasRecording(), "Empty_ZeroLengthRecording_IsNotAValidRecording");
		rig.Play();
		Check(!rig.Recorder.IsPlaying() && rig.Events.PendingCount() == 0, "Empty_PlayOfEmptyRecording_IsIgnored");

		// A quiet but non-zero-length recording IS valid (a game may still have state to show).
		rig.Start();
		rig.Now = 5.0;
		rig.Stop();
		rig.Play();
		Check(rig.Recorder.IsPlaying() && rig.Events.PendingCount() == 0, "Empty_QuietButTimedRecording_PlaysItsSpan");
	}

	// 12. Starting again restarts from scratch.
	{
		Rig rig;
		rig.Now = 10.0;
		rig.Start();
		rig.Now = 11.0;
		rig.Emit(Rig::Primary());
		rig.Now = 12.0;
		rig.Start(); // restart while recording
		Check(rig.Recorder.IsRecording() && rig.Recorder.RecordedEvents().empty() && rig.Recorder.RecordingStartTime() == 12.0,
		      "Start_WhileRecording_RestartsAndClears");
		rig.Now = 13.0;
		rig.Emit(Rig::Left(true));
		rig.Stop();
		Check(rig.Recorder.RecordedEvents().size() == 1 && rig.Recorder.RecordedEvents()[0].RelativeTime == 1.0,
		      "Start_AfterRestart_RecordsFromNewOrigin");
		rig.Start(); // a new recording after a completed one also clears it
		Check(rig.Recorder.RecordedEvents().empty() && !rig.Recorder.HasRecording(), "Start_AfterCompletedRecording_ClearsIt");
	}

	// 13. Stop while idle is a no-op.
	{
		Rig rig;
		rig.Stop();
		Check(!rig.Recorder.IsRecording() && !rig.Recorder.IsPlaying() && !rig.Recorder.HasRecording(), "Stop_WhileIdle_IsNoOp");
	}

	// 14. Replayed events never re-record themselves, and Start/Stop/Play are ignored while playing.
	{
		Rig rig;
		rig.Start();
		rig.Now = 1.0;
		rig.Emit(Rig::Primary());
		rig.Now = 2.0;
		rig.Stop();
		size_t recordedBefore = rig.Recorder.RecordedEvents().size();

		rig.Now = 10.0;
		rig.Play();
		rig.Now = 11.0;
		rig.Events.DispatchDue(); // the replayed ActionPrimary dispatches here
		Check(rig.Dispatched.size() >= 1, "Playback_ReplayedEventWasDispatched");
		Check(rig.Recorder.RecordedEvents().size() == recordedBefore, "Playback_ReplayedEventsAreNotReRecorded");

		rig.Start(); // ignored: would clobber the recording being played
		rig.Stop();  // ignored: scheduled events cannot be cancelled
		rig.Play();  // ignored: no double scheduling
		Check(rig.Recorder.IsPlaying() && !rig.Recorder.IsRecording() && rig.Recorder.RecordedEvents().size() == recordedBefore &&
		          rig.Events.PendingCount() == 0,
		      "Playback_StartStopPlayAreIgnoredWhilePlaying");
	}

	// Play requested mid-recording stops the recording first, then plays it.
	{
		Rig rig;
		rig.Start();
		rig.Now = 1.0;
		rig.Emit(Rig::Primary());
		rig.Now = 2.0;
		rig.Play();
		Check(rig.Recorder.IsPlaying() && !rig.Recorder.IsRecording() && rig.Recorder.RecordedDuration() == 2.0,
		      "Play_WhileRecording_StopsRecordingThenPlays");
	}

	// Playback completion: reported exactly once, only after the whole recorded span elapsed,
	// then the recorder is idle and can record or play again.
	{
		Rig rig;
		rig.Now = 0.0;
		rig.Start();
		rig.Now = 1.0;
		rig.Emit(Rig::Primary());
		rig.Now = 2.0;
		rig.Stop();
		rig.Now = 10.0;
		rig.Play();
		Check(!rig.Recorder.PollPlaybackComplete(), "Complete_NotReportedAtPlaybackStart");
		rig.Now = 11.999;
		rig.Events.DispatchDue();
		Check(!rig.Recorder.PollPlaybackComplete() && rig.Recorder.IsPlaying(), "Complete_NotReportedBeforeSpanElapsed");
		rig.Now = 12.0;
		rig.Events.DispatchDue();
		Check(rig.Recorder.PollPlaybackComplete() && !rig.Recorder.IsPlaying(), "Complete_ReportedWhenSpanElapsed");
		Check(!rig.Recorder.PollPlaybackComplete(), "Complete_ReportedExactlyOnce");

		rig.Dispatched.clear();
		rig.Now = 20.0;
		rig.Play(); // the same recording plays again, from a fresh origin
		rig.Now = 21.0;
		rig.Events.DispatchDue();
		Check(rig.Recorder.IsPlaying() && rig.Dispatched.size() == 1 && rig.Dispatched[0].Timestamp == 21.0,
		      "Complete_RecordingCanBePlayedAgain");
	}

	// A frozen clock (paused Timeline) pauses playback: nothing becomes due, completion waits.
	{
		Rig rig;
		rig.Start();
		rig.Now = 1.0;
		rig.Emit(Rig::Primary());
		rig.Now = 2.0;
		rig.Stop();
		rig.Now = 10.0;
		rig.Play();
		rig.Dispatched.clear();
		for (int frame = 0; frame < 5; ++frame)
		{
			rig.Events.DispatchDue();
			rig.Recorder.PollPlaybackComplete();
		}
		Check(rig.Dispatched.empty() && rig.Recorder.IsPlaying(), "Playback_FrozenClock_HoldsPlayback");
	}

	// The filter keeps unrelated events out of the recording.
	{
		Rig rig([](const Event& event) { return event.Type != EventType::Input; });
		rig.Start();
		rig.Emit(Rig::Primary());
		rig.Emit(DeathEventData{ 1, 0 });
		Check(rig.Recorder.RecordedEvents().size() == 1 && TypeOf(rig.Recorder.RecordedEvents()[0].Payload) == EventType::Death,
		      "Filter_ExcludesEventsTheOwnerDoesNotWant");
	}

	// An event stamped after the recording began but dispatched with an older stamp than the
	// recorder's "now" is clamped to 0, never negative.
	{
		Rig rig;
		rig.Now = 10.0;
		rig.Events.Raise(ReplayStartEventData{}, ControlPriority); // stamped 10.0, handled later
		rig.Events.Schedule(Rig::Primary(), 0.5);                     // stamped 10.5
		rig.Now = 11.0;                                            // recording start will read 11.0
		rig.Events.DispatchDue();
		Check(rig.Recorder.RecordedEvents().size() == 1 && rig.Recorder.RecordedEvents()[0].RelativeTime == 0.0,
		      "Recording_EventStampedBeforeStartHandled_ClampsToZero");
	}

	// 15. Construction validation (and, by linking EngineCore only, no SDL dependency).
	{
		bool threw = false;
		try
		{
			double now = 0.0;
			EventManager events([&] { return now; });
			ReplayRecorder recorder(events, std::function<double()>());
		}
		catch (const std::invalid_argument&)
		{
			threw = true;
		}
		Check(threw, "Constructor_EmptyTimeSource_Throws");
		Check(EventTypeCount == 7, "EventTypes_FourGameplayPlusThreeReplayControl");
	}

	if (g_Failures == 0)
	{
		std::printf("All tests passed.\n");
		return 0;
	}

	std::printf("%d test(s) failed.\n", g_Failures);
	return 1;
}
