// Focused unit tests for Engine::EventProtocol (Milestone 4 Checkpoint 3): the network event
// notification wire format. Pure encode/decode logic. This target links EngineNetworkCore
// only — no SDL, no ZeroMQ — which is itself the proof the format needs neither.

#include "Engine/Network/EventProtocol.h"
#include "Engine/Network/ExchangeProtocol.h"
#include "Engine/Network/Protocol.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
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

// The bit pattern of a float, so expected Shoot bytes are written out by hand below rather than
// produced by the code under test.
static std::uint32_t BitsOf(float value)
{
	std::uint32_t bits;
	std::memcpy(&bits, &value, sizeof(bits));
	return bits;
}

static NetworkEvent ShootEvent(PlayerId id, float x, float y, float heading)
{
	NetworkEvent event{ NetworkEventKind::Shoot, id };
	event.OriginX = x;
	event.OriginY = y;
	event.HeadingRadians = heading;
	return event;
}

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
		for (int kind : { 0, 4, 5, 100, 255 })
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

	// ---- Spawn ----
	{
		NetworkEvent original{ NetworkEventKind::Spawn, 0x0A0B0C0Du };
		std::vector<std::uint8_t> bytes = EncodeNetworkEvent(original);
		Check(bytes == std::vector<std::uint8_t>({ 0x10, 0x02, 0x0A, 0x0B, 0x0C, 0x0D }), "Spawn_Encode_ExactSixBytes");
		auto decoded = DecodeNetworkEvent(bytes);
		Check(decoded.has_value() && decoded->Kind == NetworkEventKind::Spawn && decoded->SourcePlayerId == 0x0A0B0C0Du,
		      "Spawn_RoundTrip_KindAndSourceId");
		Check(decoded.has_value() && decoded->OriginX == 0.0f && decoded->OriginY == 0.0f && decoded->HeadingRadians == 0.0f,
		      "Spawn_HasNoShootPayload");
		Check(!DecodeNetworkEvent(EncodeNetworkEvent({ NetworkEventKind::Spawn, 0 })).has_value(), "Spawn_ZeroPlayerId_Rejected");
	}

	// ---- Shoot: round trip and exact bytes ----
	{
		NetworkEvent original = ShootEvent(0x01020304u, 516.5f, -12.25f, 1.5f);
		std::vector<std::uint8_t> bytes = EncodeNetworkEvent(original);
		Check(bytes.size() == 18, "Shoot_Encode_ExactSize_EighteenBytes");
		// 516.5f = 0x44012000, -12.25f = 0xC1440000, 1.5f = 0x3FC00000, written out by hand.
		Check(bytes == std::vector<std::uint8_t>({ 0x10, 0x03, 0x01, 0x02, 0x03, 0x04, 0x44, 0x01, 0x20, 0x00, 0xC1,
		                                           0x44, 0x00, 0x00, 0x3F, 0xC0, 0x00, 0x00 }),
		      "Shoot_Encode_ExactBytes_TagKindIdThenThreeBigEndianFloats");
		auto decoded = DecodeNetworkEvent(bytes);
		Check(decoded.has_value() && decoded->Kind == NetworkEventKind::Shoot && decoded->SourcePlayerId == 0x01020304u,
		      "Shoot_RoundTrip_KindAndSourceId");
		Check(decoded.has_value() && decoded->OriginX == 516.5f && decoded->OriginY == -12.25f, "Shoot_RoundTrip_Origin");
		Check(decoded.has_value() && decoded->HeadingRadians == 1.5f, "Shoot_RoundTrip_Heading");
	}
	{
		bool allExact = true;
		for (float x : { 0.0f, -0.0f, 1920.0f, -48.0f, 1943.0f, 0.1f, 1.0e-30f, 3.0e30f })
		{
			for (float heading : { 0.0f, 3.14159265f, -3.14159265f, 1.5707964f, -2.3561945f, 6.2831855f })
			{
				auto decoded = DecodeNetworkEvent(EncodeNetworkEvent(ShootEvent(9, x, -x, heading)));
				allExact = allExact && decoded.has_value() && BitsOf(decoded->OriginX) == BitsOf(x) &&
				           BitsOf(decoded->OriginY) == BitsOf(-x) && BitsOf(decoded->HeadingRadians) == BitsOf(heading);
			}
		}
		Check(allExact, "Shoot_RoundTrip_NegativeAndRepresentativeValues_BitExact");
	}

	// ---- Shoot / Death / Spawn are not confused with one another ----
	{
		std::vector<std::uint8_t> shoot = EncodeNetworkEvent(ShootEvent(5, 1.0f, 2.0f, 3.0f));
		std::vector<std::uint8_t> death = EncodeNetworkEvent({ NetworkEventKind::Death, 5 });
		std::vector<std::uint8_t> spawn = EncodeNetworkEvent({ NetworkEventKind::Spawn, 5 });

		// A Shoot frame relabelled Death or Spawn has the wrong length for that kind.
		std::vector<std::uint8_t> shootAsDeath = shoot;
		shootAsDeath[1] = static_cast<std::uint8_t>(NetworkEventKind::Death);
		std::vector<std::uint8_t> shootAsSpawn = shoot;
		shootAsSpawn[1] = static_cast<std::uint8_t>(NetworkEventKind::Spawn);
		Check(!DecodeNetworkEvent(shootAsDeath).has_value() && !DecodeNetworkEvent(shootAsSpawn).has_value(),
		      "Kinds_ShootFrameRelabelledAsDeathOrSpawn_Rejected");

		// A short frame relabelled Shoot is too short to be one.
		std::vector<std::uint8_t> deathAsShoot = death;
		deathAsShoot[1] = static_cast<std::uint8_t>(NetworkEventKind::Shoot);
		std::vector<std::uint8_t> spawnAsShoot = spawn;
		spawnAsShoot[1] = static_cast<std::uint8_t>(NetworkEventKind::Shoot);
		Check(!DecodeNetworkEvent(deathAsShoot).has_value() && !DecodeNetworkEvent(spawnAsShoot).has_value(),
		      "Kinds_DeathOrSpawnFrameRelabelledAsShoot_Rejected");

		Check(DecodeNetworkEvent(death)->Kind == NetworkEventKind::Death && DecodeNetworkEvent(spawn)->Kind == NetworkEventKind::Spawn &&
		          DecodeNetworkEvent(shoot)->Kind == NetworkEventKind::Shoot,
		      "Kinds_EachDecodesAsItself");
		Check(death.size() == 6 && spawn.size() == 6 && shoot.size() == 18, "Kinds_FrameSizesAreSixSixEighteen");
	}

	// ---- Shoot validation ----
	{
		std::vector<std::uint8_t> good = EncodeNetworkEvent(ShootEvent(7, 100.0f, 200.0f, 0.5f));
		bool truncatedRejected = true;
		for (std::size_t length = 0; length < good.size(); ++length)
		{
			std::vector<std::uint8_t> truncated(good.begin(), good.begin() + static_cast<std::ptrdiff_t>(length));
			truncatedRejected = truncatedRejected && !DecodeNetworkEvent(truncated).has_value();
		}
		Check(truncatedRejected, "ShootDecode_EveryTruncation_Rejected");

		std::vector<std::uint8_t> padded = good;
		padded.push_back(0);
		Check(!DecodeNetworkEvent(padded).has_value(), "ShootDecode_PaddedFrame_Rejected");

		std::vector<std::uint8_t> wrongTag = good;
		wrongTag[0] = 0x11;
		Check(!DecodeNetworkEvent(wrongTag).has_value(), "ShootDecode_WrongTag_Rejected");

		Check(!DecodeNetworkEvent(EncodeNetworkEvent(ShootEvent(0, 1.0f, 2.0f, 3.0f))).has_value(),
		      "ShootDecode_ZeroPlayerId_Rejected");

		const float nan = std::numeric_limits<float>::quiet_NaN();
		const float inf = std::numeric_limits<float>::infinity();
		Check(!DecodeNetworkEvent(EncodeNetworkEvent(ShootEvent(7, nan, 2.0f, 3.0f))).has_value(), "ShootDecode_NaNOriginX_Rejected");
		Check(!DecodeNetworkEvent(EncodeNetworkEvent(ShootEvent(7, 1.0f, nan, 3.0f))).has_value(), "ShootDecode_NaNOriginY_Rejected");
		Check(!DecodeNetworkEvent(EncodeNetworkEvent(ShootEvent(7, 1.0f, 2.0f, nan))).has_value(), "ShootDecode_NaNHeading_Rejected");
		Check(!DecodeNetworkEvent(EncodeNetworkEvent(ShootEvent(7, inf, 2.0f, 3.0f))).has_value(), "ShootDecode_PositiveInfinityOrigin_Rejected");
		Check(!DecodeNetworkEvent(EncodeNetworkEvent(ShootEvent(7, 1.0f, -inf, 3.0f))).has_value(), "ShootDecode_NegativeInfinityOrigin_Rejected");
		Check(!DecodeNetworkEvent(EncodeNetworkEvent(ShootEvent(7, 1.0f, 2.0f, inf))).has_value(), "ShootDecode_InfiniteHeading_Rejected");
		Check(!DecodeNetworkEvent(EncodeNetworkEvent(ShootEvent(7, 1.0f, 2.0f, -inf))).has_value(), "ShootDecode_NegativeInfiniteHeading_Rejected");
	}

	// ---- Shoot shares the channel safely too ----
	{
		std::vector<std::uint8_t> shoot = EncodeNetworkEvent(ShootEvent(3, 1.0f, 2.0f, 3.0f));
		Check(IsNetworkEventFrame(shoot), "Multiplex_ShootFrame_IsRoutedAsAnEvent");
		Check(!DecodeStateUpdate(shoot).has_value(), "Multiplex_ShootFrame_IsNotAStateUpdate");
		Check(!DecodeExchangeFrame(shoot).has_value(), "Multiplex_ShootFrame_IsNotAnExchangeFrame");
	}

	if (g_Failures == 0)
	{
		std::printf("All tests passed.\n");
		return 0;
	}

	std::printf("%d test(s) failed.\n", g_Failures);
	return 1;
}
