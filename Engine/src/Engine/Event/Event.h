#pragma once

#include "Engine/Core/Core.h"
#include "Engine/GameObject/GameObject.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <variant>

namespace Engine
{
	// Event representation (Milestone 4, Event Management System design decision 1).
	//
	// One small envelope (Event) carrying one of a closed set of plain-data payload structs
	// (EventPayload, a std::variant) — deliberately not std::any, not a polymorphic heap
	// class hierarchy, not a string-keyed dictionary. A closed variant means every payload
	// type is known at compile time, handlers get a type-checked std::get<T>(), and an event
	// is a cheap, copyable value that a later checkpoint can record, encode or re-raise
	// without any virtual machinery.
	//
	// Everything here is SDL-free, ZeroMQ-free and game-agnostic: ObjectId is the only type
	// borrowed from the object model, and it lives in EngineCore too.

	/// Which kind of event this is — what handlers register for. Always equal to the index
	/// of the active alternative in EventPayload (see TypeOf()), so the type and the payload
	/// can never disagree.
	enum class EventType : std::uint8_t
	{
		Collision = 0,
		Death,
		Spawn,
		Input,
		// Replay control (Milestone 4). Ordinary events like any other: raising one only
		// queues it, and ReplayRecorder reacts when it is dispatched.
		ReplayStart,
		ReplayStop,
		ReplayPlay
	};

	/// Dispatch priority. Only ever breaks a tie between events with the SAME timestamp (see
	/// EventManager's ordering rules); it never lets a newer event jump ahead of an older one.
	enum class EventPriority : std::uint8_t
	{
		Low = 0,
		Normal,
		High,
		Critical
	};

	/// Engine/game-neutral gameplay input intent. Deliberately not SDL_Scancode: which
	/// physical key (or network message, or replay entry) produced an action is none of the
	/// event system's business, and a headless process has no scancodes at all.
	enum class InputAction : std::uint8_t
	{
		MoveLeft = 0,
		MoveRight,
		Jump
	};

	/// ObjectId value meaning "no object": GameObjectRegistry never assigns 0.
	constexpr ObjectId NoObjectId = 0;

	/// Two runtime objects were found to be meaningfully colliding. The pair is unordered as
	/// far as the engine is concerned; the raiser decides which is A and which is B.
	struct CollisionEventData
	{
		ObjectId A;
		ObjectId B;
	};

	/// An object died. `Cause` is the object responsible (e.g. a hazard or enemy), or
	/// NoObjectId when there is no single causing object.
	struct DeathEventData
	{
		ObjectId Subject;
		ObjectId Cause;
	};

	/// An object (re)appeared at `Position`.
	struct SpawnEventData
	{
		ObjectId Subject;
		Vector2 Position;
	};

	/// A gameplay input action started (`Pressed == true`) or stopped (`false`).
	struct InputEventData
	{
		InputAction Action;
		bool Pressed;
	};

	/// Replay control payloads (no data needed: the event type IS the command).
	/// ReplayStart: begin a fresh recording. ReplayStop: end the recording in progress.
	/// ReplayPlay: play the completed recording back. See ReplayRecorder for exact semantics.
	struct ReplayStartEventData
	{
	};

	struct ReplayStopEventData
	{
	};

	struct ReplayPlayEventData
	{
	};

	/// The closed set of event payloads. Alternative order MUST match EventType's values;
	/// the static_asserts below enforce it.
	using EventPayload = std::variant<CollisionEventData, DeathEventData, SpawnEventData, InputEventData,
	                                  ReplayStartEventData, ReplayStopEventData, ReplayPlayEventData>;

	static_assert(std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(EventType::Collision), EventPayload>,
	                             CollisionEventData>,
	              "EventPayload alternative order must match EventType");
	static_assert(std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(EventType::Death), EventPayload>,
	                             DeathEventData>,
	              "EventPayload alternative order must match EventType");
	static_assert(std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(EventType::Spawn), EventPayload>,
	                             SpawnEventData>,
	              "EventPayload alternative order must match EventType");
	static_assert(std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(EventType::Input), EventPayload>,
	                             InputEventData>,
	              "EventPayload alternative order must match EventType");

	static_assert(std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(EventType::ReplayStart), EventPayload>,
	                             ReplayStartEventData>,
	              "EventPayload alternative order must match EventType");
	static_assert(std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(EventType::ReplayStop), EventPayload>,
	                             ReplayStopEventData>,
	              "EventPayload alternative order must match EventType");
	static_assert(std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(EventType::ReplayPlay), EventPayload>,
	                             ReplayPlayEventData>,
	              "EventPayload alternative order must match EventType");

	/// Number of distinct EventType values.
	constexpr std::size_t EventTypeCount = std::variant_size_v<EventPayload>;

	/// The EventType matching whichever payload alternative is active.
	inline EventType TypeOf(const EventPayload& payload)
	{
		return static_cast<EventType>(payload.index());
	}

	/// What a handler receives. EventManager is the only thing that creates these:
	/// `Timestamp` and `Sequence` are assigned when an event is raised/scheduled, never
	/// supplied by callers, so ordering can't be forged or accidentally corrupted.
	struct Event
	{
		EventType Type;
		/// Logical time (the EventManager's injected time source) at which this event becomes
		/// due: "now" at Raise(), "now + delay" at Schedule().
		double Timestamp;
		EventPriority Priority;
		/// Strictly increasing per EventManager; exists only to make equal-timestamp,
		/// equal-priority ordering a deterministic FIFO.
		std::uint64_t Sequence;
		EventPayload Payload;
	};
}
