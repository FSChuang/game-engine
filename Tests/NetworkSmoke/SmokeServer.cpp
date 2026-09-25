// Headless end-to-end proof that Engine::Socket's REP half works: bind, receive
// exactly one request, reply once, exit. No SDL, no Game, no Entity, no Timeline,
// no threads (Milestone 2 Section 2 transport vertical slice).

#include "Engine/Network/Socket.h"

#include <cstdio>

namespace
{
	constexpr const char* Endpoint = "tcp://127.0.0.1:5555";
	constexpr const char* ExpectedRequest = "PING";
	constexpr const char* Reply = "PONG";
}

int main()
{
	Engine::Socket server(Engine::SocketRole::Reply);
	server.Bind(Endpoint);

	std::string request = server.Receive();
	if (request != ExpectedRequest)
	{
		std::fprintf(stderr, "NetworkSmokeServer: expected \"%s\", got \"%s\"\n", ExpectedRequest, request.c_str());
		return 1;
	}

	server.Send(Reply);
	return 0;
}
