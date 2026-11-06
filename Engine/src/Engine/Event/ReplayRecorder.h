#pragma once

#include "Engine/Event/EventManager.h"

#include <functional>
#include <vector>

namespace Engine
{
	/// One recorded gameplay event: what happened, how urgent it was (kept for inspection;
	/// playback does not reuse it, see ReplayRecorder), and WHEN, measured from
	/// the start of the recording — never an absolute Timeline reading.
	struct RecordedEvent
	{
		double RelativeTime;
		EventPriority Priority;
		EventPayload Payload;
	};

	/// Event-driven replay core (Milestone 4). A replay is built entirely on the EventManager,
	/// not beside it:
	///
	///  - CONTROL is three ordinary events: ReplayStart, ReplayStop, ReplayPlay. This class
	///    subscribes to them, so nothing happens when a control event is merely raised — only
	///    when it is dispatched.
	///  - RECORDING is an EventManager subscription to the gameplay events (Input, Collision,
	///    Death, Spawn). EventManager itself knows nothing about replay.
	///  - PLAYBACK re-schedules the recorded events through the SAME EventManager, so the same
	///    handlers see them in the same order.
	///
	/// It stores only events. State a game needs in order to reproduce what it SAW (positions,
	/// camera, ...) is the game's own business — see Spare Parts' ReplayController — because
	/// events alone do not reproduce a trajectory when frame times vary.
	///
	/// Semantics (each covered by a test):
	///  ReplayStart  clears any previous recording and starts a new one, stamped from "now" in
	///               logical time. Starting again while recording simply restarts it. Ignored
	///               during playback.
	///  ReplayStop   ends the recording and freezes the log; nothing more is added. A no-op
	///               when idle. Ignored during playback — an EventManager cannot cancel events
	///               already scheduled, so playback always runs to completion.
	///  ReplayPlay   needs a completed, non-empty recording (otherwise ignored). If a
	///               recording is still in progress it is stopped first. Then every recorded
	///               event except Collision is scheduled at (playback start + its relative
	///               time). All replay at one priority, so events recorded at the same instant
	///               replay in the order they were originally dispatched (a recorded
	///               event's own priority is kept in the log for inspection only). Ignored
	///               while already playing.
	///
	/// Time: all stamps are the logical time the EventManager's own time source reports, so a
	/// paused or scaled Timeline pauses or scales a recording and its playback alike. Recording
	/// stores RELATIVE times and playback uses a fresh origin; the original absolute timestamps
	/// are never reused.
	///
	/// Which events are recorded: Input, Collision, Death and Spawn that pass the optional
	/// filter (a game uses it to keep only events about its own player). Replay control events
	/// are never recorded, and nothing is recorded during playback, so a replay cannot record
	/// itself.
	///
	/// Collision policy: Collision is DERIVED state — the Death/Spawn it caused are recorded as
	/// events of their own. So Collision events are kept in the log (for inspection) but not
	/// re-dispatched; replaying both would run each consequence twice.
	///
	/// Main-thread only, like EventManager. Not copyable or movable (handlers capture `this`).
	class ReplayRecorder
	{
	public:
		/// Decides whether a dispatched gameplay event belongs in the recording. Empty = all.
		using EventFilter = std::function<bool(const Event&)>;

		/// `events` must outlive the recorder. `timeSource` must be the same logical clock the
		/// EventManager uses. Subscribes immediately; unsubscribes on destruction.
		ReplayRecorder(EventManager& events, std::function<double()> timeSource, EventFilter filter = {});

		ReplayRecorder(const ReplayRecorder&) = delete;
		ReplayRecorder& operator=(const ReplayRecorder&) = delete;

		bool IsRecording() const;
		bool IsPlaying() const;

		/// True once a recording has been started AND stopped and it is not empty (it spans
		/// some logical time or holds at least one event).
		bool HasRecording() const;

		/// The recorded events in dispatch order (frozen once recording stops).
		const std::vector<RecordedEvent>& RecordedEvents() const;

		/// Logical time the current/last recording began.
		double RecordingStartTime() const;

		/// Logical length of the completed recording (stop time minus start time).
		double RecordedDuration() const;

		/// Logical time the current/last playback began (its time origin).
		double PlaybackStartTime() const;

		/// Call once per update, after DispatchDue(). Returns true exactly once, on the
		/// update where a playback's whole recorded span has elapsed (every scheduled event is
		/// then already due, hence already dispatched); the recorder becomes idle again.
		bool PollPlaybackComplete();

		/// Abandons a playback the owner decided it cannot perform (e.g. it has nothing to
		/// show). Events already scheduled stay queued, so only use it when none were.
		void EndPlayback();

	private:
		enum class State
		{
			Idle,
			Recording,
			Playing
		};

		void OnStart();
		void OnStop();
		void OnPlay();
		void OnGameplayEvent(const Event& event);
		void StopRecording();

		EventManager& m_Events;
		std::function<double()> m_TimeSource;
		EventFilter m_Filter;

		State m_State = State::Idle;
		std::vector<RecordedEvent> m_Recorded;
		double m_RecordingStart = 0.0;
		double m_RecordedDuration = 0.0;
		bool m_HasCompletedRecording = false;
		double m_PlaybackStart = 0.0;

		// Declared last: destroyed first, unsubscribing while the EventManager is still alive.
		std::vector<ScopedSubscription> m_Subscriptions;
	};
}
