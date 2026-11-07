// Focused unit tests for Engine::EventManager (Milestone 4, Checkpoint 1). Pure logic: the
// time source is a plain variable the test advances by hand, so ordering and scheduling are
// fully deterministic. This target links EngineCore only — no SDL, no Timeline — which is
// itself the proof that the event system is usable headless.

#include "Engine/Event/EventManager.h"

#include <cstdio>
#include <stdexcept>
#include <utility>
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
	// Spawn events are used as numbered markers: the Subject field says WHICH event ran.
	EventPayload Marker(ObjectId id)
	{
		return SpawnEventData{ id, { 0.0f, 0.0f } };
	}

	ObjectId MarkerOf(const Event& event)
	{
		return std::get<SpawnEventData>(event.Payload).Subject;
	}

	// Subscribes a handler that appends each Spawn marker it sees to `order`.
	HandlerId RecordMarkers(EventManager& manager, std::vector<ObjectId>& order)
	{
		return manager.Subscribe(EventType::Spawn, [&order](const Event& event) { order.push_back(MarkerOf(event)); });
	}

	bool Equals(const std::vector<ObjectId>& actual, const std::vector<ObjectId>& expected)
	{
		return actual == expected;
	}
}

int main()
{
	// 1. Subscribe + Raise + DispatchDue delivers the event.
	{
		double now = 0.0;
		EventManager manager([&] { return now; });
		int calls = 0;
		manager.Subscribe(EventType::Input, [&](const Event&) { ++calls; });
		manager.Raise(InputEventData{ InputAction::ActionPrimary, true });
		std::size_t dispatched = manager.DispatchDue();
		Check(calls == 1 && dispatched == 1, "SubscribeRaiseDispatch_DeliversEvent");
	}

	// 2. A handler for a different type is not triggered.
	{
		double now = 0.0;
		EventManager manager([&] { return now; });
		int deathCalls = 0;
		manager.Subscribe(EventType::Death, [&](const Event&) { ++deathCalls; });
		manager.Raise(InputEventData{ InputAction::ActionPrimary, true });
		manager.DispatchDue();
		Check(deathCalls == 0, "UnrelatedEventType_DoesNotTriggerHandler");
	}

	// 3. Several subscribers for one type all run, in registration order.
	{
		double now = 0.0;
		EventManager manager([&] { return now; });
		std::vector<int> calls;
		manager.Subscribe(EventType::Death, [&](const Event&) { calls.push_back(1); });
		manager.Subscribe(EventType::Death, [&](const Event&) { calls.push_back(2); });
		manager.Raise(DeathEventData{ 1, NoObjectId });
		manager.DispatchDue();
		Check(calls == std::vector<int>({ 1, 2 }), "MultipleSubscribers_AllRunInRegistrationOrder");
	}

	// 4. Unsubscribe stops delivery; unknown / repeated ids report false.
	{
		double now = 0.0;
		EventManager manager([&] { return now; });
		int calls = 0;
		HandlerId id = manager.Subscribe(EventType::Death, [&](const Event&) { ++calls; });
		bool removed = manager.Unsubscribe(id);
		bool removedAgain = manager.Unsubscribe(id);
		manager.Raise(DeathEventData{ 1, NoObjectId });
		manager.DispatchDue();
		Check(removed && calls == 0, "Unsubscribe_StopsDelivery");
		Check(!removedAgain && !manager.Unsubscribe(9999), "Unsubscribe_UnknownOrRepeatedId_ReturnsFalse");
	}

	// 5. ScopedSubscription unsubscribes on destruction, Reset, and is safe when moved/empty.
	{
		double now = 0.0;
		EventManager manager([&] { return now; });
		int calls = 0;
		{
			ScopedSubscription scoped(manager, EventType::Death, [&](const Event&) { ++calls; });
			manager.Raise(DeathEventData{ 1, NoObjectId });
			manager.DispatchDue();
			Check(calls == 1 && scoped.IsActive(), "ScopedSubscription_ReceivesWhileAlive");
		}
		manager.Raise(DeathEventData{ 1, NoObjectId });
		manager.DispatchDue();
		Check(calls == 1, "ScopedSubscription_Destruction_Unsubscribes");

		ScopedSubscription first(manager, EventType::Death, [&](const Event&) { ++calls; });
		ScopedSubscription second(std::move(first));
		Check(!first.IsActive() && second.IsActive(), "ScopedSubscription_Move_TransfersOwnership");
		second.Reset();
		manager.Raise(DeathEventData{ 1, NoObjectId });
		manager.DispatchDue();
		Check(calls == 1 && !second.IsActive(), "ScopedSubscription_Reset_Unsubscribes");

		ScopedSubscription empty;
		empty.Reset();
		Check(!empty.IsActive(), "ScopedSubscription_DefaultConstructed_IsEmptyAndSafe");

		// Move-assignment releases what the target previously held.
		int heldCalls = 0;
		ScopedSubscription target(manager, EventType::Death, [&](const Event&) { ++heldCalls; });
		ScopedSubscription replacement(manager, EventType::Death, [&](const Event&) { ++calls; });
		target = std::move(replacement);
		manager.Raise(DeathEventData{ 1, NoObjectId });
		manager.DispatchDue();
		Check(heldCalls == 0 && calls == 2, "ScopedSubscription_MoveAssign_ReleasesPreviousRegistration");
	}

	// 6. Raise only queues; the handler runs inside DispatchDue, never inside Raise.
	{
		double now = 0.0;
		EventManager manager([&] { return now; });
		int calls = 0;
		manager.Subscribe(EventType::Input, [&](const Event&) { ++calls; });
		manager.Raise(InputEventData{ InputAction::MoveLeft, true });
		Check(calls == 0 && manager.PendingCount() == 1, "Raise_QueuesWithoutDispatching");
		manager.DispatchDue();
		Check(calls == 1 && manager.PendingCount() == 0, "DispatchDue_DrainsQueue");
	}

	// 7 & 8. Schedule is held back until its timestamp, then dispatches exactly when due.
	{
		double now = 10.0;
		EventManager manager([&] { return now; });
		std::vector<ObjectId> order;
		RecordMarkers(manager, order);
		manager.Schedule(Marker(1), 5.0);
		manager.DispatchDue();
		Check(order.empty() && manager.PendingCount() == 1, "Schedule_NotDispatchedBeforeTimestamp");
		now = 14.999;
		manager.DispatchDue();
		Check(order.empty(), "Schedule_NotDispatchedJustBeforeTimestamp");
		now = 15.0;
		manager.DispatchDue();
		Check(Equals(order, { 1 }) && manager.PendingCount() == 0, "Schedule_DispatchedExactlyWhenDue");
	}

	// 9. Earlier timestamps run first regardless of the order they were raised in.
	{
		double now = 0.0;
		EventManager manager([&] { return now; });
		std::vector<ObjectId> order;
		RecordMarkers(manager, order);
		manager.Schedule(Marker(3), 3.0);
		manager.Schedule(Marker(1), 1.0);
		manager.Schedule(Marker(2), 2.0);
		now = 10.0;
		manager.DispatchDue();
		Check(Equals(order, { 1, 2, 3 }), "Ordering_EarlierTimestampFirst");
	}

	// 10. Equal timestamp: higher priority first.
	{
		double now = 0.0;
		EventManager manager([&] { return now; });
		std::vector<ObjectId> order;
		RecordMarkers(manager, order);
		manager.Raise(Marker(1), EventPriority::Low);
		manager.Raise(Marker(2), EventPriority::Critical);
		manager.Raise(Marker(3), EventPriority::Normal);
		manager.Raise(Marker(4), EventPriority::High);
		manager.DispatchDue();
		Check(Equals(order, { 2, 4, 3, 1 }), "Ordering_EqualTimestamp_HigherPriorityFirst");
	}

	// 11. Equal timestamp and priority: FIFO.
	{
		double now = 0.0;
		EventManager manager([&] { return now; });
		std::vector<ObjectId> order;
		RecordMarkers(manager, order);
		for (ObjectId id = 1; id <= 5; ++id)
		{
			manager.Raise(Marker(id), EventPriority::Normal);
		}
		manager.DispatchDue();
		Check(Equals(order, { 1, 2, 3, 4, 5 }), "Ordering_EqualTimestampAndPriority_IsFifo");
	}

	// 12. Backlog: an older low-priority event runs before a newer high-priority one.
	{
		double now = 0.0;
		EventManager manager([&] { return now; });
		std::vector<ObjectId> order;
		RecordMarkers(manager, order);
		manager.Raise(Marker(1), EventPriority::Low);
		now = 1.0;
		manager.Raise(Marker(2), EventPriority::Critical);
		manager.DispatchDue();
		Check(Equals(order, { 1, 2 }), "Ordering_OlderLowPriority_BeforeNewerHighPriority");
	}

	// Raise and Schedule share one queue: a Schedule(0) and a Raise interleave by sequence.
	{
		double now = 0.0;
		EventManager manager([&] { return now; });
		std::vector<ObjectId> order;
		RecordMarkers(manager, order);
		manager.Raise(Marker(1));
		manager.Schedule(Marker(2), 0.0);
		manager.Raise(Marker(3));
		manager.DispatchDue();
		Check(Equals(order, { 1, 2, 3 }), "RaiseAndSchedule_ShareOneQueue");
	}

	// 13. A handler may Raise another event; if due it runs in the same DispatchDue call.
	{
		double now = 0.0;
		EventManager manager([&] { return now; });
		std::vector<ObjectId> order;
		manager.Subscribe(EventType::Death, [&](const Event&)
		{
			order.push_back(100);
			manager.Raise(Marker(7));
		});
		RecordMarkers(manager, order);
		manager.Raise(DeathEventData{ 1, NoObjectId });
		std::size_t dispatched = manager.DispatchDue();
		Check(Equals(order, { 100, 7 }) && dispatched == 2, "Reentrancy_HandlerRaisedDueEvent_RunsInSameDispatch");
	}

	// A handler-scheduled FUTURE event is not dispatched by that same call.
	{
		double now = 0.0;
		EventManager manager([&] { return now; });
		std::vector<ObjectId> order;
		manager.Subscribe(EventType::Death, [&](const Event&) { manager.Schedule(Marker(9), 1.0); });
		RecordMarkers(manager, order);
		manager.Raise(DeathEventData{ 1, NoObjectId });
		manager.DispatchDue();
		Check(order.empty() && manager.PendingCount() == 1, "Reentrancy_HandlerScheduledFutureEvent_StaysPending");
	}

	// 14. Unsubscribe during dispatch is safe: self-removal, and removing a later handler.
	{
		double now = 0.0;
		EventManager manager([&] { return now; });
		int selfRemovingCalls = 0;
		int victimCalls = 0;
		HandlerId selfId = 0;
		HandlerId victimId = 0;
		selfId = manager.Subscribe(EventType::Death, [&](const Event&)
		{
			++selfRemovingCalls;
			manager.Unsubscribe(selfId);
			manager.Unsubscribe(victimId);
		});
		victimId = manager.Subscribe(EventType::Death, [&](const Event&) { ++victimCalls; });
		manager.Raise(DeathEventData{ 1, NoObjectId });
		manager.Raise(DeathEventData{ 2, NoObjectId });
		manager.DispatchDue();
		Check(selfRemovingCalls == 1, "UnsubscribeDuringDispatch_SelfRemoval_RunsOnlyOnce");
		Check(victimCalls == 0, "UnsubscribeDuringDispatch_RemovedLaterHandler_IsSkipped");
	}

	// 15. Subscribe during dispatch: the new handler misses the in-flight event but sees later ones.
	{
		double now = 0.0;
		EventManager manager([&] { return now; });
		std::vector<ObjectId> lateSaw;
		bool subscribed = false;
		manager.Subscribe(EventType::Spawn, [&](const Event&)
		{
			if (!subscribed)
			{
				subscribed = true;
				manager.Subscribe(EventType::Spawn, [&](const Event& event) { lateSaw.push_back(MarkerOf(event)); });
			}
		});
		manager.Raise(Marker(1));
		manager.Raise(Marker(2));
		manager.DispatchDue();
		Check(Equals(lateSaw, { 2 }), "SubscribeDuringDispatch_NewHandlerSeesOnlyLaterEvents");
	}

	// 16. A future event stays pending while time does not advance.
	{
		double now = 5.0;
		EventManager manager([&] { return now; });
		std::vector<ObjectId> order;
		RecordMarkers(manager, order);
		manager.Schedule(Marker(1), 2.0);
		manager.DispatchDue();
		manager.DispatchDue();
		manager.DispatchDue();
		Check(order.empty() && manager.PendingCount() == 1, "FrozenTime_FutureEventRemainsPending");
	}

	// 17. An event raised at frozen time is due immediately (the accepted paused-Timeline behavior).
	{
		double now = 5.0;
		EventManager manager([&] { return now; });
		std::vector<ObjectId> order;
		RecordMarkers(manager, order);
		manager.Schedule(Marker(1), 2.0);
		manager.Raise(Marker(2));
		manager.DispatchDue();
		Check(Equals(order, { 2 }) && manager.PendingCount() == 1, "FrozenTime_RaisedEventIsImmediatelyDue");
	}

	// 18. The dispatch guard stops a self-raising cycle, explicitly, without dropping events.
	{
		double now = 0.0;
		const std::size_t limit = 50;
		EventManager manager([&] { return now; }, limit);
		std::size_t handled = 0;
		manager.Subscribe(EventType::Collision, [&](const Event&)
		{
			++handled;
			manager.Raise(CollisionEventData{ 1, 2 });
		});
		manager.Raise(CollisionEventData{ 1, 2 });

		bool threw = false;
		try
		{
			manager.DispatchDue();
		}
		catch (const EventDispatchLimitExceeded&)
		{
			threw = true;
		}
		Check(threw, "DispatchGuard_SelfRaisingLoop_ThrowsExplicitly");
		Check(handled == limit, "DispatchGuard_DispatchesExactlyTheLimitBeforeStopping");
		Check(manager.PendingCount() == 1, "DispatchGuard_DoesNotDropTheWaitingEvent");

		// The manager is still usable afterwards (dispatch state was reset).
		bool secondThrew = false;
		try
		{
			manager.DispatchDue();
		}
		catch (const EventDispatchLimitExceeded&)
		{
			secondThrew = true;
		}
		Check(secondThrew && handled == limit * 2, "DispatchGuard_ManagerRemainsUsableAfterTrip");
	}

	// The guard must not trip when the queue empties exactly at the limit.
	{
		double now = 0.0;
		EventManager manager([&] { return now; }, 3);
		int calls = 0;
		manager.Subscribe(EventType::Input, [&](const Event&) { ++calls; });
		for (int i = 0; i < 3; ++i)
		{
			manager.Raise(InputEventData{ InputAction::ActionPrimary, true });
		}
		bool threw = false;
		try
		{
			manager.DispatchDue();
		}
		catch (const EventDispatchLimitExceeded&)
		{
			threw = true;
		}
		Check(!threw && calls == 3, "DispatchGuard_ExactlyAtLimit_DoesNotThrow");
	}

	// 19. Headless: this whole executable links EngineCore only (see file header). Also check
	// that no handler or time source is needed beyond plain callables.
	{
		EventManager manager([] { return 0.0; });
		manager.Raise(InputEventData{ InputAction::MoveRight, false });
		Check(manager.DispatchDue() == 1, "Headless_EventManagerWorksWithoutSdl");
	}

	// 20. Payload correctness for every required event type, and Event envelope fields.
	{
		double now = 2.5;
		EventManager manager([&] { return now; });
		std::vector<Event> seen;
		for (EventType type : { EventType::Collision, EventType::Death, EventType::Spawn, EventType::Input })
		{
			manager.Subscribe(type, [&](const Event& event) { seen.push_back(event); });
		}

		manager.Raise(CollisionEventData{ 11, 22 }, EventPriority::High);
		manager.Raise(DeathEventData{ 33, 44 });
		manager.Raise(SpawnEventData{ 55, { 6.0f, 7.0f } });
		manager.Raise(InputEventData{ InputAction::ActionPrimary, true }, EventPriority::Low);
		manager.DispatchDue();

		// Priority orders these: Collision(High) first, then Death/Spawn (Normal, FIFO), then Input (Low).
		bool shapeOk = seen.size() == 4;
		Check(shapeOk, "Payloads_AllFourEventTypesDelivered");
		if (shapeOk)
		{
			const auto& collision = std::get<CollisionEventData>(seen[0].Payload);
			Check(seen[0].Type == EventType::Collision && collision.A == 11 && collision.B == 22 &&
			          seen[0].Priority == EventPriority::High && seen[0].Timestamp == 2.5,
			      "Payloads_Collision_FieldsAndEnvelope");

			const auto& death = std::get<DeathEventData>(seen[1].Payload);
			Check(seen[1].Type == EventType::Death && death.Subject == 33 && death.Cause == 44,
			      "Payloads_Death_Fields");

			const auto& spawn = std::get<SpawnEventData>(seen[2].Payload);
			Check(seen[2].Type == EventType::Spawn && spawn.Subject == 55 && spawn.Position.X == 6.0f &&
			          spawn.Position.Y == 7.0f,
			      "Payloads_Spawn_Fields");

			const auto& input = std::get<InputEventData>(seen[3].Payload);
			Check(seen[3].Type == EventType::Input && input.Action == InputAction::ActionPrimary && input.Pressed,
			      "Payloads_Input_Fields");

			Check(seen[1].Sequence < seen[2].Sequence, "Envelope_SequenceIncreasesInRaiseOrder");
		}
	}

	// TypeOf maps each payload alternative to its EventType.
	{
		Check(TypeOf(EventPayload{ CollisionEventData{ 1, 2 } }) == EventType::Collision &&
		          TypeOf(EventPayload{ DeathEventData{ 1, 2 } }) == EventType::Death &&
		          TypeOf(EventPayload{ SpawnEventData{ 1, { 0.0f, 0.0f } } }) == EventType::Spawn &&
		          TypeOf(EventPayload{ InputEventData{ InputAction::ActionPrimary, true } }) == EventType::Input,
		      "TypeOf_MatchesPayloadAlternative");
	}

	// Argument validation.
	{
		bool emptyClockThrew = false;
		try
		{
			EventManager manager{ std::function<double()>() };
		}
		catch (const std::invalid_argument&)
		{
			emptyClockThrew = true;
		}
		Check(emptyClockThrew, "Constructor_EmptyTimeSource_Throws");

		bool zeroLimitThrew = false;
		try
		{
			EventManager manager([] { return 0.0; }, 0);
		}
		catch (const std::invalid_argument&)
		{
			zeroLimitThrew = true;
		}
		Check(zeroLimitThrew, "Constructor_ZeroDispatchLimit_Throws");

		EventManager manager([] { return 0.0; });
		bool negativeDelayThrew = false;
		try
		{
			manager.Schedule(Marker(1), -1.0);
		}
		catch (const std::invalid_argument&)
		{
			negativeDelayThrew = true;
		}
		Check(negativeDelayThrew && manager.PendingCount() == 0, "Schedule_NegativeDelay_ThrowsAndQueuesNothing");
	}

	// Calling DispatchDue from inside a handler is rejected, and the outer dispatch recovers.
	{
		double now = 0.0;
		EventManager manager([&] { return now; });
		bool nestedThrew = false;
		manager.Subscribe(EventType::Death, [&](const Event&)
		{
			try
			{
				manager.DispatchDue();
			}
			catch (const std::logic_error&)
			{
				nestedThrew = true;
			}
		});
		manager.Raise(DeathEventData{ 1, NoObjectId });
		manager.DispatchDue();
		Check(nestedThrew, "NestedDispatchDue_ThrowsLogicError");
	}

	if (g_Failures == 0)
	{
		std::printf("All tests passed.\n");
		return 0;
	}

	std::printf("%d test(s) failed.\n", g_Failures);
	return 1;
}
