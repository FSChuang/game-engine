// Headless end-to-end proof that Engine::Socket's REQ half works: connect, send a
// request, require the exact expected reply, exit. No SDL, no Game, no Entity, no
// Timeline, no threads (Milestone 2 Section 2 transport vertical slice).

#include "Engine/Network/Socket.h"

#include <cstdio>

namespace
{
	constexpr const char* Endpoint = "tcp://127.0.0.1:5555";
	constexpr const char* Request = "PING";
	constexpr const char* ExpectedReply = "PONG";
}

int main()
{
	Engine::Socket client(Engine::SocketRole::Request);
	client.Connect(Endpoint);

	client.Send(Request);
	std::string reply = client.Receive();

	if (reply != ExpectedReply)
	{
		std::fprintf(stderr, "NetworkSmokeClient: expected \"%s\", got \"%s\"\n", ExpectedReply, reply.c_str());
		return 1;
	}

	std::printf("NetworkSmokeClient: received \"%s\" as expected\n", reply.c_str());
	return 0;
}
