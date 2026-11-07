#include "Engine/Network/EventProtocol.h"

#include "Engine/Network/ExchangeProtocol.h"

#include <cmath>
#include <cstring>

namespace Engine
{
	namespace
	{
		constexpr std::size_t FrameTagSize = sizeof(std::uint8_t);
		constexpr std::size_t EventKindSize = sizeof(std::uint8_t);
		constexpr std::size_t PlayerIdSize = sizeof(std::uint32_t);
		constexpr std::size_t FloatFieldSize = sizeof(std::uint32_t);
		// Death and Spawn carry nothing beyond who; Shoot adds origin X, origin Y and heading.
		constexpr std::size_t PlainEventWireSize = FrameTagSize + EventKindSize + PlayerIdSize;
		constexpr std::size_t ShootEventWireSize = PlainEventWireSize + FloatFieldSize * 3;

		// The event tag must stay clear of every other leading byte used on the shared peer
		// channel. These fail to compile if a future MessageType / ExchangeStrategy ever grows
		// into 0x10.
		constexpr std::uint8_t HighestOtherLeadingByte =
		    static_cast<std::uint8_t>(MessageType::JoinAccepted) > static_cast<std::uint8_t>(ExchangeStrategy::DeltaState)
		        ? static_cast<std::uint8_t>(MessageType::JoinAccepted)
		        : static_cast<std::uint8_t>(ExchangeStrategy::DeltaState);
		static_assert(NetworkEventFrameTag > HighestOtherLeadingByte,
		              "NetworkEventFrameTag must not collide with MessageType or ExchangeStrategy values");

		// The exact frame size each kind must have; 0 for an unknown kind.
		std::size_t WireSizeForKind(std::uint8_t kind)
		{
			switch (kind)
			{
			case static_cast<std::uint8_t>(NetworkEventKind::Death):
			case static_cast<std::uint8_t>(NetworkEventKind::Spawn):
				return PlainEventWireSize;
			case static_cast<std::uint8_t>(NetworkEventKind::Shoot):
				return ShootEventWireSize;
			default:
				return 0;
			}
		}

		// Big-endian / network byte order, same convention as Protocol.cpp.
		void AppendUint32(std::vector<std::uint8_t>& bytes, std::uint32_t value)
		{
			bytes.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFFu));
			bytes.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFFu));
			bytes.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
			bytes.push_back(static_cast<std::uint8_t>(value & 0xFFu));
		}

		// A float travels as its IEEE-754 bit pattern (via memcpy, never a pointer cast).
		void AppendFloat(std::vector<std::uint8_t>& bytes, float value)
		{
			std::uint32_t bits;
			static_assert(sizeof(bits) == sizeof(value), "float must be 32 bits");
			std::memcpy(&bits, &value, sizeof(bits));
			AppendUint32(bytes, bits);
		}

		std::uint32_t ReadUint32(const std::vector<std::uint8_t>& bytes, std::size_t offset)
		{
			return (static_cast<std::uint32_t>(bytes[offset]) << 24) | (static_cast<std::uint32_t>(bytes[offset + 1]) << 16) |
			       (static_cast<std::uint32_t>(bytes[offset + 2]) << 8) | static_cast<std::uint32_t>(bytes[offset + 3]);
		}

		float ReadFloat(const std::vector<std::uint8_t>& bytes, std::size_t offset)
		{
			std::uint32_t bits = ReadUint32(bytes, offset);
			float value;
			std::memcpy(&value, &bits, sizeof(value));
			return value;
		}
	}

	std::vector<std::uint8_t> EncodeNetworkEvent(const NetworkEvent& event)
	{
		std::vector<std::uint8_t> bytes;
		bytes.reserve(ShootEventWireSize);
		bytes.push_back(NetworkEventFrameTag);
		bytes.push_back(static_cast<std::uint8_t>(event.Kind));
		AppendUint32(bytes, event.SourcePlayerId);

		if (event.Kind == NetworkEventKind::Shoot)
		{
			AppendFloat(bytes, event.OriginX);
			AppendFloat(bytes, event.OriginY);
			AppendFloat(bytes, event.HeadingRadians);
		}
		return bytes;
	}

	bool IsNetworkEventFrame(const std::vector<std::uint8_t>& bytes)
	{
		return !bytes.empty() && bytes[0] == NetworkEventFrameTag;
	}

	std::optional<NetworkEvent> DecodeNetworkEvent(const std::vector<std::uint8_t>& bytes)
	{
		if (bytes.size() < PlainEventWireSize || bytes[0] != NetworkEventFrameTag)
		{
			return std::nullopt;
		}

		// An unknown kind has size 0 and so never matches; a known kind must be EXACTLY its size.
		if (bytes.size() != WireSizeForKind(bytes[1]))
		{
			return std::nullopt;
		}

		PlayerId sourcePlayerId = ReadUint32(bytes, FrameTagSize + EventKindSize);
		if (sourcePlayerId == 0)
		{
			return std::nullopt;
		}

		NetworkEvent event{ static_cast<NetworkEventKind>(bytes[1]), sourcePlayerId };
		if (event.Kind == NetworkEventKind::Shoot)
		{
			event.OriginX = ReadFloat(bytes, PlainEventWireSize);
			event.OriginY = ReadFloat(bytes, PlainEventWireSize + FloatFieldSize);
			event.HeadingRadians = ReadFloat(bytes, PlainEventWireSize + FloatFieldSize * 2);
			if (!std::isfinite(event.OriginX) || !std::isfinite(event.OriginY) || !std::isfinite(event.HeadingRadians))
			{
				return std::nullopt;
			}
		}
		return event;
	}
}
