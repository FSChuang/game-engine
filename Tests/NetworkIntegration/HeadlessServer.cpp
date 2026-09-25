// Milestone 2 Section 2 headless server: binds a REP socket, applies each request to
// a PlayerRegistry via Engine::HandleRequest, and replies exactly once per request.
// Stops deterministically after a fixed request count (no signal-handling framework
// yet; an unbounded loop belongs to the later real server executable).
//
// This is integration-test tooling, not the eventual Spare Parts submission server —
// it lives under Tests/, not Engine/src/Engine/Network/, which stays library code only.

#include "Engine/Network/PlayerRegistry.h"
#include "Engine/Network/ServerDispatch.h"
#include "Engine/Network/Socket.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace
{
	constexpr const char* Endpoint = "tcp://127.0.0.1:5556"; // distinct from the PING/PONG smoke's port
}

int main(int argc, char* argv[])
{
	if (argc != 3 || std::string(argv[1]) != "--max-requests")
	{
		std::fprintf(stderr, "usage: HeadlessServer --max-requests N\n");
		return 1;
	}

	long maxRequests = std::strtol(argv[2], nullptr, 10);
	if (maxRequests <= 0)
	{
		std::fprintf(stderr, "HeadlessServer: N must be a positive integer\n");
		return 1;
	}

	Engine::Socket server(Engine::SocketRole::Reply);
	server.Bind(Endpoint);

	Engine::PlayerRegistry registry;

	for (long i = 0; i < maxRequests; ++i)
	{
		std::string requestFrame = server.Receive();
		std::vector<std::uint8_t> requestBytes(requestFrame.begin(), requestFrame.end());

		std::vector<std::uint8_t> replyBytes = Engine::HandleRequest(registry, requestBytes);

		server.Send(std::string(replyBytes.begin(), replyBytes.end()));
	}

	return 0;
}
