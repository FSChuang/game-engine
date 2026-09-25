// Lightweight, non-SDL test client for Milestone 2 Section 2 integration verification.
// Each invocation is a single, self-contained request/reply exchange over a fresh
// connection — real Engine::Socket + Engine::Protocol, no reimplemented ZeroMQ calls.
// It is test tooling only, not the future Spare Parts game client.
//
// Usage:
//   TestClient join
//   TestClient update <id> <posX> <posY> <velX> <velY>
//   TestClient leave <id>
//
// Prints one line to stdout describing the decoded reply:
//   SNAPSHOT <recipientId> <rosterCount> [<id> <posX> <posY> <velX> <velY>]...
//   ERROR <code>
// Exit code 0 for a decoded Snapshot, 1 for anything else (error reply or malformed
// input/reply), so a driving shell script can also check $? if it wants to.

#include "Engine/Network/Protocol.h"
#include "Engine/Network/Socket.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace
{
	constexpr const char* Endpoint = "tcp://127.0.0.1:5556";

	void PrintSnapshot(const Engine::Snapshot& snapshot)
	{
		std::printf("SNAPSHOT %u %zu", snapshot.RecipientId, snapshot.Roster.size());
		for (const Engine::PlayerState& state : snapshot.Roster)
		{
			std::printf(" %u %f %f %f %f", state.Id, state.PositionX, state.PositionY, state.VelocityX,
			            state.VelocityY);
		}
		std::printf("\n");
	}

	int SendAndReport(const std::vector<std::uint8_t>& requestBytes)
	{
		Engine::Socket client(Engine::SocketRole::Request);
		client.Connect(Endpoint);
		client.Send(std::string(requestBytes.begin(), requestBytes.end()));

		std::string replyFrame = client.Receive();
		std::vector<std::uint8_t> replyBytes(replyFrame.begin(), replyFrame.end());

		std::optional<Engine::MessageType> type = Engine::PeekMessageType(replyBytes);
		if (type == Engine::MessageType::Snapshot)
		{
			std::optional<Engine::Snapshot> snapshot = Engine::DecodeSnapshot(replyBytes);
			if (!snapshot.has_value())
			{
				std::fprintf(stderr, "TestClient: malformed SNAPSHOT reply\n");
				return 1;
			}
			PrintSnapshot(*snapshot);
			return 0;
		}

		if (type == Engine::MessageType::Error)
		{
			std::optional<Engine::ErrorResponse> error = Engine::DecodeError(replyBytes);
			if (!error.has_value())
			{
				std::fprintf(stderr, "TestClient: malformed ERROR reply\n");
				return 1;
			}
			std::printf("ERROR %u\n", static_cast<unsigned>(error->Code));
			return 1;
		}

		std::fprintf(stderr, "TestClient: unexpected or unrecognized reply\n");
		return 1;
	}
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::fprintf(stderr, "usage: TestClient join | update <id> <posX> <posY> <velX> <velY> | leave <id>\n");
		return 1;
	}

	std::string action = argv[1];

	if (action == "join" && argc == 2)
	{
		return SendAndReport(Engine::EncodeJoinRequest());
	}

	if (action == "update" && argc == 7)
	{
		Engine::PlayerState state{ static_cast<Engine::PlayerId>(std::strtoul(argv[2], nullptr, 10)),
			                        std::strtof(argv[3], nullptr), std::strtof(argv[4], nullptr),
			                        std::strtof(argv[5], nullptr), std::strtof(argv[6], nullptr) };
		return SendAndReport(Engine::EncodeStateUpdate(Engine::StateUpdate{ state, false }));
	}

	if (action == "leave" && argc == 3)
	{
		Engine::PlayerId id = static_cast<Engine::PlayerId>(std::strtoul(argv[2], nullptr, 10));
		Engine::PlayerState state{ id, 0.0f, 0.0f, 0.0f, 0.0f };
		return SendAndReport(Engine::EncodeStateUpdate(Engine::StateUpdate{ state, true }));
	}

	std::fprintf(stderr, "usage: TestClient join | update <id> <posX> <posY> <velX> <velY> | leave <id>\n");
	return 1;
}
