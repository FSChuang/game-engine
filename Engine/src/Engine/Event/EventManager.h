#pragma once

#include "Engine/Event/Event.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <queue>
#include <stdexcept>
#include <vector>

namespace Engine
{
	/// Identifies one registered handler, for Unsubscribe(). Never reused within one
	/// EventManager; 0 is never assigned and means "no handler".
	using HandlerId = std::uint64_t;

	/// What a registered handler looks like: any callable taking the dispatched event.
	using EventHandler = std::function<void(const Event&)>;

	/// Thrown by EventManager::DispatchDue() when one call dispatched its maximum number of
	/// events and more due events were still waiting — almost certainly an event cycle (a
	/// handler that keeps raising the event that triggers it). Nothing is dropped: the
	/// events still due remain queued.
	class EventDispatchLimitExceeded : public std::runtime_error
	{
	public:
		EventDispatchLimitExceeded();
	};

	/// The Event Management System (Milestone 4): queues events, orders them by Timeline
	/// time, and dispatches them to registered handlers. It answers the four design
	/// questions as follows.
	///
	/// 1. Representation: Event envelope + EventPayload variant (Event.h).
	/// 2. Registration: Subscribe(EventType, callback) -> HandlerId; Unsubscribe(HandlerId),
	///    or a ScopedSubscription that unsubscribes itself. Plain std::function callbacks —
	///    no listener class hierarchy, no raw listener pointers.
	/// 3. Raising: Raise() (due now) and Schedule() (due after a delay). Both stamp the event
	///    from the injected time source and enter the SAME queue; neither runs a handler.
	/// 4. Handling: DispatchDue() pops every due event in order and calls that type's
	///    handlers. Nothing is ever dispatched from inside Raise().
	///
	/// Time: the manager does not know about Timeline (which lives in the SDL-backed Engine
	/// library). It is given a `std::function<double()>` returning the current logical time —
	/// e.g. `[&timeline] { return timeline.GetTime(); }` — mirroring how Timeline itself takes
	/// an injectable anchor. The source must be non-decreasing. Because the source is LOGICAL
	/// time: a paused Timeline stops future-scheduled events from becoming due, while an event
	/// raised during the pause is stamped with the (frozen) current time and is therefore due
	/// immediately. That is intentional; the manager has no pause logic of its own.
	///
	/// Ordering: events dispatch by (1) timestamp ascending, then (2) priority descending, then
	/// (3) raise order. An older low-priority event therefore runs before a newer
	/// high-priority one; priority only breaks ties between events with equal timestamps.
	///
	/// Single-threaded by design: raise and dispatch from one thread (a network thread must
	/// hand events to the owning thread, which raises them). Not copyable or movable.
	class EventManager
	{
	public:
		/// Default cap on events dispatched by a single DispatchDue() call.
		static constexpr std::size_t DefaultMaxEventsPerDispatch = 10000;

		/// Throws std::invalid_argument if `timeSource` is empty or the limit is zero.
		explicit EventManager(std::function<double()> timeSource,
		                      std::size_t maxEventsPerDispatch = DefaultMaxEventsPerDispatch);

		EventManager(const EventManager&) = delete;
		EventManager& operator=(const EventManager&) = delete;
		EventManager(EventManager&&) = delete;
		EventManager& operator=(EventManager&&) = delete;

		/// Registers `handler` for every event of `type`. Handlers for one type run in
		/// registration order. A handler registered while a dispatch is running first sees
		/// events dispatched AFTER the one currently in flight (it never sees the event that
		/// was already being delivered when it registered).
		HandlerId Subscribe(EventType type, EventHandler handler);

		/// Removes a handler; returns false if `id` is unknown or already removed. Safe from
		/// inside a handler (including the handler removing itself): the handler is skipped
		/// from that moment on, even for the event currently being delivered, and its storage
		/// is released once the dispatch finishes.
		bool Unsubscribe(HandlerId id);

		/// Queues an event due at the current logical time. Does not call any handler.
		void Raise(EventPayload payload, EventPriority priority = EventPriority::Normal);

		/// Queues an event due `delay` logical-time units from now. Throws
		/// std::invalid_argument if `delay` is negative or NaN.
		void Schedule(EventPayload payload, double delay, EventPriority priority = EventPriority::Normal);

		/// Dispatches every queued event whose timestamp is at or before the logical time read
		/// when this call starts, in the documented order, and returns how many were
		/// dispatched. Handlers may Raise()/Schedule(); an event raised during dispatch that is
		/// already due is dispatched by this same call. Throws EventDispatchLimitExceeded if
		/// the per-call cap is reached with due events still waiting, and std::logic_error if
		/// called from inside a handler.
		std::size_t DispatchDue();

		/// Events queued and not yet dispatched (due or not).
		std::size_t PendingCount() const;

	private:
		struct HandlerEntry
		{
			HandlerId Id;
			EventHandler Handler;
			bool Active;
		};

		// std::priority_queue's "less" puts the LAST event to run at the bottom, so this
		// returns true when `a` should run AFTER `b`.
		struct RunsAfter
		{
			bool operator()(const Event& a, const Event& b) const;
		};

		void Enqueue(EventPayload payload, double timestamp, EventPriority priority);
		void DeliverToHandlers(const Event& event);
		void RemoveInactiveHandlers();

		std::function<double()> m_TimeSource;
		std::size_t m_MaxEventsPerDispatch;

		std::priority_queue<Event, std::vector<Event>, RunsAfter> m_Queue;
		std::uint64_t m_NextSequence = 0;

		// std::list so that registering a handler mid-dispatch never moves a handler that is
		// currently executing.
		std::array<std::list<HandlerEntry>, EventTypeCount> m_Handlers;
		HandlerId m_NextHandlerId = 1;
		bool m_IsDispatching = false;
		bool m_HasInactiveHandlers = false;
	};

	/// Move-only RAII registration: unsubscribes when destroyed (or Reset()). The
	/// EventManager must outlive it. A default-constructed one is empty and harmless.
	class ScopedSubscription
	{
	public:
		ScopedSubscription() = default;
		/// Takes ownership of an existing registration made on `manager`.
		ScopedSubscription(EventManager& manager, HandlerId id);
		/// Subscribes and owns the result in one step.
		ScopedSubscription(EventManager& manager, EventType type, EventHandler handler);
		~ScopedSubscription();

		ScopedSubscription(const ScopedSubscription&) = delete;
		ScopedSubscription& operator=(const ScopedSubscription&) = delete;
		ScopedSubscription(ScopedSubscription&& other) noexcept;
		ScopedSubscription& operator=(ScopedSubscription&& other) noexcept;

		/// Unsubscribes now (if still holding a registration) and becomes empty.
		void Reset();

		/// True while this object holds a registration.
		bool IsActive() const;

	private:
		EventManager* m_Manager = nullptr;
		HandlerId m_Id = 0;
	};
}
