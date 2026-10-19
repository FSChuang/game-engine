#include "Engine/Network/ExchangeProtocol.h"

#include <cstring>

namespace Engine
{
	namespace
	{
		constexpr std::size_t StrategyTagSize = sizeof(std::uint8_t);
		constexpr std::size_t Uint16FieldSize = sizeof(std::uint16_t);
		constexpr std::size_t Uint32FieldSize = sizeof(std::uint32_t);
		constexpr std::size_t ObjectStateWireSize = Uint32FieldSize * 5; // Id + 4 floats
		constexpr std::size_t RecordCountSize = sizeof(std::uint16_t);
		constexpr std::size_t FrameHeaderSize = StrategyTagSize + Uint32FieldSize + RecordCountSize;

		void AppendUint8(std::vector<std::uint8_t>& bytes, std::uint8_t value)
		{
			bytes.push_back(value);
		}

		void AppendUint16(std::vector<std::uint8_t>& bytes, std::uint16_t value)
		{
			bytes.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
			bytes.push_back(static_cast<std::uint8_t>(value & 0xFFu));
		}

		void AppendUint32(std::vector<std::uint8_t>& bytes, std::uint32_t value)
		{
			bytes.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFFu));
			bytes.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFFu));
			bytes.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
			bytes.push_back(static_cast<std::uint8_t>(value & 0xFFu));
		}

		void AppendFloat(std::vector<std::uint8_t>& bytes, float value)
		{
			std::uint32_t bits;
			std::memcpy(&bits, &value, sizeof(bits));
			AppendUint32(bytes, bits);
		}

		void AppendObjectState(std::vector<std::uint8_t>& bytes, const ExchangeObjectState& object)
		{
			AppendUint32(bytes, object.Id);
			AppendFloat(bytes, object.PositionX);
			AppendFloat(bytes, object.PositionY);
			AppendFloat(bytes, object.VelocityX);
			AppendFloat(bytes, object.VelocityY);
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

		bool ReadUint32(const std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t& outValue)
		{
			if (offset + sizeof(std::uint32_t) > bytes.size())
			{
				return false;
			}
			outValue = (static_cast<std::uint32_t>(bytes[offset]) << 24) |
			           (static_cast<std::uint32_t>(bytes[offset + 1]) << 16) |
			           (static_cast<std::uint32_t>(bytes[offset + 2]) << 8) | static_cast<std::uint32_t>(bytes[offset + 3]);
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

		bool ReadObjectState(const std::vector<std::uint8_t>& bytes, std::size_t offset, ExchangeObjectState& outObject)
		{
			std::uint32_t id;
			float positionX;
			float positionY;
			float velocityX;
			float velocityY;
			if (!ReadUint32(bytes, offset, id) || !ReadFloat(bytes, offset + Uint32FieldSize, positionX) ||
			    !ReadFloat(bytes, offset + Uint32FieldSize * 2, positionY) ||
			    !ReadFloat(bytes, offset + Uint32FieldSize * 3, velocityX) ||
			    !ReadFloat(bytes, offset + Uint32FieldSize * 4, velocityY))
			{
				return false;
			}
			outObject = ExchangeObjectState{ id, positionX, positionY, velocityX, velocityY };
			return true;
		}
	}

	std::optional<std::vector<std::uint8_t>> EncodeExchangeFrame(ExchangeStrategy strategy, std::uint32_t iterationSeq,
	                                                              const std::vector<ExchangeObjectState>& objects)
	{
		if (objects.size() > MaxExchangeObjects)
		{
			return std::nullopt;
		}

		std::vector<std::uint8_t> bytes;
		bytes.reserve(FrameHeaderSize + ObjectStateWireSize * objects.size());
		AppendUint8(bytes, static_cast<std::uint8_t>(strategy));
		AppendUint32(bytes, iterationSeq);
		AppendUint16(bytes, static_cast<std::uint16_t>(objects.size()));
		for (const ExchangeObjectState& object : objects)
		{
			AppendObjectState(bytes, object);
		}
		return bytes;
	}

	std::optional<ExchangeFrame> DecodeExchangeFrame(const std::vector<std::uint8_t>& bytes)
	{
		if (bytes.size() < FrameHeaderSize)
		{
			return std::nullopt;
		}

		std::uint8_t tag;
		if (!ReadUint8(bytes, 0, tag))
		{
			return std::nullopt;
		}
		if (tag != static_cast<std::uint8_t>(ExchangeStrategy::FullState) &&
		    tag != static_cast<std::uint8_t>(ExchangeStrategy::DeltaState))
		{
			return std::nullopt;
		}

		std::uint32_t iterationSeq;
		if (!ReadUint32(bytes, StrategyTagSize, iterationSeq))
		{
			return std::nullopt;
		}

		std::uint16_t recordCount;
		if (!ReadUint16(bytes, StrategyTagSize + Uint32FieldSize, recordCount))
		{
			return std::nullopt;
		}

		if (recordCount > MaxExchangeObjects)
		{
			return std::nullopt;
		}

		std::size_t expectedSize = FrameHeaderSize + ObjectStateWireSize * recordCount;
		if (bytes.size() != expectedSize)
		{
			return std::nullopt;
		}

		std::vector<ExchangeObjectState> objects;
		objects.reserve(recordCount);
		std::size_t offset = FrameHeaderSize;
		for (std::uint16_t i = 0; i < recordCount; ++i)
		{
			ExchangeObjectState object;
			if (!ReadObjectState(bytes, offset, object))
			{
				return std::nullopt;
			}
			objects.push_back(object);
			offset += ObjectStateWireSize;
		}

		return ExchangeFrame{ static_cast<ExchangeStrategy>(tag), iterationSeq, std::move(objects) };
	}
}
