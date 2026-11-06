#include "Engine/Event/EventManager.h"

#include <cmath>
#include <utility>

namespace Engine
{
	EventDispatchLimitExceeded::EventDispatchLimitExceeded()
		: std::runtime_error("EventManager::DispatchDue exceeded its per-call event limit "
		                     "(probable event cycle); remaining due events were left queued")
	{
	}

	bool EventManager::RunsAfter::operator()(const Event& a, const Event& b) const
	{
		if (a.Timestamp != b.Timestamp)
		{
			return a.Timestamp > b.Timestamp;
		}
		if (a.Priority != b.Priority)
		{
			return a.Priority < b.Priority;
		}
		return a.Sequence > b.Sequence;
	}

	EventManager::EventManager(std::function<double()> timeSource, std::size_t maxEventsPerDispatch)
		: m_TimeSource(std::move(timeSource)),
		  m_MaxEventsPerDispatch(maxEventsPerDispatch)
	{
		if (!m_TimeSource)
		{
			throw std::invalid_argument("EventManager requires a time source");
		}
		if (m_MaxEventsPerDispatch == 0)
		{
			throw std::invalid_argument("EventManager's per-dispatch event limit must be greater than zero");
		}
	}

	HandlerId EventManager::Subscribe(EventType type, EventHandler handler)
	{
		HandlerId id = m_NextHandlerId++;
		m_Handlers[static_cast<std::size_t>(type)].push_back(HandlerEntry{ id, std::move(handler), true });
		return id;
	}

	bool EventManager::Unsubscribe(HandlerId id)
	{
		for (std::list<HandlerEntry>& handlers : m_Handlers)
		{
			for (auto it = handlers.begin(); it != handlers.end(); ++it)
			{
				if (it->Id != id || !it->Active)
				{
					continue;
				}

				if (m_IsDispatching)
				{
					// The running handler (possibly this very one) must not be destroyed
					// under its own feet: just disable it and sweep after the dispatch.
					it->Active = false;
					m_HasInactiveHandlers = true;
				}
				else
				{
					handlers.erase(it);
				}
				return true;
			}
		}
		return false;
	}

	void EventManager::Raise(EventPayload payload, EventPriority priority)
	{
		Enqueue(std::move(payload), m_TimeSource(), priority);
	}

	void EventManager::Schedule(EventPayload payload, double delay, EventPriority priority)
	{
		if (!(delay >= 0.0))
		{
			throw std::invalid_argument("EventManager::Schedule delay must be non-negative");
		}
		Enqueue(std::move(payload), m_TimeSource() + delay, priority);
	}

	void EventManager::Enqueue(EventPayload payload, double timestamp, EventPriority priority)
	{
		EventType type = TypeOf(payload);
		m_Queue.push(Event{ type, timestamp, priority, m_NextSequence++, std::move(payload) });
	}

	std::size_t EventManager::DispatchDue()
	{
		if (m_IsDispatching)
		{
			throw std::logic_error("EventManager::DispatchDue must not be called from inside a handler");
		}

		// Clears the dispatching flag and sweeps removed handlers on every exit path,
		// including a handler that throws or the dispatch limit being hit.
		struct DispatchScope
		{
			explicit DispatchScope(EventManager& manager) : Manager(manager) { Manager.m_IsDispatching = true; }
			~DispatchScope()
			{
				Manager.m_IsDispatching = false;
				Manager.RemoveInactiveHandlers();
			}
			EventManager& Manager;
		} scope(*this);

		const double now = m_TimeSource();
		std::size_t dispatched = 0;

		while (!m_Queue.empty() && m_Queue.top().Timestamp <= now)
		{
			if (dispatched == m_MaxEventsPerDispatch)
			{
				throw EventDispatchLimitExceeded();
			}

			// Copied out and popped BEFORE delivery, so handlers are free to Raise() (which
			// pushes onto the queue) while this event is in flight.
			Event event = m_Queue.top();
			m_Queue.pop();
			DeliverToHandlers(event);
			++dispatched;
		}

		return dispatched;
	}

	void EventManager::DeliverToHandlers(const Event& event)
	{
		// Handler ids only ever increase, so anything registered during this delivery has
		// an id at or beyond this mark and is skipped until the next event.
		const HandlerId firstIdNotYetRegistered = m_NextHandlerId;

		std::list<HandlerEntry>& handlers = m_Handlers[static_cast<std::size_t>(event.Type)];
		for (HandlerEntry& entry : handlers)
		{
			if (entry.Id >= firstIdNotYetRegistered)
			{
				break;
			}
			if (entry.Active)
			{
				entry.Handler(event);
			}
		}
	}

	void EventManager::RemoveInactiveHandlers()
	{
		if (!m_HasInactiveHandlers)
		{
			return;
		}

		for (std::list<HandlerEntry>& handlers : m_Handlers)
		{
			handlers.remove_if([](const HandlerEntry& entry) { return !entry.Active; });
		}
		m_HasInactiveHandlers = false;
	}

	std::size_t EventManager::PendingCount() const
	{
		return m_Queue.size();
	}

	ScopedSubscription::ScopedSubscription(EventManager& manager, HandlerId id)
		: m_Manager(&manager), m_Id(id)
	{
	}

	ScopedSubscription::ScopedSubscription(EventManager& manager, EventType type, EventHandler handler)
		: m_Manager(&manager), m_Id(manager.Subscribe(type, std::move(handler)))
	{
	}

	ScopedSubscription::~ScopedSubscription()
	{
		Reset();
	}

	ScopedSubscription::ScopedSubscription(ScopedSubscription&& other) noexcept
		: m_Manager(std::exchange(other.m_Manager, nullptr)), m_Id(std::exchange(other.m_Id, 0))
	{
	}

	ScopedSubscription& ScopedSubscription::operator=(ScopedSubscription&& other) noexcept
	{
		if (this != &other)
		{
			Reset();
			m_Manager = std::exchange(other.m_Manager, nullptr);
			m_Id = std::exchange(other.m_Id, 0);
		}
		return *this;
	}

	void ScopedSubscription::Reset()
	{
		if (m_Manager != nullptr)
		{
			m_Manager->Unsubscribe(m_Id);
			m_Manager = nullptr;
			m_Id = 0;
		}
	}

	bool ScopedSubscription::IsActive() const
	{
		return m_Manager != nullptr;
	}
}
