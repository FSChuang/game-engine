// Focused unit tests for Engine's generic object-state exchange protocol (CSC 581
// performance checkpoint). Pure logic, no sockets, no SDL. Floats round-trip through
// their exact IEEE-754 bit pattern, so exact equality is valid here — no epsilon
// comparison needed.

#include "Engine/Network/ExchangeProtocol.h"

#include <cstdio>

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

static Engine::ExchangeObjectState MakeObject(std::uint32_t id, float positionX, float positionY, float velocityX,
                                               float velocityY)
{
	return Engine::ExchangeObjectState{ id, positionX, positionY, velocityX, velocityY };
}

static bool ObjectsEqual(const Engine::ExchangeObjectState& a, const Engine::ExchangeObjectState& b)
{
	return a.Id == b.Id && a.PositionX == b.PositionX && a.PositionY == b.PositionY && a.VelocityX == b.VelocityX &&
	       a.VelocityY == b.VelocityY;
}

int main()
{
	// FullState round-trip with several objects preserves every field exactly, and the
	// decoded strategy tag matches what was encoded.
	{
		std::vector<Engine::ExchangeObjectState> objects{ MakeObject(1, 10.0f, 20.0f, 1.0f, 0.0f),
			                                               MakeObject(2, -5.5f, 100.25f, 0.0f, -3.0f),
			                                               MakeObject(3, 0.0f, 0.0f, 0.0f, 0.0f) };
		std::optional<std::vector<std::uint8_t>> encoded =
		    Engine::EncodeExchangeFrame(Engine::ExchangeStrategy::FullState, 42, objects);
		Check(encoded.has_value(), "FullState_Encode_Succeeds");

		std::optional<Engine::ExchangeFrame> decoded = encoded.has_value() ? Engine::DecodeExchangeFrame(*encoded)
		                                                                   : std::nullopt;
		Check(decoded.has_value(), "FullState_Decode_Succeeds");
		Check(decoded.has_value() && decoded->Strategy == Engine::ExchangeStrategy::FullState,
		      "FullState_RoundTrip_PreservesStrategyTag");
		Check(decoded.has_value() && decoded->IterationSeq == 42, "FullState_RoundTrip_PreservesIterationSeq");
		Check(decoded.has_value() && decoded->Objects.size() == 3, "FullState_RoundTrip_PreservesObjectCount");
		bool allMatch = decoded.has_value() && decoded->Objects.size() == 3;
		for (std::size_t i = 0; allMatch && i < objects.size(); ++i)
		{
			allMatch = ObjectsEqual(decoded->Objects[i], objects[i]);
		}
		Check(allMatch, "FullState_RoundTrip_PreservesEveryObjectExactly");
	}

	// DeltaState round-trip with only a SUBSET of objects (the "changed" ones) preserves
	// exactly that subset — this is the core behavioral difference from FullState: the
	// same encoder, but the caller hands it fewer records.
	{
		std::vector<Engine::ExchangeObjectState> changedOnly{ MakeObject(7, 1.0f, 2.0f, 3.0f, 4.0f) };
		std::optional<std::vector<std::uint8_t>> encoded =
		    Engine::EncodeExchangeFrame(Engine::ExchangeStrategy::DeltaState, 5, changedOnly);
		std::optional<Engine::ExchangeFrame> decoded = encoded.has_value() ? Engine::DecodeExchangeFrame(*encoded)
		                                                                   : std::nullopt;

		Check(decoded.has_value() && decoded->Strategy == Engine::ExchangeStrategy::DeltaState,
		      "DeltaState_RoundTrip_PreservesStrategyTag");
		Check(decoded.has_value() && decoded->Objects.size() == 1,
		      "DeltaState_RoundTrip_OnlyCarriesTheChangedObject");
		Check(decoded.has_value() && !decoded->Objects.empty() && ObjectsEqual(decoded->Objects[0], changedOnly[0]),
		      "DeltaState_RoundTrip_PreservesChangedObjectExactly");
	}

	// An empty object list (every object unchanged this iteration) round-trips to zero
	// records — the minimal valid DeltaState frame.
	{
		std::optional<std::vector<std::uint8_t>> encoded =
		    Engine::EncodeExchangeFrame(Engine::ExchangeStrategy::DeltaState, 9, {});
		Check(encoded.has_value(), "DeltaState_EncodeEmptyObjectList_Succeeds");
		std::optional<Engine::ExchangeFrame> decoded = encoded.has_value() ? Engine::DecodeExchangeFrame(*encoded)
		                                                                   : std::nullopt;
		Check(decoded.has_value() && decoded->Objects.empty(), "DeltaState_RoundTrip_UnchangedObjectsOmitted");
	}

	// Distinct object ids in the same frame never collide/overwrite one another.
	{
		std::vector<Engine::ExchangeObjectState> objects{ MakeObject(100, 1.0f, 1.0f, 1.0f, 1.0f),
			                                               MakeObject(200, 2.0f, 2.0f, 2.0f, 2.0f) };
		std::optional<std::vector<std::uint8_t>> encoded =
		    Engine::EncodeExchangeFrame(Engine::ExchangeStrategy::FullState, 0, objects);
		std::optional<Engine::ExchangeFrame> decoded = encoded.has_value() ? Engine::DecodeExchangeFrame(*encoded)
		                                                                   : std::nullopt;
		Check(decoded.has_value() && decoded->Objects.size() == 2 && decoded->Objects[0].Id == 100 &&
		          decoded->Objects[1].Id == 200,
		      "DistinctObjectIds_DoNotCollideOrOverwrite");
	}

	// Encoding more than MaxExchangeObjects is rejected, exactly like an oversized roster
	// in the player protocol.
	{
		std::vector<Engine::ExchangeObjectState> tooMany;
		for (std::size_t i = 0; i < Engine::MaxExchangeObjects + 1; ++i)
		{
			tooMany.push_back(MakeObject(static_cast<std::uint32_t>(i), 0.0f, 0.0f, 0.0f, 0.0f));
		}
		std::optional<std::vector<std::uint8_t>> encoded =
		    Engine::EncodeExchangeFrame(Engine::ExchangeStrategy::FullState, 0, tooMany);
		Check(!encoded.has_value(), "Encode_MoreThanMaxExchangeObjects_Rejected");
	}

	// Malformed/truncated/wrong-tag input is rejected by the decoder.
	{
		Check(!Engine::DecodeExchangeFrame({}).has_value(), "Decode_EmptyBuffer_Rejected");

		std::vector<std::uint8_t> wrongTag{ 0xFF, 0, 0, 0, 0, 0, 0 };
		Check(!Engine::DecodeExchangeFrame(wrongTag).has_value(), "Decode_UnrecognizedStrategyTag_Rejected");

		std::optional<std::vector<std::uint8_t>> encoded = Engine::EncodeExchangeFrame(
		    Engine::ExchangeStrategy::FullState, 1, { MakeObject(1, 1.0f, 1.0f, 1.0f, 1.0f) });
		std::vector<std::uint8_t> truncated = *encoded;
		truncated.resize(truncated.size() - 3); // chop into the middle of the one record
		Check(!Engine::DecodeExchangeFrame(truncated).has_value(), "Decode_TruncatedRecord_Rejected");

		std::vector<std::uint8_t> plusExtra = *encoded;
		plusExtra.push_back(0xFF);
		Check(!Engine::DecodeExchangeFrame(plusExtra).has_value(), "Decode_ExtraTrailingByte_Rejected");

		std::vector<std::uint8_t> headerOnly{ 1, 0, 0, 0, 0 }; // valid tag, but short of the 2-byte count field
		Check(!Engine::DecodeExchangeFrame(headerOnly).has_value(), "Decode_TruncatedHeader_Rejected");
	}

	// A record count exceeding MaxExchangeObjects is rejected even if the buffer length
	// happens to match that (invalid) declared count's expected size.
	{
		std::vector<std::uint8_t> malformed{ static_cast<std::uint8_t>(Engine::ExchangeStrategy::FullState),
			                                  0, 0, 0, 0 };
		// RecordCount = MaxExchangeObjects + 1, encoded big-endian.
		std::uint16_t badCount = static_cast<std::uint16_t>(Engine::MaxExchangeObjects + 1);
		malformed.push_back(static_cast<std::uint8_t>((badCount >> 8) & 0xFFu));
		malformed.push_back(static_cast<std::uint8_t>(badCount & 0xFFu));
		Check(!Engine::DecodeExchangeFrame(malformed).has_value(), "Decode_RecordCountExceedsMax_Rejected");
	}

	std::printf("\n%s\n", g_Failures == 0 ? "All tests passed." : "Some tests FAILED.");
	return g_Failures == 0 ? 0 : 1;
}
