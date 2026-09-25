#pragma once

#include <functional>

namespace Engine
{
	// Timeline represents local, logical time derived from an anchor: either a monotonic
	// real-time source, or another Timeline. It never reads wall-clock/calendar time.
	//
	// Anchor: the source of raw, ever-increasing time this Timeline measures itself against.
	// A root Timeline anchors to a monotonic time source (real time by default, or an injected
	// callable for testing). A child Timeline anchors to a parent Timeline's logical time, so
	// scaling or pausing the parent propagates to the child without the child knowing about it.
	// Call the parent's GetDeltaTime() before the child's each update cycle so the child observes
	// the parent's latest logical time.
	//
	// Logical time: GetTime() returns this Timeline's own accumulated time, in local time units.
	// It only changes when GetDeltaTime(), SetScale(), SetTicSize(), or Pause() samples the
	// anchor; GetTime() itself is a pure getter and never advances the clock.
	//
	// Scale: how fast logical time advances relative to the anchor. 1.0 tracks the anchor
	// one-to-one, 0.5 advances at half the anchor's rate, 2.0 at double. Anchor time already
	// elapsed before a scale change keeps being credited at the OLD scale; only anchor time
	// sampled after the change uses the new one. The current logical time never jumps when scale
	// is set, and no elapsed time is ever dropped: SetScale() itself samples the anchor and
	// credits everything elapsed so far (at the old scale) before adopting the new one, so a
	// change that lands between two GetDeltaTime() calls cannot retroactively reinterpret anchor
	// time the caller hasn't been given a delta for yet.
	//
	// Tic size: the anchor-time duration that one local logical-time unit represents (NOT a
	// speed multiplier like scale). A tic size of 2.0 means 2 anchor-time units must elapse for
	// logical time to advance by 1 unit (half rate); 0.5 means logical time advances twice as
	// fast per anchor unit. It composes with scale as: logicalDelta = anchorDelta * scale /
	// ticSize. Exactly like scale, a tic size change only affects anchor time sampled after it.
	//
	// Pause: an explicit state, not scale 0. While paused, GetDeltaTime() reports zero elapsed
	// time and GetTime() does not advance. Time that passes on the anchor while paused is
	// discarded, not deferred, so Unpause() never produces a catch-up jump.
	class Timeline
	{
	public:
		// Supplies the current anchor time (seconds for a real-time anchor, or a parent
		// Timeline's local time units for a child); must be monotonically non-decreasing.
		using AnchorSource = std::function<double()>;

		// Anchors to real time, via a monotonic engine time source (not wall-clock/calendar time).
		Timeline();

		// Anchors to an arbitrary monotonic time source. This is the seam that makes Timeline
		// logic deterministically testable without depending on real elapsed time.
		explicit Timeline(AnchorSource anchorSource);

		// Anchors to another Timeline's logical time. `parent` must outlive this Timeline.
		explicit Timeline(Timeline& parent);

		// This Timeline's own accumulated logical time, in local time units. A pure getter: it
		// never samples the anchor or changes state.
		double GetTime() const;

		// Delivers all logical time elapsed since the last call to GetDeltaTime(): this includes
		// any interval already flushed by an intervening SetScale()/SetTicSize()/Pause() call (at
		// whichever rate was in effect while each interval elapsed), so no elapsed time is ever
		// silently lost. Zero while paused. Call once per update per Timeline.
		double GetDeltaTime();

		void Pause();
		void Unpause();
		bool IsPaused() const;

		// Throws std::invalid_argument if scale <= 0; the previous value is retained and no
		// state changes. Otherwise, first credits anchor time elapsed so far at the current
		// (old) scale, then applies the new scale to anchor time sampled after this call.
		void SetScale(double scale);
		double GetScale() const;

		// Throws std::invalid_argument if ticSize <= 0; the previous value is retained and no
		// state changes. Otherwise, first credits anchor time elapsed so far at the current
		// (old) tic size, then applies the new tic size to anchor time sampled after this call.
		void SetTicSize(double ticSize);
		double GetTicSize() const;

	private:
		// Samples the anchor, credits any newly-elapsed anchor time to m_CurrentTime and
		// m_PendingDelta at the CURRENT scale/tic size (or discards it if paused), and re-anchors
		// to the sample just taken. Shared by GetDeltaTime(), SetScale(), SetTicSize(), and
		// Pause() so a rate change or pause always accounts for elapsed-but-unsampled anchor time
		// under the rate that was actually in effect while it elapsed.
		void AdvanceToNow();

		AnchorSource m_AnchorSource;
		double m_LastAnchorSample;
		double m_CurrentTime = 0.0;
		// Logical time credited to m_CurrentTime but not yet handed to the caller via
		// GetDeltaTime(). Needed because AdvanceToNow() runs from more places than GetDeltaTime()
		// (SetScale/SetTicSize/Pause also flush), so a delta can be produced between two
		// GetDeltaTime() calls; without this, that delta would be silently dropped instead of
		// delivered on the next GetDeltaTime() call.
		double m_PendingDelta = 0.0;
		double m_Scale = 1.0;
		double m_TicSize = 1.0;
		bool m_IsPaused = false;
	};
}
