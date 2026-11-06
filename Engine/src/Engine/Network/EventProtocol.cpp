#include "Engine/Network/EventProtocol.h"

#include "Engine/Network/ExchangeProtocol.h"

namespace Engine
{
	namespace
	{
		constexpr std::size_t FrameTagSize = sizeof(std::uint8_t);
		constexpr std::size_t EventKindSize = sizeof(std::uint8_t);
		constexpr std::size_t PlayerIdSize = sizeof(std::uint32_t);
		constexpr std::size_t NetworkEventWireSize = FrameTagSize + EventKindSize + PlayerIdSize;

		// The event tag must stay clear of every other leading byte used on the shared peer
		// channel. These fail to compile if a future MessageType / ExchangeStrategy ever grows
		// into 0x10.
		constexpr std::uint8_t HighestOtherLeadingByte =
		    static_cast<std::uint8_t>(MessageType::JoinAccepted) > static_cast<std::uint8_t>(ExchangeStrategy::DeltaState)
		        ? static_cast<std::uint8_t>(MessageType::JoinAccepted)
		        : static_cast<std::uint8_t>(ExchangeStrategy::DeltaState);
		static_assert(NetworkEventFrameTag > HighestOtherLeadingByte,
		              "NetworkEventFrameTag must not collide with MessageType or ExchangeStrategy values");

		bool IsKnownKind(std::uint8_t kind)
		{
			return kind == static_cast<std::uint8_t>(NetworkEventKind::Death);
		}
	}

	std::vector<std::uint8_t> EncodeNetworkEvent(const NetworkEvent& event)
	{
		std::vector<std::uint8_t> bytes;
		bytes.reserve(NetworkEventWireSize);
		bytes.push_back(NetworkEventFrameTag);
		bytes.push_back(static_cast<std::uint8_t>(event.Kind));

		// Big-endian / network byte order, same convention as Protocol.cpp.
		bytes.push_back(static_cast<std::uint8_t>((event.SourcePlayerId >> 24) & 0xFFu));
		bytes.push_back(static_cast<std::uint8_t>((event.SourcePlayerId >> 16) & 0xFFu));
		bytes.push_back(static_cast<std::uint8_t>((event.SourcePlayerId >> 8) & 0xFFu));
		bytes.push_back(static_cast<std::uint8_t>(event.SourcePlayerId & 0xFFu));
		return bytes;
	}

	bool IsNetworkEventFrame(const std::vector<std::uint8_t>& bytes)
	{
		return !bytes.empty() && bytes[0] == NetworkEventFrameTag;
	}

	std::optional<NetworkEvent> DecodeNetworkEvent(const std::vector<std::uint8_t>& bytes)
	{
		if (bytes.size() != NetworkEventWireSize || bytes[0] != NetworkEventFrameTag || !IsKnownKind(bytes[1]))
		{
			return std::nullopt;
		}

		PlayerId sourcePlayerId = (static_cast<PlayerId>(bytes[2]) << 24) | (static_cast<PlayerId>(bytes[3]) << 16) |
		                          (static_cast<PlayerId>(bytes[4]) << 8) | static_cast<PlayerId>(bytes[5]);
		if (sourcePlayerId == 0)
		{
			return std::nullopt;
		}

		return NetworkEvent{ static_cast<NetworkEventKind>(bytes[1]), sourcePlayerId };
	}
}
