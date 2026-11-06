#include "Engine/Event/ReplayRecorder.h"

#include <algorithm>
#include <utility>

namespace Engine
{
	namespace
	{
		constexpr EventPriority PlaybackPriority = EventPriority::Normal;
	}

	ReplayRecorder::ReplayRecorder(EventManager& events, std::function<double()> timeSource, EventFilter filter)
		: m_Events(events), m_TimeSource(std::move(timeSource)), m_Filter(std::move(filter))
	{
		if (!m_TimeSource)
		{
			throw std::invalid_argument("ReplayRecorder requires a time source");
		}

		// Control events: this is what makes Start/Stop/Play real EventManager events.
		m_Subscriptions.emplace_back(m_Events, EventType::ReplayStart, [this](const Event&) { OnStart(); });
		m_Subscriptions.emplace_back(m_Events, EventType::ReplayStop, [this](const Event&) { OnStop(); });
		m_Subscriptions.emplace_back(m_Events, EventType::ReplayPlay, [this](const Event&) { OnPlay(); });

		// The gameplay events a recording may contain. Control events are deliberately NOT in
		// this list, so they can never be recorded.
		for (EventType type : { EventType::Input, EventType::Collision, EventType::Death, EventType::Spawn })
		{
			m_Subscriptions.emplace_back(m_Events, type, [this](const Event& event) { OnGameplayEvent(event); });
		}
	}

	bool ReplayRecorder::IsRecording() const
	{
		return m_State == State::Recording;
	}

	bool ReplayRecorder::IsPlaying() const
	{
		return m_State == State::Playing;
	}

	bool ReplayRecorder::HasRecording() const
	{
		return m_HasCompletedRecording && (!m_Recorded.empty() || m_RecordedDuration > 0.0);
	}

	const std::vector<RecordedEvent>& ReplayRecorder::RecordedEvents() const
	{
		return m_Recorded;
	}

	double ReplayRecorder::RecordingStartTime() const
	{
		return m_RecordingStart;
	}

	double ReplayRecorder::RecordedDuration() const
	{
		return m_RecordedDuration;
	}

	double ReplayRecorder::PlaybackStartTime() const
	{
		return m_PlaybackStart;
	}

	void ReplayRecorder::OnStart()
	{
		if (m_State == State::Playing)
		{
			return;
		}

		// Starting always begins from scratch, including a restart mid-recording.
		m_Recorded.clear();
		m_RecordedDuration = 0.0;
		m_HasCompletedRecording = false;
		m_RecordingStart = m_TimeSource();
		m_State = State::Recording;
	}

	void ReplayRecorder::OnStop()
	{
		if (m_State == State::Recording)
		{
			StopRecording();
		}
	}

	void ReplayRecorder::StopRecording()
	{
		m_RecordedDuration = std::max(0.0, m_TimeSource() - m_RecordingStart);
		m_HasCompletedRecording = true;
		m_State = State::Idle;
	}

	void ReplayRecorder::OnPlay()
	{
		if (m_State == State::Playing)
		{
			return;
		}
		if (m_State == State::Recording)
		{
			StopRecording();
		}
		if (!HasRecording())
		{
			return;
		}

		m_PlaybackStart = m_TimeSource();
		m_State = State::Playing;

		for (const RecordedEvent& recorded : m_Recorded)
		{
			// Collision is derived state; its recorded consequences (Death, Spawn) replay
			// instead. See the class comment.
			if (TypeOf(recorded.Payload) == EventType::Collision)
			{
				continue;
			}
			// Every event replays at the same priority on purpose: with equal timestamps the
			// queue then keeps RECORDED DISPATCH ORDER (FIFO). Replaying each event's original
			// priority would reorder events recorded at the same instant — e.g. a causal
			// Death -> Spawn chain, where Spawn's higher priority would run it first.
			m_Events.Schedule(recorded.Payload, recorded.RelativeTime, PlaybackPriority);
		}
	}

	void ReplayRecorder::OnGameplayEvent(const Event& event)
	{
		// Nothing is recorded outside a recording — in particular not during playback, which
		// is what stops replayed events from recording themselves.
		if (m_State != State::Recording)
		{
			return;
		}
		if (m_Filter && !m_Filter(event))
		{
			return;
		}

		// Relative to the recording's start, never negative: an event stamped just before
		// the recording began but dispatched after it starts at 0.
		double relativeTime = std::max(0.0, event.Timestamp - m_RecordingStart);
		m_Recorded.push_back(RecordedEvent{ relativeTime, event.Priority, event.Payload });
	}

	bool ReplayRecorder::PollPlaybackComplete()
	{
		if (m_State != State::Playing)
		{
			return false;
		}

		// Same arithmetic as the scheduling in OnPlay (origin + relative time), so once this
		// holds every scheduled event's timestamp is <= now and has been dispatched.
		if (m_TimeSource() < m_PlaybackStart + m_RecordedDuration)
		{
			return false;
		}

		m_State = State::Idle;
		return true;
	}

	void ReplayRecorder::EndPlayback()
	{
		if (m_State == State::Playing)
		{
			m_State = State::Idle;
		}
	}
}
