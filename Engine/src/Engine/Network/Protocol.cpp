#include "Engine/Network/Protocol.h"

#include <cstring>

namespace Engine
{
	namespace
	{
		constexpr std::size_t MessageTypeSize = sizeof(std::uint8_t);
		constexpr std::size_t Uint16FieldSize = sizeof(std::uint16_t);
		constexpr std::size_t Uint32FieldSize = sizeof(std::uint32_t);
		constexpr std::size_t PlayerStateWireSize = Uint32FieldSize * 5; // Id + 4 floats
		constexpr std::size_t PlatformStateWireSize = Uint32FieldSize * 4; // 4 floats
		constexpr std::size_t LeavingFlagSize = sizeof(std::uint8_t);
		constexpr std::size_t JoinRequestWireSize = MessageTypeSize;
		constexpr std::size_t JoinAcceptedWireSize =
		    MessageTypeSize + Uint32FieldSize + Uint16FieldSize + Uint16FieldSize;
		constexpr std::size_t StateUpdateWireSize = MessageTypeSize + PlayerStateWireSize + LeavingFlagSize;
		constexpr std::size_t RosterCountSize = sizeof(std::uint8_t);
		constexpr std::size_t PeerInfoWireSize = Uint32FieldSize + Uint16FieldSize; // Id + P2pPort
		constexpr std::size_t PeerCountSize = sizeof(std::uint8_t);
		constexpr std::size_t SnapshotHeaderSize =
		    MessageTypeSize + Uint32FieldSize + PlatformStateWireSize + RosterCountSize;
		constexpr std::size_t ErrorCodeSize = sizeof(std::uint8_t);
		constexpr std::size_t ErrorWireSize = MessageTypeSize + ErrorCodeSize;

		void AppendUint8(std::vector<std::uint8_t>& bytes, std::uint8_t value)
		{
			bytes.push_back(value);
		}

		// Big-endian / network byte order, independent of host endianness.
		void AppendUint32(std::vector<std::uint8_t>& bytes, std::uint32_t value)
		{
			bytes.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFFu));
			bytes.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFFu));
			bytes.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
			bytes.push_back(static_cast<std::uint8_t>(value & 0xFFu));
		}

		// Big-endian / network byte order, same convention as AppendUint32.
		void AppendUint16(std::vector<std::uint8_t>& bytes, std::uint16_t value)
		{
			bytes.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
			bytes.push_back(static_cast<std::uint8_t>(value & 0xFFu));
		}

		void AppendFloat(std::vector<std::uint8_t>& bytes, float value)
		{
			std::uint32_t bits;
			std::memcpy(&bits, &value, sizeof(bits));
			AppendUint32(bytes, bits);
		}

		void AppendPlayerState(std::vector<std::uint8_t>& bytes, const PlayerState& state)
		{
			AppendUint32(bytes, state.Id);
			AppendFloat(bytes, state.PositionX);
			AppendFloat(bytes, state.PositionY);
			AppendFloat(bytes, state.VelocityX);
			AppendFloat(bytes, state.VelocityY);
		}

		void AppendPlatformState(std::vector<std::uint8_t>& bytes, const PlatformState& platform)
		{
			AppendFloat(bytes, platform.PositionX);
			AppendFloat(bytes, platform.PositionY);
			AppendFloat(bytes, platform.VelocityX);
			AppendFloat(bytes, platform.VelocityY);
		}

		void AppendPeerInfo(std::vector<std::uint8_t>& bytes, const PeerInfo& peer)
		{
			AppendUint32(bytes, peer.Id);
			AppendUint16(bytes, peer.P2pPort);
		}

		bool ReadUint8(const std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint8_t& outValue)
		{
			if (offset + sizeof(std::uint8_t) > bytes.size())
			{
				return false;
			}
			outValue = bytes[offset];
			return true;
		}

		bool ReadUint32(const std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t& outValue)
		{
			if (offset + sizeof(std::uint32_t) > bytes.size())
			{
				return false;
			}
			outValue = (static_cast<std::uint32_t>(bytes[offset]) << 24) |
			           (static_cast<std::uint32_t>(bytes[offset + 1]) << 16) |
			           (static_cast<std::uint32_t>(bytes[offset + 2]) << 8) |
			           static_cast<std::uint32_t>(bytes[offset + 3]);
			return true;
		}

		bool ReadUint16(const std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint16_t& outValue)
		{
			if (offset + sizeof(std::uint16_t) > bytes.size())
			{
				return false;
			}
			outValue = static_cast<std::uint16_t>((static_cast<std::uint16_t>(bytes[offset]) << 8) |
			                                       static_cast<std::uint16_t>(bytes[offset + 1]));
			return true;
		}

		bool ReadFloat(const std::vector<std::uint8_t>& bytes, std::size_t offset, float& outValue)
		{
			std::uint32_t bits;
			if (!ReadUint32(bytes, offset, bits))
			{
				return false;
			}
			std::memcpy(&outValue, &bits, sizeof(outValue));
			return true;
		}

		bool ReadPlayerState(const std::vector<std::uint8_t>& bytes, std::size_t offset, PlayerState& outState)
		{
			std::uint32_t id;
			float positionX;
			float positionY;
			float velocityX;
			float velocityY;
			if (!ReadUint32(bytes, offset, id) ||
			    !ReadFloat(bytes, offset + Uint32FieldSize, positionX) ||
			    !ReadFloat(bytes, offset + Uint32FieldSize * 2, positionY) ||
			    !ReadFloat(bytes, offset + Uint32FieldSize * 3, velocityX) ||
			    !ReadFloat(bytes, offset + Uint32FieldSize * 4, velocityY))
			{
				return false;
			}
			outState = PlayerState{ id, positionX, positionY, velocityX, velocityY };
			return true;
		}

		bool ReadPlatformState(const std::vector<std::uint8_t>& bytes, std::size_t offset, PlatformState& outPlatform)
		{
			float positionX;
			float positionY;
			float velocityX;
			float velocityY;
			if (!ReadFloat(bytes, offset, positionX) ||
			    !ReadFloat(bytes, offset + Uint32FieldSize, positionY) ||
			    !ReadFloat(bytes, offset + Uint32FieldSize * 2, velocityX) ||
			    !ReadFloat(bytes, offset + Uint32FieldSize * 3, velocityY))
			{
				return false;
			}
			outPlatform = PlatformState{ positionX, positionY, velocityX, velocityY };
			return true;
		}

		bool ReadPeerInfo(const std::vector<std::uint8_t>& bytes, std::size_t offset, PeerInfo& outPeer)
		{
			std::uint32_t id;
			std::uint16_t p2pPort;
			if (!ReadUint32(bytes, offset, id) || !ReadUint16(bytes, offset + Uint32FieldSize, p2pPort))
			{
				return false;
			}
			outPeer = PeerInfo{ id, p2pPort };
			return true;
		}
	}

	std::vector<std::uint8_t> EncodeJoinRequest()
	{
		std::vector<std::uint8_t> bytes;
		bytes.reserve(JoinRequestWireSize);
		AppendUint8(bytes, static_cast<std::uint8_t>(MessageType::Join));
		return bytes;
	}

	std::vector<std::uint8_t> EncodeJoinAccepted(const JoinAccepted& accepted)
	{
		std::vector<std::uint8_t> bytes;
		bytes.reserve(JoinAcceptedWireSize);
		AppendUint8(bytes, static_cast<std::uint8_t>(MessageType::JoinAccepted));
		AppendUint32(bytes, accepted.AssignedId);
		AppendUint16(bytes, accepted.AssignedPort);
		AppendUint16(bytes, accepted.P2pPort);
		return bytes;
	}

	std::vector<std::uint8_t> EncodeStateUpdate(const StateUpdate& update)
	{
		std::vector<std::uint8_t> bytes;
		bytes.reserve(StateUpdateWireSize);
		AppendUint8(bytes, static_cast<std::uint8_t>(MessageType::StateUpdate));
		AppendPlayerState(bytes, update.State);
		AppendUint8(bytes, update.Leaving ? 1 : 0);
		return bytes;
	}

	std::optional<std::vector<std::uint8_t>> EncodeSnapshot(const Snapshot& snapshot)
	{
		if (snapshot.Roster.size() > MaxPlayers || snapshot.Peers.size() > MaxPlayers)
		{
			return std::nullopt;
		}

		std::vector<std::uint8_t> bytes;
		bytes.reserve(SnapshotHeaderSize + PlayerStateWireSize * snapshot.Roster.size() + PeerCountSize +
		              PeerInfoWireSize * snapshot.Peers.size());
		AppendUint8(bytes, static_cast<std::uint8_t>(MessageType::Snapshot));
		AppendUint32(bytes, snapshot.RecipientId);
		AppendPlatformState(bytes, snapshot.Platform);
		AppendUint8(bytes, static_cast<std::uint8_t>(snapshot.Roster.size()));
		for (const PlayerState& state : snapshot.Roster)
		{
			AppendPlayerState(bytes, state);
		}
		AppendUint8(bytes, static_cast<std::uint8_t>(snapshot.Peers.size()));
		for (const PeerInfo& peer : snapshot.Peers)
		{
			AppendPeerInfo(bytes, peer);
		}
		return bytes;
	}

	std::optional<JoinRequest> DecodeJoinRequest(const std::vector<std::uint8_t>& bytes)
	{
		if (bytes.size() != JoinRequestWireSize)
		{
			return std::nullopt;
		}

		std::uint8_t type;
		if (!ReadUint8(bytes, 0, type) || type != static_cast<std::uint8_t>(MessageType::Join))
		{
			return std::nullopt;
		}

		return JoinRequest{};
	}

	std::optional<JoinAccepted> DecodeJoinAccepted(const std::vector<std::uint8_t>& bytes)
	{
		if (bytes.size() != JoinAcceptedWireSize)
		{
			return std::nullopt;
		}

		std::uint8_t type;
		if (!ReadUint8(bytes, 0, type) || type != static_cast<std::uint8_t>(MessageType::JoinAccepted))
		{
			return std::nullopt;
		}

		std::uint32_t assignedId;
		if (!ReadUint32(bytes, MessageTypeSize, assignedId))
		{
			return std::nullopt;
		}

		std::uint16_t assignedPort;
		if (!ReadUint16(bytes, MessageTypeSize + Uint32FieldSize, assignedPort))
		{
			return std::nullopt;
		}

		std::uint16_t p2pPort;
		if (!ReadUint16(bytes, MessageTypeSize + Uint32FieldSize + Uint16FieldSize, p2pPort))
		{
			return std::nullopt;
		}

		return JoinAccepted{ assignedId, assignedPort, p2pPort };
	}

	std::optional<StateUpdate> DecodeStateUpdate(const std::vector<std::uint8_t>& bytes)
	{
		if (bytes.size() != StateUpdateWireSize)
		{
			return std::nullopt;
		}

		std::uint8_t type;
		if (!ReadUint8(bytes, 0, type) || type != static_cast<std::uint8_t>(MessageType::StateUpdate))
		{
			return std::nullopt;
		}

		PlayerState state;
		if (!ReadPlayerState(bytes, MessageTypeSize, state))
		{
			return std::nullopt;
		}

		std::uint8_t leavingByte;
		if (!ReadUint8(bytes, MessageTypeSize + PlayerStateWireSize, leavingByte) ||
		    (leavingByte != 0 && leavingByte != 1))
		{
			return std::nullopt;
		}

		return StateUpdate{ state, leavingByte == 1 };
	}

	std::optional<Snapshot> DecodeSnapshot(const std::vector<std::uint8_t>& bytes)
	{
		if (bytes.size() < SnapshotHeaderSize)
		{
			return std::nullopt;
		}

		std::uint8_t type;
		if (!ReadUint8(bytes, 0, type) || type != static_cast<std::uint8_t>(MessageType::Snapshot))
		{
			return std::nullopt;
		}

		std::uint32_t recipientId;
		if (!ReadUint32(bytes, MessageTypeSize, recipientId))
		{
			return std::nullopt;
		}

		PlatformState platform;
		if (!ReadPlatformState(bytes, MessageTypeSize + Uint32FieldSize, platform))
		{
			return std::nullopt;
		}

		std::uint8_t rosterCount;
		if (!ReadUint8(bytes, MessageTypeSize + Uint32FieldSize + PlatformStateWireSize, rosterCount))
		{
			return std::nullopt;
		}

		if (rosterCount > MaxPlayers)
		{
			return std::nullopt;
		}

		std::size_t peerCountOffset = SnapshotHeaderSize + PlayerStateWireSize * rosterCount;

		std::uint8_t peerCount;
		if (!ReadUint8(bytes, peerCountOffset, peerCount))
		{
			return std::nullopt;
		}

		if (peerCount > MaxPlayers)
		{
			return std::nullopt;
		}

		std::size_t expectedSize = peerCountOffset + PeerCountSize + PeerInfoWireSize * peerCount;
		if (bytes.size() != expectedSize)
		{
			return std::nullopt;
		}

		std::vector<PlayerState> roster;
		roster.reserve(rosterCount);
		std::size_t offset = SnapshotHeaderSize;
		for (std::uint8_t i = 0; i < rosterCount; ++i)
		{
			PlayerState state;
			if (!ReadPlayerState(bytes, offset, state))
			{
				return std::nullopt;
			}
			roster.push_back(state);
			offset += PlayerStateWireSize;
		}

		std::vector<PeerInfo> peers;
		peers.reserve(peerCount);
		offset = peerCountOffset + PeerCountSize;
		for (std::uint8_t i = 0; i < peerCount; ++i)
		{
			PeerInfo peer;
			if (!ReadPeerInfo(bytes, offset, peer))
			{
				return std::nullopt;
			}
			peers.push_back(peer);
			offset += PeerInfoWireSize;
		}

		return Snapshot{ recipientId, platform, std::move(roster), std::move(peers) };
	}

	std::vector<std::uint8_t> EncodeError(ErrorCode code)
	{
		std::vector<std::uint8_t> bytes;
		bytes.reserve(ErrorWireSize);
		AppendUint8(bytes, static_cast<std::uint8_t>(MessageType::Error));
		AppendUint8(bytes, static_cast<std::uint8_t>(code));
		return bytes;
	}

	std::optional<ErrorResponse> DecodeError(const std::vector<std::uint8_t>& bytes)
	{
		if (bytes.size() != ErrorWireSize)
		{
			return std::nullopt;
		}

		std::uint8_t type;
		if (!ReadUint8(bytes, 0, type) || type != static_cast<std::uint8_t>(MessageType::Error))
		{
			return std::nullopt;
		}

		std::uint8_t code;
		if (!ReadUint8(bytes, MessageTypeSize, code))
		{
			return std::nullopt;
		}

		switch (static_cast<ErrorCode>(code))
		{
			case ErrorCode::MalformedRequest:
			case ErrorCode::RegistryFull:
			case ErrorCode::UnknownPlayer:
				return ErrorResponse{ static_cast<ErrorCode>(code) };
		}

		return std::nullopt; // unrecognized error code value
	}

	std::optional<MessageType> PeekMessageType(const std::vector<std::uint8_t>& bytes)
	{
		if (bytes.empty())
		{
			return std::nullopt;
		}

		switch (static_cast<MessageType>(bytes[0]))
		{
			case MessageType::Join:
			case MessageType::StateUpdate:
			case MessageType::Snapshot:
			case MessageType::Error:
			case MessageType::JoinAccepted:
				return static_cast<MessageType>(bytes[0]);
		}

		return std::nullopt; // unrecognized message-type byte
	}
}
