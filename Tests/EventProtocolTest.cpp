// Focused unit tests for Engine::EventProtocol (Milestone 4 Checkpoint 3): the network event
// notification wire format. Pure encode/decode logic. This target links EngineNetworkCore
// only — no SDL, no ZeroMQ — which is itself the proof the format needs neither.

#include "Engine/Network/EventProtocol.h"
#include "Engine/Network/ExchangeProtocol.h"
#include "Engine/Network/Protocol.h"

#include <cstdio>
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

int main()
{
	// 1 & 2. Death round trip preserves the kind and the exact PlayerId.
	{
		NetworkEvent original{ NetworkEventKind::Death, 0xA1B2C3D4u };
		std::optional<NetworkEvent> decoded = DecodeNetworkEvent(EncodeNetworkEvent(original));
		Check(decoded.has_value() && decoded->Kind == NetworkEventKind::Death, "Death_RoundTrip_PreservesKind");
		Check(decoded.has_value() && decoded->SourcePlayerId == 0xA1B2C3D4u, "Death_RoundTrip_PreservesExactPlayerId");
	}
	{
		bool allPreserved = true;
		for (PlayerId id : { 1u, 2u, 7u, 255u, 256u, 65536u, 0x7FFFFFFFu, 0xFFFFFFFFu })
		{
			auto decoded = DecodeNetworkEvent(EncodeNetworkEvent({ NetworkEventKind::Death, id }));
			allPreserved = allPreserved && decoded.has_value() && decoded->SourcePlayerId == id;
		}
		Check(allPreserved, "Death_RoundTrip_PreservesPlayerIdAcrossByteBoundaries");
	}

	// 7. Exact encoded size and byte layout (tag, kind, big-endian PlayerId) — the layout
	// itself carries no ObjectId, no timestamp and no cause: exactly 6 bytes, all accounted for.
	{
		std::vector<std::uint8_t> bytes = EncodeNetworkEvent({ NetworkEventKind::Death, 0x01020304u });
		Check(bytes.size() == 6, "Encode_ExactSize_SixBytes");
		Check(bytes == std::vector<std::uint8_t>({ 0x10, 0x01, 0x01, 0x02, 0x03, 0x04 }),
		      "Encode_ExactBytes_TagKindBigEndianPlayerId");
		// 9. Nothing but the PlayerId varies: two different ids differ ONLY in the last four bytes.
		std::vector<std::uint8_t> other = EncodeNetworkEvent({ NetworkEventKind::Death, 0x05060708u });
		Check(bytes[0] == other[0] && bytes[1] == other[1] && bytes.size() == other.size(),
		      "Encode_NoHiddenFields_OnlyPlayerIdVaries");
	}

	// 4. Malformed / truncated / padded input is rejected.
	{
		std::vector<std::uint8_t> good = EncodeNetworkEvent({ NetworkEventKind::Death, 9 });
		Check(!DecodeNetworkEvent({}).has_value(), "Decode_Empty_Rejected");
		bool truncatedRejected = true;
		for (std::size_t length = 0; length < good.size(); ++length)
		{
			std::vector<std::uint8_t> truncated(good.begin(), good.begin() + static_cast<std::ptrdiff_t>(length));
			truncatedRejected = truncatedRejected && !DecodeNetworkEvent(truncated).has_value();
		}
		Check(truncatedRejected, "Decode_EveryTruncation_Rejected");
		std::vector<std::uint8_t> padded = good;
		padded.push_back(0);
		Check(!DecodeNetworkEvent(padded).has_value(), "Decode_PaddedFrame_Rejected");
	}

	// 5. Wrong tag rejected.
	{
		std::vector<std::uint8_t> wrongTag = EncodeNetworkEvent({ NetworkEventKind::Death, 9 });
		wrongTag[0] = 0x11;
		Check(!DecodeNetworkEvent(wrongTag).has_value(), "Decode_WrongTag_Rejected");
		wrongTag[0] = static_cast<std::uint8_t>(MessageType::StateUpdate);
		Check(!DecodeNetworkEvent(wrongTag).has_value(), "Decode_StateUpdateTag_Rejected");
	}

	// 6. Unknown event kind rejected.
	{
		std::vector<std::uint8_t> bytes = EncodeNetworkEvent({ NetworkEventKind::Death, 9 });
		bool allRejected = true;
		for (int kind : { 0, 2, 3, 100, 255 })
		{
			bytes[1] = static_cast<std::uint8_t>(kind);
			allRejected = allRejected && !DecodeNetworkEvent(bytes).has_value();
		}
		Check(allRejected, "Decode_UnknownKind_Rejected");
	}

	// PlayerId 0 is the reserved "unassigned" id and is never a valid event source.
	{
		Check(!DecodeNetworkEvent(EncodeNetworkEvent({ NetworkEventKind::Death, 0 })).has_value(),
		      "Decode_ZeroPlayerId_Rejected");
	}

	// Multiplexing safety on the shared peer channel: the three formats never decode as one
	// another, and IsNetworkEventFrame routes by the leading byte alone.
	{
		std::vector<std::uint8_t> eventFrame = EncodeNetworkEvent({ NetworkEventKind::Death, 3 });
		std::vector<std::uint8_t> stateFrame =
		    EncodeStateUpdate(StateUpdate{ PlayerState{ 3, 1.0f, 2.0f, 3.0f, 4.0f }, false });
		std::optional<std::vector<std::uint8_t>> exchangeFrame =
		    EncodeExchangeFrame(ExchangeStrategy::DeltaState, 1, { ExchangeObjectState{ 3, 1.0f, 2.0f, 3.0f, 4.0f } });

		Check(IsNetworkEventFrame(eventFrame) && !IsNetworkEventFrame(stateFrame) &&
		          !IsNetworkEventFrame(*exchangeFrame) && !IsNetworkEventFrame({}),
		      "Multiplex_IsNetworkEventFrame_RoutesByLeadingByte");
		Check(!DecodeStateUpdate(eventFrame).has_value(), "Multiplex_EventFrame_IsNotAStateUpdate");
		Check(!DecodeExchangeFrame(eventFrame).has_value(), "Multiplex_EventFrame_IsNotAnExchangeFrame");
		Check(!DecodeNetworkEvent(stateFrame).has_value(), "Multiplex_StateUpdate_IsNotAnEvent");
		Check(!DecodeNetworkEvent(*exchangeFrame).has_value(), "Multiplex_ExchangeFrame_IsNotAnEvent");
		Check(DecodeStateUpdate(stateFrame).has_value(), "Multiplex_StateUpdate_StillDecodesNormally");
	}

	if (g_Failures == 0)
	{
		std::printf("All tests passed.\n");
		return 0;
	}

	std::printf("%d test(s) failed.\n", g_Failures);
	return 1;
}
