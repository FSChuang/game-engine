#include "Engine/Time/Timeline.h"

#include <SDL3/SDL.h>

#include <stdexcept>

namespace Engine
{
	namespace
	{
		// The default real-time anchor: a monotonic clock, in seconds, independent of wall-clock/
		// calendar time (ENGINEERING_SPEC.md §8: use SDL for anything the platform provides).
		double RealTimeSeconds()
		{
			return static_cast<double>(SDL_GetTicks()) / 1000.0;
		}
	}

	Timeline::Timeline()
		: Timeline(AnchorSource(&RealTimeSeconds))
	{
	}

	Timeline::Timeline(AnchorSource anchorSource)
		: m_AnchorSource(std::move(anchorSource)), m_LastAnchorSample(m_AnchorSource())
	{
	}

	Timeline::Timeline(Timeline& parent)
		: Timeline(AnchorSource([&parent]() { return parent.GetTime(); }))
	{
	}

	double Timeline::GetTime() const
	{
		return m_CurrentTime;
	}

	void Timeline::AdvanceToNow()
	{
		double anchorTime = m_AnchorSource();

		if (m_IsPaused)
		{
			// Re-anchor without accumulating, so anchor time that passes while paused is
			// discarded rather than deferred into a catch-up jump on Unpause().
			m_LastAnchorSample = anchorTime;
			return;
		}

		double anchorDelta = anchorTime - m_LastAnchorSample;
		if (anchorDelta < 0.0)
		{
			// Defensive monotonicity guard (semantic rule: logical time never goes backward).
			anchorDelta = 0.0;
		}
		m_LastAnchorSample = anchorTime;

		// Uses whatever scale/tic size are in effect right now, so a caller that changes them
		// between two GetDeltaTime() calls only affects anchor time sampled after the change:
		// SetScale()/SetTicSize() call this first, under the OLD value, before adopting the new
		// one.
		double logicalDelta = anchorDelta * m_Scale / m_TicSize;
		m_CurrentTime += logicalDelta;
		m_PendingDelta += logicalDelta;
	}

	double Timeline::GetDeltaTime()
	{
		AdvanceToNow();

		double delta = m_PendingDelta;
		m_PendingDelta = 0.0;
		return delta;
	}

	void Timeline::Pause()
	{
		if (m_IsPaused)
		{
			return;
		}

		// Flush time elapsed since the last sample, at the current rate, before freezing, so the
		// paused logical time reflects the exact moment Pause() was called. Uses AdvanceToNow()
		// directly (not GetDeltaTime()) so any flushed-but-undelivered delta stays queued in
		// m_PendingDelta for the caller's next GetDeltaTime() call instead of being discarded here.
		AdvanceToNow();
		m_IsPaused = true;
	}

	void Timeline::Unpause()
	{
		if (!m_IsPaused)
		{
			return;
		}

		m_IsPaused = false;
		m_LastAnchorSample = m_AnchorSource();
	}

	bool Timeline::IsPaused() const
	{
		return m_IsPaused;
	}

	void Timeline::SetScale(double scale)
	{
		if (scale <= 0.0)
		{
			throw std::invalid_argument("Timeline scale must be greater than zero");
		}

		AdvanceToNow();
		m_Scale = scale;
	}

	double Timeline::GetScale() const
	{
		return m_Scale;
	}

	void Timeline::SetTicSize(double ticSize)
	{
		if (ticSize <= 0.0)
		{
			throw std::invalid_argument("Timeline tic size must be greater than zero");
		}

		AdvanceToNow();
		m_TicSize = ticSize;
	}

	double Timeline::GetTicSize() const
	{
		return m_TicSize;
	}
}
